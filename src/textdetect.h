//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Text detection and recognition class (.h, .cpp)
// using Efficient and Accurate Scene Text Detection (EAST) and text recognition library (tesseract)
//*****************************************************************************/

#ifndef __TEXTDETECT_H
#define __TEXTDETECT_H

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/dnn.hpp>


using namespace cv;
using namespace cv::dnn;



class TextDetector
{
public:
	TextDetector();
	~TextDetector();

	bool loadNet(std::string model_path);
    bool detect(Mat& image, std::vector<Rect>& text_region_list);
    bool detect(Mat& image, std::vector<RotatedRect>& text_region_list);
    bool opened() { return m_Opened; }

private:
    void decode(const Mat& scores, const Mat& geometry, float scoreThresh, \
                std::vector<RotatedRect>& detections, std::vector<float>& confidences);
	
private:
	bool 	m_Opened;
    Net     m_Net;
	
    int     m_Width;
    int     m_Height;
	float 	m_CnfThreshold;
	float 	m_NMSThreshold;
	
};



#endif
