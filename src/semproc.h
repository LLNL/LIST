//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// SEM image process classes (.h, .cpp)
//*****************************************************************************/

#ifndef __SEMPROC_H
#define __SEMPROC_H

#include "semutil.h"
#include "textdetect.h"

// check out this website: https://github.com/tesseract-ocr/tesseract/wiki/Compiling
// reference: https://github.com/tesseract-ocr/tesseract/wiki/APIExample
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>



struct TScalebarSegmenter_Param
{
    TScalebarSegmenter_Param()
    {
        threshold = 0.2;
        completeness = 0.8;
        min_thickness = 2;
        max_thickness = 10;
        min_length = 10;
        max_length = 150;

        // in case of around 600 x 400
        base_width = 570;
    }
    int getBaseMinThickness() { return 2; }
    int getBaseMaxThickness() { return 10; }
    int getBaseMinLength() { return 10; }
    int getBaseMaxLength() { return 150; }
    int getBaseMinARatio() { return 5; }
    int getBaseMaxARatio() { return 50; }

    float   threshold;
    float   completeness;
    int     min_thickness;
    int     max_thickness;
    int     min_length;
    int     max_length;
    int     base_width;
};


struct TShapeSegmenter_Param
{
    TShapeSegmenter_Param() // determined by experiment
    {
        bin_inv = 0;
        bin_threshold = -1;
        rg_threshold = 50;  // 5/255.0, 50 / 255.0
        min_offset = -1;  // -1 , 80
        min_size = 10;
        shape_hist_size = 20;
    }
    int bin_inv;        // for binarization (thresholding), whether invert (1) ot not (0)
    int bin_threshold; // for binarization (thresholding), -1: not used (use image statistics)
    int rg_threshold;  // for core segmentation
    int min_offset;    // for collecting centers (ignore outer segments), -1: use image size * 0.03
    int min_size;      // for core segmentation (pruning small segments)
    int shape_hist_size; // not used
};


struct TShapeInfo
{
    TShapeInfo()
    {
        CoreType = 0;
        CoreSizeS = 0;
        CoreSizeL = 0;
        ShellType = 0;
        ShellSizeS = 0;
        ShellSizeL = 0;
        Selected = 0;
        Outlier = 0;
    }

    INT2 Center;        // center point (x, y)
    int CoreType;       // 0: unknown, 1: rectangle, 2: ellipse
    int CoreSizeS;      // shortest distance from center to core boundary * 2 (dS)
    int CoreSizeL;      // longest distance from center to core boundary * 2 (dL)
    int ShellType;      // 0: unknown, 1: rectangle, 2: ellipse
    int ShellSizeS;     // shortest distance from center to shell boundary * 2 (dS)
    int ShellSizeL;     // longest distance from center to shell boundary * 2 (dL)
    int Selected;       // used for select mode, 0: unselected, 1: selected
    int Outlier;        // used for checking outlier, 0: inlier, 1: outlier

    std::vector<INT2> CoreSizeSPoints;
    std::vector<INT2> CoreSizeLPoints;
    std::vector<INT2> ShellSizeSPoints;
    std::vector<INT2> ShellSizeLPoints;

    //std::vector<INT2> CoreContour; // core contour
    //std::vector<INT2> ShellContour; // shell contour

};


class SEMScaleBar
{
public:
    SEMScaleBar();
    ~SEMScaleBar();

    bool initTextDetector(const char* model_path);
    bool initTextRecognizer(const char* data_path);
    bool openImage(const char* fileName);
    bool detectScaleBar();
    bool detectScaleText();
    void manualSelect(int xmin, int ymin, int xmax, int ymax, int number, int unit);
    bool getDetectedScale(int& scale_length, int& scale_number, int& scale_unit);

    static float convert(int length, int scale_length, int scale_number, int scale_unit);
    static int inverse(float length, int scale_length, int scale_number, int scale_unit);

    CImage* getImage() { return &m_Image; }
    std::vector<INT4>* getScaleBarList() { return &m_ScaleBarList; }
    std::vector<INT3>* getScaleInfoList() { return &m_ScaleInfoList; }
    TScalebarSegmenter_Param* getParam() { return &m_Param; }


protected:
    bool parseScaleNumber(std::string text, int& number, int& unit);

protected:
    CImage              m_Image;            // RGB
    Mat                 m_cvImage;          // grayscale
    std::vector<INT4>   m_ScaleBarList;     // candidate scalebar segmentation list  (box: xmin, ymin, xmax, ymax)
    std::vector<INT3>   m_ScaleInfoList;    // detected (final) scale info (index to m_CandScaleBarList, number, unit)
    TextDetector        m_TextDetector;     // EAST text detector
    tesseract::TessBaseAPI* m_Tesseract;    // text recognizer

    TScalebarSegmenter_Param    m_Param;

};



class SEMShape
{
public:
    SEMShape();
    ~SEMShape();

    bool openImage(const char* fileName);
    bool detectShape(bool autoDetect, int shapeType=0);

    int selectByBox(int xmin, int ymin, int xmax, int ymax);
    int selectByRange(int shapeMode, int sizeMode, int rangeMin, int rangeMax);
    void selectByInd(int ind);
    void clearSelected();
    void removeSelected();

    CImage* getImage() { return &m_Image; }
    CImage* getAdjImage() { return &m_AdjImage; }
    CImage* getBinImage() { return &m_BinImage; }
    CImage* getDstImage() { return &m_DistImage; }
    CImage* getOutImage() { return &m_OutImage; }

    int getShapeType()                      { return m_ShapeType; }
    TShapeSegmenter_Param* getParam()       { return &m_Param; }
    TStatInfo* getStatInfo()                { return &m_Stat; }
    std::vector<TShapeInfo>* getShapeList() { return &m_ShapeList; }

protected:
    bool detectGeneralCenters(int& invRequired, int& validCount, int& validShellCount);
    bool detectCoreShape();
    bool detectCoreShellShape();

protected:
    CImage      m_Image; // grayscale image
    Mat         m_cvImage; // grayscale cv image
    CImage      m_AdjImage; // grayscale histogram adjusted image
    CImage      m_BinImage;
    CImage      m_DistImage;
    CImage      m_OutImage;

    int                     m_ShapeType;
    TShapeSegmenter_Param   m_Param;
    TStatInfo               m_Stat;
    std::vector<TShapeInfo> m_ShapeList;

};



#endif
