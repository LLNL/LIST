//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// SEM image process classes (.h, .cpp)
//*****************************************************************************/

// 2nd last update: 08/07/2018
// last update: 10/09/2018

#include "semproc.h"
#include "semutil.h"

#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <iostream>
#include <fstream>
#include <algorithm>


const std::string g_ShapeType[3] = {"unknown", "ellipse", "rectangle"};



///////////////////////////////////////////////////////////////////////////////
// SEMScaleBar class
///////////////////////////////////////////////////////////////////////////////

SEMScaleBar::SEMScaleBar()
{
    m_Tesseract = new tesseract::TessBaseAPI();
}

SEMScaleBar::~SEMScaleBar()
{
    if (m_Tesseract) {
        m_Tesseract->End();
        delete m_Tesseract;
    }
}

bool SEMScaleBar::initTextDetector(const char* model_path)
{
    std::ifstream f(model_path);
    if (!f.good())
        return false;
    f.close();

    return m_TextDetector.loadNet(model_path);
}

bool SEMScaleBar::initTextRecognizer(const char* data_path)
{
    // initialize tesseract text recognizer to use English (eng) and the LSTM OCR engine.
    if (m_Tesseract->Init(data_path, "eng", tesseract::OEM_LSTM_ONLY)) {
        printf("can't initialize tesseract\n");
        return false;
    }

    // Set Page segmentation mode to PSM_AUTO (3)
    m_Tesseract->SetPageSegMode(tesseract::PSM_AUTO);
    return true;
}

bool SEMScaleBar::openImage(const char* fileName)
{
    // open image using opencv
    m_cvImage = imread(fileName, IMREAD_COLOR);
    if (!m_cvImage.data) {
        return false;
    }

    // create image object
    getImageObject(m_cvImage, m_Image, 3);

    // adjust TScalebarSegmenter parameter, based on the image size
    float ratio = (float)m_Image.getWidth()  / m_Param.base_width;
    m_Param.min_thickness = MAX(m_Param.getBaseMinThickness() * ratio, 2); // thickness 1 doesn't make sense
    m_Param.max_thickness = MAX(m_Param.getBaseMaxThickness() * ratio, 2);
    m_Param.min_length = MAX(m_Param.getBaseMinLength() * ratio, 10);
    m_Param.max_length = MAX(m_Param.getBaseMaxLength() * ratio, 10);

    // initialize scalebar detection
    m_ScaleBarList.clear();
    m_ScaleInfoList.clear();

    return true;
}

bool SEMScaleBar::detectScaleBar()
{
    if (m_Image.getWidth() == 0 || m_Image.getPixels() == NULL)
        return false;

    // do segmentation
    CSegmenter bsegmenter(&m_Image);
    bsegmenter.segmentSimpleRegionGrowing(1, 0, 0, m_Param.threshold);
    bsegmenter.pruneBySegmentSize(m_Param.min_length * m_Param.min_thickness);
    //bsegmenter.saveToImageFile("/Users/kim63/Desktop/scalebar_bseg.png", 0, true, MAKE_UCHAR3(0, 0, 255));

    // get segment info
    std::vector<INT4> bbox_list;
    std::vector<int> bbox_sid_list;
    for (int sid = 0; sid < bsegmenter.getNumSegments(); sid++) {
        CSegment* segment = bsegmenter.getSegment(sid);
        int xmin, ymin, xmax, ymax;
        segment->getBoundBox(xmin, ymin, xmax, ymax);
        int length = xmax - xmin + 1;
        int thickness = ymax - ymin + 1;
        int numpixels = segment->getNumPixels();
        if (length < m_Param.min_length)
            continue;
        if (thickness < m_Param.min_thickness)
            continue;

        //printf("(%d %d %d %d) \n", xmin, ymin, xmax, ymax);

        // compute actual horizontal scalebar's length and thickness, and completeness
        int length_new = 1;
        std::vector<int> pcounts;
        for (int y = ymin; y <= ymax; y++) {
            int line_pcount = 0;
            for (int x = xmin; x <= xmax; x++) {
                if (bsegmenter.getSegmentId(x, y) == sid)
                    line_pcount++;
            }
            pcounts.push_back(line_pcount);
            length_new = MAX(length_new, line_pcount);
        }
        int line_pcount_threshold = length_new * 4.0 / 5.0;
        int ymin_new = ymax + 1;
        int ymax_new = ymin - 1;
        for (int y = ymin, n = 0; y <= ymax; y++, n++) {
            if (pcounts[n] >= line_pcount_threshold) {
                ymin_new = MIN(ymin_new, y);
                ymax_new = MAX(ymax_new, y);
            }
        }
        int thickness_new = ymax_new - ymin_new + 1;
        float completeness = (float)(length_new * thickness_new) / numpixels;
        float aratio = (float)length_new / thickness_new;
        //printf("(%d %d %d %d) length: %d %d, thickness: %d %d, completeness: %f, aratio=%f\n", \
        //        xmin, ymin, xmax, ymax, length, length_new, thickness, thickness_new, completeness, aratio);
        if (completeness < m_Param.completeness || completeness > 1.1)
            continue;
        if (length_new < m_Param.min_length || length_new > m_Param.max_length)
            continue;
        if (thickness_new < m_Param.min_thickness || thickness_new > m_Param.max_thickness)
            continue;
        if (aratio < m_Param.getBaseMinARatio() || aratio > m_Param.getBaseMaxARatio())
            continue;

        //printf("min_thickness: %d\n", m_Param.min_thickness);
        //printf("(%d %d %d %d) length: %d %d, thickness: %d %d, completeness: %f, aratio: %f\n", \
        //       xmin, ymin, xmax, ymax, length, length_new, thickness, thickness_new, completeness, aratio);
        bbox_list.push_back(MAKE_INT4(xmin, ymin, xmax, ymax));
        bbox_sid_list.push_back(sid);
    }

    // do Hough line transform
    //vector<int> line_y_list;
    //int nlines = detect_hline(cvimage, line_y_list);

    // get final list
    std::vector<INT4> scalebar_list;
    std::vector<int> scalebar_sid_list;
    for (size_t n = 0; n < bbox_list.size(); n++) {
        INT4 bbox = bbox_list[n];
        int bbox_sid = bbox_sid_list[n];
        /*bool found = false;
        for (size_t m = 0; m < line_y_list.size(); m++) {
            int y = line_y_list[m];
            if (y >= bbox.y && y <= bbox.w) {
                found = true;
                break;
            }
        }
        if (!found)
            continue;*/

        // check overlapping
        if (scalebar_list.size() == 0) {
            scalebar_list.push_back(bbox);
            scalebar_sid_list.push_back(bbox_sid);
        }
        else {
            for (size_t m = 0; m < scalebar_list.size(); m++) {
                INT4 bbox2 = scalebar_list[m];
                float ratio = check_overlap(bbox, bbox2);
                if (fabs(ratio) < 0.5) {
                    scalebar_list.push_back(bbox);
                    scalebar_sid_list.push_back(bbox_sid);
                }
                else if (ratio == 1) { // replace with smaller one
                    scalebar_list[m] = bbox;
                    scalebar_sid_list[m] = bbox_sid;
                }
                else if (ratio == -1) { // do nothing, ignore bigger one
                }
            }
        }
    }
    //printf("scalebar detected: %d\n", (int)scalebar_list.size());
    //printf("%d %d", scalebar_list[0].x, scalebar_list[0].y);
    //printf("%d %d", scalebar_list[1].x, scalebar_list[1].y);

    // set to list
    m_ScaleBarList.clear();
    m_ScaleInfoList.clear();
    for (size_t n = 0; n < scalebar_list.size(); n++) {
        INT4 bbox = scalebar_list[n];
        int y1 = m_Image.getHeight() - 1 - bbox.w;
        int y2 = m_Image.getHeight() - 1 - bbox.y;
        m_ScaleBarList.push_back(MAKE_INT4(bbox.x, y1, bbox.z, y2));
    }

    return true;
}

bool SEMScaleBar::detectScaleText()
{
    if (m_Image.getWidth() == 0 || m_Image.getPixels() == NULL)
        return false;
    if (m_ScaleBarList.size() == 0)
        return false;
    std::string lang_str = "eng";
    if (lang_str.compare(m_Tesseract->GetInitLanguagesAsString()) != 0)
        return false;

    m_ScaleInfoList.clear();
    for (size_t n = 0; n < m_ScaleBarList.size(); n++) {
        INT4 sb_bbox = m_ScaleBarList[n];
        int sb_width = sb_bbox.z - sb_bbox.x + 1;
        int sb_height = sb_bbox.w - sb_bbox.y + 1;

        bool scale_detected = false;
        int scale_number = 0;
        int scale_unit = 0;

        // if EAST text detector is available
        if (m_TextDetector.opened()) {
            INT4 cand_bbox;
            cand_bbox = MAKE_INT4(sb_bbox.x-sb_width*2, sb_bbox.y-sb_height*20, sb_bbox.z+sb_width*2, sb_bbox.w+sb_height*20);
            //printf("%d, %d, cand_bbox: %d %d %d %d\n", m_Image.getWidth(), m_Image.getHeight(), cand_bbox.x, cand_bbox.y, cand_bbox.z, cand_bbox.w);
            if (cand_bbox.x > m_Image.getWidth() - 10 || cand_bbox.z < 10 || cand_bbox.y > m_Image.getHeight() - 10 || cand_bbox.w < 10)
                continue;
            cand_bbox.x = MAX(cand_bbox.x, 0);
            cand_bbox.y = MAX(cand_bbox.y, 0);
            cand_bbox.z = MIN(cand_bbox.z, m_Image.getWidth() - 1);
            cand_bbox.w = MIN(cand_bbox.w, m_Image.getHeight() - 1);

            Rect cr_rect(cand_bbox.x, cand_bbox.y, cand_bbox.z-cand_bbox.x, cand_bbox.w-cand_bbox.y);
            Mat cropped(m_cvImage, cr_rect);
            Mat cropped_smooth;
            medianBlur(cropped, cropped_smooth, 3);
            //if (n == 0)
            //    imwrite("/Users/kim63/Desktop/aaa.png", cropped_smooth);

            std::vector<Rect> region_list;
            if (!m_TextDetector.detect(cropped_smooth, region_list))
                continue;

            for (size_t m = 0; m < region_list.size(); m++) {
                Rect text_bbox = region_list[m];
                Mat cropped_text(cropped_smooth, text_bbox);
                //if (n == 0) {
                //    printf("text_bbox: %d %d %d %d\n", text_bbox.x, text_bbox.y, text_bbox.width, text_bbox.height);
                //    char path[255];
                //    sprintf(path, "/Users/kim63/Desktop/aaa_%d.png", m);
                //    imwrite(path, cropped_text);
                //}

                m_Tesseract->SetImage(cropped_text.data, cropped_text.cols, cropped_text.rows, 3, cropped_text.step);
                std::string out_text = m_Tesseract->GetUTF8Text();
                remove_if(out_text.begin(), out_text.end(), isspace);
                //printf("%s\n", out_text.c_str());

                // extract number and unit
                int number, unit;
                if (this->parseScaleNumber(out_text, number, unit)) {
                    scale_detected = true;
                    scale_number = number;
                    scale_unit = unit;
                    //printf("detected!\n");
                    break;
                }
            }
        }
        // if not, brute-force search
        else {
            // currently only bottom, top and right regions are implemented
            for (int m = 0; m < 3; m++) {
                INT4 cand_bbox;
                if (m == 0) { // bottom
                    cand_bbox = MAKE_INT4(sb_bbox.x-sb_width/2, sb_bbox.w+1, sb_bbox.z+sb_width/2, sb_bbox.w+sb_height*10);
                }
                else if (m == 1) { // top
                    cand_bbox = MAKE_INT4(sb_bbox.x-sb_width/2, sb_bbox.y-sb_height*10, sb_bbox.z+sb_width/2, sb_bbox.y-1);
                }
                else if (m == 2) { // right
                    cand_bbox = MAKE_INT4(sb_bbox.z+1, sb_bbox.y-sb_height*10, sb_bbox.z+1+sb_width*2, sb_bbox.w+sb_height*10);
                }
                if (cand_bbox.x > m_Image.getWidth() - 10 || cand_bbox.z < 10 || cand_bbox.y > m_Image.getHeight() - 10 || cand_bbox.w < 10)
                    continue;

                cand_bbox.x = max(cand_bbox.x, 0);
                cand_bbox.y = max(cand_bbox.y, 0);
                cand_bbox.z = min(cand_bbox.z, m_Image.getWidth() - 1);
                cand_bbox.w = min(cand_bbox.w, m_Image.getHeight() - 1);
                //printf("m=%d, w=%d, h=%d, %d %d %d %d\n", m, m_Image.getWidth(), m_Image.getHeight(), cand_bbox.x, cand_bbox.y, cand_bbox.z, cand_bbox.w);

                Rect cr_rect(cand_bbox.x, cand_bbox.y, cand_bbox.z-cand_bbox.x, cand_bbox.w-cand_bbox.y);
                Mat cropped(m_cvImage, cr_rect);
                Mat cropped_smooth;
                medianBlur(cropped, cropped_smooth, 3);
                //if (n == 0 && m == 0)
                //    imwrite("/Users/kim63/Desktop/aaa.png", cropped_smooth);

                // Set cropped image
                m_Tesseract->SetImage(cropped_smooth.data, cropped_smooth.cols, cropped_smooth.rows, 3, cropped_smooth.step);
                std::string out_text = m_Tesseract->GetUTF8Text();
                remove_if(out_text.begin(), out_text.end(), isspace);
                //printf("%s\n", out_text.c_str());

                // extract number and unit
                int number, unit;
                if (this->parseScaleNumber(out_text, number, unit)) {
                    scale_detected = true;
                    scale_number = number;
                    scale_unit = unit;
                    break;
                }

            }
        }

        if (scale_detected)
            m_ScaleInfoList.push_back(MAKE_INT3(n, scale_number, scale_unit));
    }

    return (m_ScaleInfoList.size() > 0) ? true : false;
}

void SEMScaleBar::manualSelect(int xmin, int ymin, int xmax, int ymax, int number, int unit)
{
    m_ScaleBarList.clear();
    m_ScaleInfoList.clear();

    m_ScaleBarList.push_back(MAKE_INT4(xmin, ymin, xmax, ymax));
    m_ScaleInfoList.push_back(MAKE_INT3(0, number, unit));
}

bool SEMScaleBar::getDetectedScale(int& scale_length, int& scale_number, int& scale_unit)
{
    if (m_ScaleBarList.size() == 0 || m_ScaleInfoList.size() == 0)
        return false;

    INT3 scale_info = m_ScaleInfoList[0];
    INT4 scalebar_box = m_ScaleBarList[scale_info.x];
    scale_length = scalebar_box.z - scalebar_box.x;
    scale_number = scale_info.y;
    scale_unit = scale_info.z;

    return true;
}

float SEMScaleBar::convert(int length, int scale_length, int scale_number, int scale_unit)
{
    if (length <= 0 || scale_length <= 0)
        return 0;

    float alength = (float)(length * scale_number) / scale_length;
    return alength;
}

int SEMScaleBar::inverse(float length, int scale_length, int scale_number, int scale_unit)
{
    if (length <= 0 || scale_number <= 0)
        return 0;

    float plength = length * scale_length / scale_number;
    return (int)plength;
}

bool SEMScaleBar::parseScaleNumber(std::string text, int& number, int& unit)
{
    // extract number and unit
    int d_ind1 = -1;
    int d_ind2 = -1;
    int u_ind1 = -1;
    for (int i = 0; i < (int)text.length(); i++) {
        char c = text[i];
        if (d_ind1 == -1 && c >= '0' && c <= '9') {
            d_ind1 = i;
        }
        if (d_ind1 != -1 && d_ind2 == -1 && !(c >= '0' && c <= '9')) {
            d_ind2 = i;
        }
        if (d_ind1 != -1 && d_ind2 != -1 && u_ind1 == -1 && (c != ' ') && !(c >= '0' && c <= '9')) {
            u_ind1 = i;
            break;
        }
    }
    //printf("%d %d %d\n", d_ind1, d_ind2, u_ind1);
    if (d_ind1 == -1 || d_ind2 == -1 || u_ind1 == -1)
        return false;

    std::string number_text = text.substr(d_ind1, d_ind2-d_ind1);
    std::string unit_text = text.substr(u_ind1, text.length()-u_ind1+1);
    //printf("%s %s\n", number_text.c_str(), unit_text.c_str());

    number = atoi(number_text.c_str());
    unit = -1;
    if (unit_text.find('u') != std::string::npos || unit_text.find('p') != std::string::npos) // unit_text = "µm"
        unit = 0;
    else if (unit_text.find('n') != std::string::npos) // unit_text = "nm"
        unit = 1;

    return ((unit >= 0) ? true : false);
}



///////////////////////////////////////////////////////////////////////////////
// SEMShape class
///////////////////////////////////////////////////////////////////////////////

SEMShape::SEMShape()
{
}

SEMShape::~SEMShape()
{

}

bool SEMShape::openImage(const char* fileName)
{
    // open image using opencv
    m_cvImage = imread(fileName, IMREAD_GRAYSCALE);
    if (!m_cvImage.data) {
        return false;
    }

    // create image object
    getImageObject(m_cvImage, m_Image, 1);

    // get image statistics (use it later)
    computeImageStat(m_Image, m_Stat);

    // setup min pixel offset to filter out outer segments
    if (m_Param.min_offset == -1)
        m_Param.min_offset = __MIN((int)(m_Image.getHeight() * 0.03), (int)(m_Image.getWidth() * 0.03));

    // initialize other images, variables
    m_AdjImage = m_Image;
    m_BinImage = m_Image;
    m_DistImage = m_Image;
    m_OutImage = m_Image;
    m_ShapeList.clear();

    return true;
}

bool SEMShape::detectShape(bool autoDetect, int shapeType)
{
    if (m_Image.getWidth() == 0 || m_Image.getPixels() == NULL)
        return false;

    // automatically detect shape type and binarization
    bool ret;
    if (autoDetect) { // general (can be used for both core and core-shell types)
        int inv_required, valid_count, valid_shell_count;
        ret = this->detectGeneralCenters(inv_required, valid_count, valid_shell_count);
        if (!ret)
            return false;

        float valid_shell_ratio = (float)valid_shell_count / valid_count;
        m_Param.bin_inv = inv_required;
        m_ShapeType = (valid_shell_ratio > 0.1) ? 2 : 1;
        printf("detected shape type: %d\n", m_ShapeType);
    }
    else {
        m_ShapeType = shapeType;
    }

    // measure core or core-shell sizes
    if (m_ShapeType == 1) {
        ret = this->detectCoreShape();
    }
    else {
        ret = this->detectCoreShellShape();
    }

    return ((ret) ? true : false);
}

bool SEMShape::detectGeneralCenters(int& invRequired, int& validCount, int& validShellCount)
{
    ///////////////////////////////////////////////////////////////////////////
    // 1. preprocessing
    ///////////////////////////////////////////////////////////////////////////

    // do histogram equalization for low contrast between cores and shells
    Mat cvimage_blurred, cvimage_histeq;
    medianBlur(m_cvImage, cvimage_blurred, 5);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(5);
    clahe->apply(cvimage_blurred, cvimage_histeq);
    //equalizeHist(cvimage_blurred, cvimage_histeq); -> create too much contrast
    //imwrite("blurred.png", cvimage_blurred);
    //imwrite("histeq.png", cvimage_histeq);


    ///////////////////////////////////////////////////////////////////////////
    // 2. generate two binarized images
    ///////////////////////////////////////////////////////////////////////////

    // create initial threshold images
    const int bin_types[2] = {THRESH_BINARY | THRESH_OTSU, THRESH_BINARY_INV | THRESH_OTSU};
    int bin_threshold = ((m_Param.bin_threshold == -1) ? m_Stat.mean*255 : m_Param.bin_threshold);
    Mat cvimage_temp[2];
    threshold(cvimage_histeq, cvimage_temp[0], bin_threshold, 255, bin_types[0]);
    threshold(cvimage_histeq, cvimage_temp[1], bin_threshold, 255, bin_types[1]);
    // blur them to remove small artifacts
    Mat cvimage_bin[2];
    medianBlur(cvimage_temp[0], cvimage_bin[0], 5);
    medianBlur(cvimage_temp[1], cvimage_bin[1], 5);

    // erode them -> to separate cores as much as possible
    cvimage_temp[0] = cvimage_bin[0].clone();
    cvimage_temp[1] = cvimage_bin[1].clone();
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), Point(1, 1));
    for (int i = 0; i < 5; i++) {
        Mat temp = cvimage_temp[0].clone();
        erode(temp, cvimage_temp[0], element);
    }
    for (int i = 0; i < 5; i++) {
        Mat temp = cvimage_temp[1].clone();
        erode(temp, cvimage_temp[1], element);
    }
    // blur them to remove small artifacts
    Mat cvimage_bin_erode[2];
    medianBlur(cvimage_temp[0], cvimage_bin_erode[0], 5);
    medianBlur(cvimage_temp[1], cvimage_bin_erode[1], 5);

    //imwrite("bin0_0.png", cvimage_bin[0]);
    //imwrite("bin0_1.png", cvimage_bin[1]);
    //imwrite("/Users/kim63/Desktop/bin1_0.png", cvimage_bin_erode[0]);
    //imwrite("/Users/kim63/Desktop/bin1_1.png", cvimage_bin_erode[1]);

    CImage image_bin[2];
    getImageObject(cvimage_bin[0], image_bin[0], 1);
    getImageObject(cvimage_bin[1], image_bin[1], 1);

    CImage image_bin_erode[2];
    getImageObject(cvimage_bin_erode[0], image_bin_erode[0], 1);
    getImageObject(cvimage_bin_erode[1], image_bin_erode[1], 1);

    CSegmenter* bsegmenter[2];
    bsegmenter[0] = new CSegmenter(&image_bin_erode[0]);
    bsegmenter[1] = new CSegmenter(&image_bin_erode[1]);


    ///////////////////////////////////////////////////////////////////////////
    // 3. pick the correct binarization by counting valid cores
    ///////////////////////////////////////////////////////////////////////////

    int valid_count[2];
    for (int n = 0; n < 2; n++) {
        valid_count[n] = 0;

        // simple segmentation
        bsegmenter[n]->segmentSimpleRegionGrowing(1, 0, 0, m_Param.rg_threshold / 255.0);

        // prune too small segments first
        bsegmenter[n]->pruneBySegmentSize(m_Param.min_size);

        for (int sid = 0; sid < bsegmenter[n]->getNumSegments(); sid++) {
            CSegment* segment = bsegmenter[n]->getSegment(sid);
            segment->setTag(0);

            // skip if this segment belongs to background (black)
            std::vector<INT2> pixels;
            segment->getPixels(pixels);
            float* pixel = image_bin_erode[n].getPixel(pixels[0].x, pixels[0].y);
            if (*pixel < 0.2)
                continue;

            // skip if at least 2 segments are adjacent
            std::vector<int> asids;
            if (segment->getAdjacentSegments(asids) >= 2)
                continue;

            // skip if it's at border
            int bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax;
            segment->getBoundBox(bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax);
            if (bbox_xmin < m_Param.min_offset || bbox_ymin < m_Param.min_offset ||
                bbox_xmax > image_bin_erode[n].getWidth() - m_Param.min_offset ||
                bbox_ymax > image_bin_erode[n].getHeight() - m_Param.min_offset)
                continue;

            // extract/smooth contour
            std::vector<std::vector<INT2> > contours;
            if (segment->getContours(contours) != 1)
                continue;
            std::vector<INT2> contour = contours[0];
            std::vector<INT2> out_contour;
            smoothContour(contour, 2, out_contour);
            smoothContour(out_contour, 2, contour);

            // get segments that are close to their convex hull by comparing the segment area with the convex hull area
            std::vector<Point> cvcontour;
            for (int cp = 0; cp < (int)contour.size(); cp++)
                cvcontour.push_back(Point(contour[cp].x, image_bin_erode[n].getHeight() - 1 - contour[cp].y));
            double area = contourArea(Mat(cvcontour));
            Mat cvhull;
            convexHull(Mat(cvcontour), cvhull);
            double hull_area = contourArea(cvhull);
            float solidity = float(area) / float(hull_area);
            if (solidity < 0.9)
                continue;

            valid_count[n]++;
            segment->setTag(1000);
        }
    }
    printf("valid count: %d %d\n", valid_count[0], valid_count[1]);

    invRequired = 0;
    validCount = valid_count[0];
    CImage* image_bin_true = &image_bin[0];
    Mat* cvimage_bin_true = &cvimage_bin[0];
    CSegmenter* bsegmenter_erode_true = bsegmenter[0];
    if (valid_count[0] < valid_count[1]) {
        invRequired = 1;
        validCount = valid_count[1];
        image_bin_true = &image_bin[1];
        cvimage_bin_true = &cvimage_bin[1];
        bsegmenter_erode_true = bsegmenter[1];
    }
    if (validCount == 0) {
        delete bsegmenter[0];
        delete bsegmenter[1];
        return false;
    }

    // set hist-adj, true binary image
    getImageObject(cvimage_histeq, m_AdjImage, 1);
    m_BinImage = *image_bin_true;

    // generate distance transform image (to get local minimum) -> not used
    //Mat cvimage_dist;
    //distanceTransform(*cvimage_bin_true, cvimage_dist, DIST_L2, DIST_MASK_PRECISE);
    //getImageObject(cvimage_dist, m_DistImage, 1);

    // set initial center list
    m_ShapeList.clear();
    for (int sid = 0; sid < bsegmenter_erode_true->getNumSegments(); sid++) {
        CSegment* segment = bsegmenter_erode_true->getSegment(sid);
        if (segment->getTag() != 1000)
            continue;

        // use segment centroid which seems better than the below algorithm
        INT2 centroid;
        if (segment->getCentroid(centroid) != 1)
            continue;

        /*
        // use local minimum and its neighbors instead of segment centroid -> not good
        // now this is a candidate core, find the local maximum
        std::vector<INT2> pixels;
        segment->getPixels(pixels);
        int local_maximum = 0;
        for (size_t p = 0; p < pixels.size(); p++) {
            INT2 pixel = pixels[p];
            float* ppvalue = m_DistImage.getPixel(pixel.x, pixel.y);
            int pvalue = (int)(*ppvalue * 255.0);
            if (local_maximum < pvalue)
                local_maximum = pvalue;
        }
        if (local_maximum <= 2) // it must be an error
            continue;

        // collect all local maximum pixels
        std::vector<INT2> local_maximum_pixels1;
        std::vector<INT2> local_maximum_pixels2; // broader
        for (size_t p = 0; p < pixels.size(); p++) {
            INT2 pixel = pixels[p];
            float* ppvalue = m_DistImage.getPixel(pixel.x, pixel.y);
            int pvalue = (int)(*ppvalue * 255.0);
            if (pvalue >= (local_maximum - 1)) {
                if (pvalue == local_maximum)
                    local_maximum_pixels1.push_back(pixel);
                local_maximum_pixels2.push_back(pixel);
            }
        }
        //printf("lmaximum size: %d %d\n", local_maximum_pixels1.size(), local_maximum_pixels2.size());

        // find their moment/centroid
        INT2 centroid;
        //getCentroid(local_maximum_pixels1, centroid);
        getCentroid(local_maximum_pixels2, centroid);*/

        // now add the shape center to list
        TShapeInfo shapeinfo;
        shapeinfo.Center.x = centroid.x;
        shapeinfo.Center.y = m_BinImage.getHeight() - 1 - centroid.y;
        m_ShapeList.push_back(shapeinfo);
    }


    ///////////////////////////////////////////////////////////////////////////
    // 4. check shell existence
    // in case of core image: only isolated cores should reach routine* but they will be filtered out eventually
    // in case of core-shell: most (except for error centers) should reach the routine*
    ///////////////////////////////////////////////////////////////////////////
    CSegmenter bsegmenter_bin_true(image_bin_true);
    bsegmenter_bin_true.segmentSimpleRegionGrowing(1, 0, 0, m_Param.rg_threshold / 255.0);
    bsegmenter_bin_true.pruneBySegmentSize(m_Param.min_size);

    validShellCount = 0;
    for (int sid = 0; sid < bsegmenter_erode_true->getNumSegments(); sid++) {
        CSegment* segment = bsegmenter_erode_true->getSegment(sid);
        if (segment->getTag() != 1000)
            continue;

        INT2 centroid;
        if (segment->getCentroid(centroid) != 1)
            continue;

        int bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax;
        segment->getBoundBox(bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax);
        int width = bbox_xmax - bbox_xmin;
        int height = bbox_ymax - bbox_ymin;

        // find the actual core segment from non-erosion binary image
        CSegment* segment_true = bsegmenter_bin_true.getSegment(centroid.x, centroid.y);

        // skip if at least 2 segments are adjacent
        std::vector<int> asids;
        if (segment_true->getAdjacentSegments(asids) >= 2)
            continue;

        int bbox_true_xmin, bbox_true_ymin, bbox_true_xmax, bbox_true_ymax;
        segment_true->getBoundBox(bbox_true_xmin, bbox_true_ymin, bbox_true_xmax, bbox_true_ymax);
        int width_true = bbox_true_xmax - bbox_true_xmin;
        int height_true = bbox_true_ymax - bbox_true_ymin;

        // skip if # pixels is very different from # pixels from the non-erosion one (it must be from background)
        float wr = (float)width_true / (float)width;
        float hr = (float)height_true / (float)height;
        if (wr > 5 || hr > 5) { // difference between original core and eroded core
            //printf("error: (%d, %d)\n", centroid.x, centroid.y);
            continue;
        }

        // skip if the core is at border
        if (bbox_true_xmin < m_Param.min_offset || bbox_true_ymin < m_Param.min_offset ||
            bbox_true_xmax > image_bin_true->getWidth() - m_Param.min_offset ||
            bbox_true_ymax > image_bin_true->getHeight() - m_Param.min_offset)
            continue;

        // routine*: check the adjacent segment to see if this is a shell
        CSegment* asegment_true = bsegmenter_bin_true.getSegment(asids[0]);

        int abbox_true_xmin, abbox_true_ymin, abbox_true_xmax, abbox_true_ymax;
        asegment_true->getBoundBox(abbox_true_xmin, abbox_true_ymin, abbox_true_xmax, abbox_true_ymax);
        int awidth_true = abbox_true_xmax - abbox_true_xmin;
        int aheight_true = abbox_true_ymax - abbox_true_ymin;
        float awr = (float)awidth_true / (float)width_true;
        float ahr = (float)aheight_true / (float)height_true;

        // skip if too big
        if (awr > 10 && ahr > 10) {
            continue;
        }

        // find contour surrounding the core
        std::vector<std::vector<INT2> > shell_contours;
        asegment_true->getContours(shell_contours);
        std::vector<INT2> shell_contour = shell_contours[0]; // take the outer one

        // get extended valid contour
        std::vector<INT2> shell_contour_valid;
        extractValidContour(shell_contour, centroid, shell_contour_valid);

        // check if the centroid is inside the polygon (check if the contour is reasonably constructed around the center)
        bool result = pointInPolygon(centroid, shell_contour_valid);
        if (result)
            validShellCount++;
    }
    printf("valid shell count: %d\n", validShellCount);

    delete bsegmenter[0];
    delete bsegmenter[1];

    return true;
}

bool SEMShape::detectCoreShape()
{
    ///////////////////////////////////////////////////////////////////////////
    // 1. preprocessing
    ///////////////////////////////////////////////////////////////////////////

    // do histogram equalization for low contrast images, with median blurring
    Mat cvimage_histeq, cvimage_temp;
    medianBlur(m_cvImage, cvimage_histeq, 5);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(5);
    clahe->apply(cvimage_histeq, cvimage_temp);
    medianBlur(cvimage_temp, cvimage_histeq, 5);

    getImageObject(cvimage_histeq, m_AdjImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_heq.png", cvimage_histeq);
    computeImageStat(m_AdjImage, m_Stat, 32);

    // threshold image -> binarize to distinguish cores and background
    // use division to separate two
    const int bin_types[2] = {THRESH_BINARY, THRESH_BINARY_INV};
    int bin_threshold = ((m_Param.bin_threshold == -1) ? m_Stat.division*255 : m_Param.bin_threshold);
    Mat cvimage_bin;
    threshold(cvimage_histeq, cvimage_temp, bin_threshold, 255, bin_types[m_Param.bin_inv]);
    //imwrite("/Users/kim63/Desktop/aaa_bin1.png", cvimage_bin);
    medianBlur(cvimage_temp, cvimage_bin, 5);
    getImageObject(cvimage_bin, m_BinImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_bin2.png", cvimage_bin);

    // compute distance transform (apply directly to the original binary image) to find local maximum
    // in case of rod types, the local maximum becomes a line
    Mat cvimage_dist;
    distanceTransform(cvimage_bin, cvimage_dist, DIST_L2, DIST_MASK_PRECISE);
    getImageObject(cvimage_dist, m_DistImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_dist.png", cvimage_dist);

    // perform segmentation on binary image (will be used for extracting the core contour)
    CSegmenter bsegmenter(&m_BinImage);
    bsegmenter.segmentSimpleRegionGrowing(1, 0, 0, m_Param.rg_threshold / 255.0);
    bsegmenter.pruneBySegmentSize(m_Param.min_size);
    //bsegmenter.saveToImageFile("/Users/kim63/Desktop/aaa_bin_seg", 3, true, MAKE_UCHAR3(0, 0, 255));


    ///////////////////////////////////////////////////////////////////////////
    // use extracted initial centers to measure core sizes (s_s, s_l)
    ///////////////////////////////////////////////////////////////////////////
    std::set<int> csids_empty;
    for (size_t n = 0; n < m_ShapeList.size(); n++) {
        INT2 center = m_ShapeList[n].Center;
        center.y = m_BinImage.getHeight() - 1 - center.y;

        CSegment* segment = bsegmenter.getSegment(center.x, center.y);
        int sid = segment->getSid();
        segment->setTag(1000);

        // examine multi-angle lines that intersect core boundary to determine the sizes
        std::vector<INT2> endS(2);
        std::vector<INT2> endL(2);
        int max_radius = m_Image.getHeight() / 2;
        int dS, dL;
        bool ret = measureSize(bsegmenter, center, sid, sid, csids_empty, dS, dL, &endS[0], &endL[0], 5, max_radius, 5, 0.8);
        if (ret) {
            endS[0].y = m_Image.getHeight() - 1 - endS[0].y;
            endS[1].y = m_Image.getHeight() - 1 - endS[1].y;
            endL[0].y = m_Image.getHeight() - 1 - endL[0].y;
            endL[1].y = m_Image.getHeight() - 1 - endL[1].y;

            m_ShapeList[n].Outlier = 0;
            m_ShapeList[n].CoreSizeS = dS;
            m_ShapeList[n].CoreSizeSPoints = endS;
            m_ShapeList[n].CoreSizeL = dL;
            m_ShapeList[n].CoreSizeLPoints = endL;
        }
        else {
            m_ShapeList[n].Outlier = 1;
        }
    }

    return true;
}

bool SEMShape::detectCoreShellShape()
{
    ///////////////////////////////////////////////////////////////////////////
    // 1. preprocessing
    ///////////////////////////////////////////////////////////////////////////

    // do histogram equalization for low contrast images, with median blurring
    Mat cvimage_histeq, cvimage_temp;
    medianBlur(m_cvImage, cvimage_histeq, 5);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(5);
    clahe->apply(cvimage_histeq, cvimage_temp);
    medianBlur(cvimage_temp, cvimage_histeq, 5);

    getImageObject(cvimage_histeq, m_AdjImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_heq.png", cvimage_histeq);
    computeImageStat(m_AdjImage, m_Stat, 32);

    // threshold image -> binarize to distinguish cores and background
    // use division to separate two
    const int bin_types[4] = {THRESH_BINARY | THRESH_OTSU, THRESH_BINARY_INV | THRESH_OTSU, THRESH_BINARY, THRESH_BINARY_INV};
    int bin_threshold = ((m_Param.bin_threshold == -1) ? m_Stat.division*255 : m_Param.bin_threshold);
    int bin_type = ((m_Param.bin_threshold == -1) ? bin_types[m_Param.bin_inv] : bin_types[2+m_Param.bin_inv]);
    Mat cvimage_bin;
    threshold(cvimage_histeq, cvimage_temp, bin_threshold, 255, bin_type);
    //imwrite("/Users/kim63/Desktop/aaa_bin1.png", cvimage_bin);
    medianBlur(cvimage_temp, cvimage_bin, 5);
    getImageObject(cvimage_bin, m_BinImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_bin2.png", cvimage_bin);

    // compute distance transform (apply directly to the original binary image) to find local maximum
    // in case of rod types, the local maximum becomes a line
    Mat cvimage_dist;
    distanceTransform(cvimage_bin, cvimage_dist, DIST_L2, DIST_MASK_PRECISE);
    getImageObject(cvimage_dist, m_DistImage, 1);
    //imwrite("/Users/kim63/Desktop/aaa_dist.png", cvimage_dist);

    // perform segmentation on binary image
    CSegmenter bsegmenter(&m_BinImage);
    bsegmenter.segmentSimpleRegionGrowing(1, 0, 0, m_Param.rg_threshold / 255.0);
    bsegmenter.pruneBySegmentSize(m_Param.min_size);
    //bsegmenter.saveToImageFile("/Users/kim63/Desktop/aaa_bin_seg", 3, true, MAKE_UCHAR3(0, 0, 255));

    // then get size statistics and prune/merge small segments again, based on the size statistics
    std::vector<int> segment_size_list(bsegmenter.getNumSegments());
    for (int sid = 0; sid < bsegmenter.getNumSegments(); sid++) {
        CSegment* segment = bsegmenter.getSegment(sid);
        segment_size_list[sid] = segment->getNumPixels();
    }
    TStatInfo segment_size_stat = computeStat<int>(segment_size_list);
    int min_size = __MIN(m_Param.min_size, int(segment_size_stat.median / 4.0));
    bsegmenter.pruneBySegmentSize(min_size);
    //m_Param.min_size = __MAX(m_Param.min_size, int(segment_size_stat.median / 4.0));
    //bsegmenter.pruneBySegmentSize(m_Param.min_size);
    //bsegmenter.saveToImageFile("/Users/kim63/Desktop/0_segmentation3", 3, true, MAKE_UCHAR3(0, 0, 255));


    ///////////////////////////////////////////////////////////////////////////
    // 2. extract initial centers
    ///////////////////////////////////////////////////////////////////////////
    // loop over to extract isloated segments
    std::vector<INT2> center_list;
    std::vector<std::vector<INT2> > contour_list;
    std::vector<std::vector<INT2> > contour_fit_list;
    std::vector<RotatedRect> ellipse_list;
    std::vector<float> dist_list;
    std::vector<int> size_list;
    std::vector<INT2> center_sid_list; // csid, ssid
    for (int sid = 0; sid < bsegmenter.getNumSegments(); sid++) {
        CSegment* segment = bsegmenter.getSegment(sid);
        int xmin, ymin, xmax, ymax;
        segment->getBoundBox(xmin, ymin, xmax, ymax);
        if (xmin < m_Param.min_offset || xmax > m_Image.getWidth()  - m_Param.min_offset ||
            ymin < m_Param.min_offset || ymax > m_Image.getHeight() - m_Param.min_offset)
            continue;

        // make sure that this segment is isoloated by extracting adjacent segments
        // skip if equal to or more than 2 segments are adjacent
        std::vector<int> asids;
        int ascount;
        ascount = segment->getAdjacentSegments(asids);
        if (ascount >= 2)
            continue;

        // get centroid, skip if centroid is outside the segment
        INT2 centroid;
        if (segment->getCentroid(centroid) != 1)
            continue;
        int segsize = segment->getNumPixels();

        // extract contour
        std::vector<std::vector<INT2> > contours;
        if (segment->getContours(contours) != 1)
            continue;

        // smooth contours
        std::vector<INT2> contour = contours[0];
        std::vector<INT2> out_contour;
        smoothContour(contour, 2, out_contour);
        smoothContour(out_contour, 2, contour);

        // fit to ellipse
        std::vector<Point> segment_contour;
        for (int cp = 0; cp < (int)contour.size(); cp++)
            segment_contour.push_back(Point(contour[cp].x, m_Image.getHeight() - 1 - contour[cp].y));
        RotatedRect segment_ellipse = fitEllipse(Mat(segment_contour));
        //RotatedRect segment_rect = minAreaRect(Mat(segment_contour));
        //rectangle_list.push_back(segment_rect);

        // get ellipse info
        FLOAT2 ellipse_center = MAKE_FLOAT2(segment_ellipse.center.x, m_Image.getHeight() - 1 - segment_ellipse.center.y);
        float ellipse_radius1 = segment_ellipse.size.width / 2.0;
        float ellipse_radius2 = segment_ellipse.size.height / 2.0;
        float ellipse_angle = 180 - segment_ellipse.angle;
        //float ellipse_area = __PI * ellipse_radius1 * ellipse_radius2;
        //printf("center: %f %f, radius: %f, %f, angle: %f\n", ellipse_center.x, ellipse_center.y, ellipse_radius1, ellipse_radius2, ellipse_angle);

        // compute average distance from contour to ellipse
        std::vector<INT2> fitpixels;
        float fit_dist = distanceContourToEllipse(ellipse_radius1, ellipse_radius2, \
                                                  ellipse_center.x, ellipse_center.y, \
                                                  ellipse_angle, contour, fitpixels);

        // add all info to list
        contour_list.push_back(contour);
        contour_fit_list.push_back(fitpixels);
        ellipse_list.push_back(segment_ellipse);
        dist_list.push_back(fit_dist);
        size_list.push_back(segsize);
        center_list.push_back(MAKE_INT2(centroid.x, centroid.y));
        center_sid_list.push_back(MAKE_INT2(sid, asids[0]));
    }
    if (center_list.size() == 0)
        return false;


    ///////////////////////////////////////////////////////////////////////////
    // finalize centers by filtering based on sizes and fit-distances
    ///////////////////////////////////////////////////////////////////////////

    // threshold using average distance to ellipse
    TStatInfo dist_stat = computeStat<float>(dist_list);
    std::vector<float> dist_list2 = dist_list;
    std::sort(dist_list2.begin(), dist_list2.end());
    //printf("dist stat: %f %f %f %f\n", dist_stat.mean, dist_stat.stdev, dist_stat.median, dist_stat.percentile75);
    float dist_threshold = ceil(dist_stat.percentile75);
    //printf("dist threshold: %f\n", dist_threshold);

    std::vector<int> size_list2 = size_list;
    std::sort(size_list2.begin(), size_list2.end());
    int percentile1 = (int)(size_list2.size() * 0.2);
    int percentile2 = (int)(size_list2.size() * 0.99);
    float size_threshold1 = size_list2[percentile1];
    float size_threshold2 = size_list2[percentile2];

    // finalize shape centers and measure core sizes
    m_ShapeList.clear();
    std::vector<INT2> center_list_new;
    std::vector<INT2> center_sid_list_new;
    std::set<int> csids_empty;
    int max_radius = m_Image.getHeight() / 2;
    for (size_t n = 0; n < center_list.size(); n++) {
        if (dist_list[n] > dist_threshold)
            continue;
        if (size_list[n] < size_threshold1 || size_list[n] > size_threshold2)
            continue;

        INT2 center = center_list[n];
        INT2 sid = center_sid_list[n];

        // set up a new shape info
        TShapeInfo info;
        info.Center.x = center.x;
        info.Center.y = m_Image.getHeight() - 1 - center.y;

        // measure core size
        std::vector<INT2> endS(2);
        std::vector<INT2> endL(2);
        int dS, dL;
        bool ret = measureSize(bsegmenter, center, sid.x, sid.x, csids_empty, dS, dL, &endS[0], &endL[0], 5, max_radius, 5, 0.8);
        if (ret) {
            endS[0].y = m_Image.getHeight() - 1 - endS[0].y;
            endS[1].y = m_Image.getHeight() - 1 - endS[1].y;
            endL[0].y = m_Image.getHeight() - 1 - endL[0].y;
            endL[1].y = m_Image.getHeight() - 1 - endL[1].y;

            info.Outlier = 0;
            info.CoreSizeS = dS;
            info.CoreSizeSPoints = endS;
            info.CoreSizeL = dL;
            info.CoreSizeLPoints = endL;
        }
        else {
            info.Outlier = 1;
        }

        // add to list
        center_list_new.push_back(center);
        center_sid_list_new.push_back(sid);
        m_ShapeList.push_back(info);
    }

/*
    // measure shell sizes -> contains some flaws, especially when the core-shells are heavily cluttered
    for (size_t n = 0; n < center_list_new.size(); n++) {
        INT2 center = center_list_new[n];
        INT2 sid = center_sid_list_new[n];
        TShapeInfo* info = &m_ShapeList[n];
        if (info->Outlier != 0)
            continue;

        // get other cores inside the shell
        CBaseSegment* segment = bsegmenter.getSegment(sid.y);
        std::vector<int> asids;
        segment->getAdjacentBaseSegments(asids);

        std::set<int> csids;
        for (size_t a = 0; a < asids.size(); a++) {
            if (asids[a] == sid.x)
                continue;

            CBaseSegment* asegment = bsegmenter.getSegment(asids[a]);
            std::vector<int> asids2;
            if (asegment->getAdjacentBaseSegments(asids2) != 1)
                continue;
            csids.insert(asids[a]);
        }

        std::vector<INT2> endS(2);
        std::vector<INT2> endL(2);
        int dS, dL;
        bool ret = measureSize(bsegmenter2, center, sid.x, sid.y, csids, dS, dL, &endS[0], &endL[0], 5, max_radius, 5, 0.8);
        //bool ret = measureSize(bsegmenter, center, sid.x, sid.y, csids, dS, dL, &endS[0], &endL[0], 5, max_radius, 5, 0.8);
        if (ret) {
            endS[0].y = m_Image.getHeight() - 1 - endS[0].y;
            endS[1].y = m_Image.getHeight() - 1 - endS[1].y;
            endL[0].y = m_Image.getHeight() - 1 - endL[0].y;
            endL[1].y = m_Image.getHeight() - 1 - endL[1].y;

            info->Outlier = 0;
            info->ShellSizeS = dS;
            info->ShellSizeSPoints = endS;
            info->ShellSizeL = dL;
            info->ShellSizeLPoints = endL;
        }
        else {
            info->Outlier = 1;
        }
    }
*/

    // binary image and markers for watershed
    Mat cvimage_markers(cvimage_histeq.size(), CV_32S);
    cvimage_markers = Scalar::all(1); // unknown is 0, background is 1
    Mat cvimage_bin2 = cvimage_bin.clone();

    int sure_fid = 2;
    for (size_t n = 0; n < center_list_new.size(); n++) {
        INT2 sid = center_sid_list_new[n];
        CSegment* core_segment = bsegmenter.getSegment(sid.x);
        CSegment* shell_segment = bsegmenter.getSegment(sid.y);

        std::vector<INT2> core_pixels;
        core_segment->getPixels(core_pixels);
        for (size_t m = 0; m < core_pixels.size(); m++) {
            int x = core_pixels[m].x;
            int y = m_Image.getHeight() - 1 - core_pixels[m].y;
            cvimage_markers.at<int>(y, x) = sure_fid;
            cvimage_bin2.at<uchar>(y, x) = 0;
        }
        sure_fid++;

        std::vector<INT2> shell_pixels;
        shell_segment->getPixels(shell_pixels);
        for (size_t m = 0; m < shell_pixels.size(); m++) {
            int x = shell_pixels[m].x;
            int y = m_Image.getHeight() - 1 - shell_pixels[m].y;
            cvimage_markers.at<int>(y, x) = 0; // unknown
        }
    }
    //imwrite("/Users/kim63/Desktop/aaa_markers.png", cvimage_markers);

    // perform watershed segmentation
    //cvtColor(cvimage_histeq, cvimage_temp, COLOR_GRAY2BGR);
    cvtColor(cvimage_bin2, cvimage_temp, COLOR_GRAY2BGR);
    watershed(cvimage_temp, cvimage_markers);
    //imwrite("/Users/kim63/Desktop/aaa_bin2.png", cvimage_temp);
    //imwrite("/Users/kim63/Desktop/aaa_watershed.png", cvimage_markers);

    for (size_t n = 0; n < center_list_new.size(); n++) {
        //INT2 sid = center_sid_list_new[n];
        INT2 center = center_list_new[n];
        TShapeInfo* info = &m_ShapeList[n];
        if (info->Outlier != 0)
            continue;

        std::vector<INT2> endS(2);
        std::vector<INT2> endL(2);
        int dS, dL;
        bool ret = measureSizeCV(cvimage_markers, center, dS, dL, &endS[0], &endL[0], 5, max_radius, 5, 0.8);
        if (ret) {
            endS[0].y = m_Image.getHeight() - 1 - endS[0].y;
            endS[1].y = m_Image.getHeight() - 1 - endS[1].y;
            endL[0].y = m_Image.getHeight() - 1 - endL[0].y;
            endL[1].y = m_Image.getHeight() - 1 - endL[1].y;

            info->Outlier = 0;
            info->ShellSizeS = dS;
            info->ShellSizeSPoints = endS;
            info->ShellSizeL = dL;
            info->ShellSizeLPoints = endL;
        }
        else {
            info->Outlier = 1;
        }
    }

    return true;
}

int SEMShape::selectByBox(int xmin, int ymin, int xmax, int ymax)
{
    int ret = 0;
    for (int n = 0; n < (int)m_ShapeList.size(); n++) {
        TShapeInfo* sinfo = &m_ShapeList[n];
        int x = sinfo->Center.x;
        int y = sinfo->Center.y;
        if (x >= xmin && y >= ymin && x <= xmax && y <= ymax) {
            sinfo->Selected = 1;
            ret++;
        }
    }
    return ret;
}

int SEMShape::selectByRange(int shapeMode, int sizeMode, int rangeMin, int rangeMax)
{
    int ret = 0;
    for (int n = 0; n < (int)m_ShapeList.size(); n++) {
        TShapeInfo* sinfo = &m_ShapeList[n];

        if (shapeMode == 0) {
            if (sizeMode == 0) {
                if (sinfo->CoreSizeS >= rangeMin && sinfo->CoreSizeS <= rangeMax) {
                    sinfo->Selected = 1;
                    ret++;
                }
            }
            else if (sizeMode == 1) {
                if (sinfo->CoreSizeL >= rangeMin && sinfo->CoreSizeL <= rangeMax) {
                    sinfo->Selected = 1;
                    ret++;
                }
            }
            else {
                if ((sinfo->CoreSizeS >= rangeMin && sinfo->CoreSizeS <= rangeMax) ||
                    (sinfo->CoreSizeL >= rangeMin && sinfo->CoreSizeL <= rangeMax)) {
                    sinfo->Selected = 1;
                    ret++;
                    printf("here: %d\n", n);
                }
            }
        }
        else {
            if (sizeMode == 0) {
                if (sinfo->ShellSizeS >= rangeMin && sinfo->ShellSizeS <= rangeMax) {
                    sinfo->Selected = 1;
                    ret++;
                }
            }
            else if (sizeMode == 1) {
                if (sinfo->ShellSizeL >= rangeMin && sinfo->ShellSizeL <= rangeMax) {
                    sinfo->Selected = 1;
                    ret++;
                }
            }
            else {
                if ((sinfo->ShellSizeS >= rangeMin && sinfo->ShellSizeS <= rangeMax) ||
                    (sinfo->ShellSizeL >= rangeMin && sinfo->ShellSizeL <= rangeMax)) {
                    sinfo->Selected = 1;
                    ret++;
                }
            }
        }
    }
    return ret;
}

void SEMShape::selectByInd(int ind)
{
    if (ind < 0 || ind >= (int)m_ShapeList.size())
        return;
    TShapeInfo* sinfo = &m_ShapeList[ind];
    sinfo->Selected = 1;
}

void SEMShape::clearSelected()
{
    for (int n = 0; n < (int)m_ShapeList.size(); n++) {
        TShapeInfo* sinfo = &m_ShapeList[n];
        sinfo->Selected = 0;
    }
}

void SEMShape::removeSelected()
{
    std::vector<TShapeInfo> temp_shapelist = m_ShapeList;
    m_ShapeList.clear();
    for (int n = 0; n < (int)temp_shapelist.size(); n++) {
        TShapeInfo* sinfo = &temp_shapelist[n];
        if (sinfo->Selected == 0)
            m_ShapeList.push_back(*sinfo);
    }
}

