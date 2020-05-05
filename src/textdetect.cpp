//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Text detection and recognition class (.h, .cpp)
// using Efficient and Accurate Scene Text Detection (EAST) and text recognition library (tesseract)
//*****************************************************************************/

#include "textdetect.h"

#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include <fstream>



TextDetector::TextDetector()
{
	m_Opened = false;

    m_CnfThreshold = 0.95; // confidence threshold
    m_NMSThreshold = 0.9; // non-maximum suppression threshold

    m_Width = 320;
    m_Height = 320;
}

TextDetector::~TextDetector()
{
}

bool TextDetector::loadNet(std::string model_path)
{
	if (m_Opened)
		return true;
	
    m_Net = readNet(model_path);
    if (m_Net.empty())
        return false;

    m_Net.setPreferableBackend(0);
    m_Net.setPreferableTarget(0);

    m_Opened = true;
	return true;
}

bool TextDetector::detect(Mat& image, std::vector<Rect>& text_region_list)
{
	if (!m_Opened)
		return false;
	
	// get blob from image
	Mat blob;
    blobFromImage(image, blob, 1.0, Size(m_Width, m_Height), Scalar(123.68, 116.78, 103.94), false, false); // first false -> RGB swap
	// set blob
    m_Net.setInput(blob);
	
	// network forward for inference
	std::vector<Mat> outs;
	std::vector<String> out_names(2);
    out_names[0] = "feature_fusion/Conv_7/Sigmoid";
    out_names[1] = "feature_fusion/concat_3";
    m_Net.forward(outs, out_names);
	
	// decode predicted bounding boxes
	Mat scores = outs[0];
	Mat geometry = outs[1];
    std::vector<RotatedRect> boxes;
	std::vector<float> confidences;
    this->decode(scores, geometry, m_CnfThreshold, boxes, confidences);
	
	// apply non-maximum suppression procedure.
	std::vector<int> indices;
    NMSBoxes(boxes, confidences, m_CnfThreshold, m_NMSThreshold, indices);

    float ratio_x = (float)image.cols / m_Width;
    float ratio_y = (float)image.rows / m_Height;

    text_region_list.clear();
    for (size_t n = 0; n < indices.size(); n++) {
        Rect bbox = boxes[indices[n]].boundingRect();

        int xmin = MAX((bbox.x * ratio_x - 1), 0);
        int ymin = MAX((bbox.y * ratio_y - 1), 0);
        int xmax = MIN(((bbox.x + bbox.width + 0) * ratio_x), image.cols-1);
        int ymax = MIN(((bbox.y + bbox.height + 0) * ratio_y), image.rows-1);
        bbox.x = xmin;
        bbox.y = ymin;
        bbox.width = xmax - xmin + 1;
        bbox.height = ymax - ymin + 1;
        text_region_list.push_back(bbox);
    }
    //printf("# detected text regions: %d\n", (int)text_region_list.size());

	// put efficiency information.
    //std::vector<double> layersTimes;
    //double freq = getTickFrequency() / 1000;
    //double t = m_Net.getPerfProfile(layersTimes) / freq;
    //std::string label = format("Inference time: %.2f ms", t);
	
    return ((text_region_list.size() > 0) ? true : false);
}

bool TextDetector::detect(Mat& image, std::vector<RotatedRect>& text_region_list)
{
    if (!m_Opened)
        return false;

    return true;
}

void TextDetector::decode(const Mat& scores, const Mat& geometry, float scoreThresh, std::vector<RotatedRect>& detections, std::vector<float>& confidences)
{
	detections.clear();
	CV_Assert(scores.dims == 4); CV_Assert(geometry.dims == 4); CV_Assert(scores.size[0] == 1);
	CV_Assert(geometry.size[0] == 1); CV_Assert(scores.size[1] == 1); CV_Assert(geometry.size[1] == 5);
	CV_Assert(scores.size[2] == geometry.size[2]); CV_Assert(scores.size[3] == geometry.size[3]);

	const int height = scores.size[2];
	const int width = scores.size[3];
	for (int y = 0; y < height; ++y) {
		const float* scoresData = scores.ptr<float>(0, 0, y);
		const float* x0_data = geometry.ptr<float>(0, 0, y);
		const float* x1_data = geometry.ptr<float>(0, 1, y);
		const float* x2_data = geometry.ptr<float>(0, 2, y);
		const float* x3_data = geometry.ptr<float>(0, 3, y);
		const float* anglesData = geometry.ptr<float>(0, 4, y);
		for (int x = 0; x < width; ++x) {
			float score = scoresData[x];
			if (score < scoreThresh)
				continue;

			// decode a prediction.
			// multiple by 4 because feature maps are 4 time less than input image.
			float offsetX = x * 4.0f, offsetY = y * 4.0f;
			float angle = anglesData[x];
			float cosA = std::cos(angle);
			float sinA = std::sin(angle);
			float h = x0_data[x] + x2_data[x];
			float w = x1_data[x] + x3_data[x];

			Point2f offset(offsetX + cosA * x1_data[x] + sinA * x2_data[x],
			offsetY - sinA * x1_data[x] + cosA * x2_data[x]);
			Point2f p1 = Point2f(-sinA * h, -cosA * h) + offset;
			Point2f p3 = Point2f(-cosA * w, sinA * w) + offset;
            RotatedRect r(0.5f * (p1 + p3), Size2f(w, h), -angle * 180.0f / (float)CV_PI);
			detections.push_back(r);
			confidences.push_back(score);
		}
	}
}
