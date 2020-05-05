//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// SEM image utility functions (.h, .cpp)
//*****************************************************************************/

#ifndef __SEMUTIL_H
#define __SEMUTIL_H

#include "datatype.h"
#include "segmenter.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//using namespace std; -> conflict with other header files
using namespace cv;



struct TStatInfo
{
    float mean;
    float stdev;
    float median;
    float percentile25;
    float percentile75;
    float division; // usually global minimum that separates the peaks of fore- and back-ground
};


// utility functions
bool getImageObject(Mat& cvimage, CImage& image, int num_channels);
bool computeImageStat(CImage& image, TStatInfo& stat_info, int hist_bin_size=256);
float computeFeatureDist(float* feat1, float* feat2, int feat_size);
double safe_acos(double x);

bool measureSize(CSegmenter& bsegmenter, INT2 center, int csid, int ssid, std::set<int>& csids, int& dS, int &dL, \
                 INT2 endS[2], INT2 endL[2], int angle_interval=5, int max_radius=500, int delta_p=5, float delta_r=0.8);
bool measureSizeCV(Mat& cvimage, INT2 center, int& dS, int &dL, INT2 endS[2], INT2 endL[2], \
                   int angle_interval=5, int max_radius=500, int delta_p=5, float delta_r=0.8);

uchar getCVImagePixel1(void* Image, bool Flip, int px, int py);
UCHAR3 getCVImagePixel3(void* Image, bool Flip, int px, int py);
void setCVImagePixel1(void* Image, bool Flip, int px, int py, int value);

bool pointInPolygon(INT2 pt, std::vector<INT2> polygon);
bool smoothContour(std::vector<INT2> contour, int window_size, std::vector<INT2>& out_contour);
bool extractValidContour(std::vector<INT2> contour, INT2 center, std::vector<INT2>& out_contour, int angle_threshold=85, int angle_threshold2=5);
float computeEllipse(float r1, float r2, float cx, float cy, float angle, float x, float y);
int getEllipsePixels(float r1, float r2, float cx, float cy, float angle, int xmin, int ymin, int xmax, int ymax, std::vector<INT2>& ellipse_pixels);
FLOAT2 getNearestEllipse(float r1, float r2, float cx, float cy, float angle, float x, float y);
FLOAT2 rotatePoint(FLOAT2 cpoint, float angle, FLOAT2 point);
float pointToLineDistance(FLOAT2 p, FLOAT2 a, FLOAT2 b);
float getLineAngle(INT2 a, INT2 p, INT2 b);
float check_overlap(INT4& bbox1, INT4& bbox2);
float distanceContourToRectangle(INT2 rect_pts[4], std::vector<INT2>& contour_pixels);
float distanceContourToEllipse(float r1, float r2, float cx, float cy, float angle,
                               std::vector<INT2>& contour_pixels, std::vector<INT2>& fit_pixels);
void getCentroid(std::vector<INT2>& pixels, INT2& centroid);
void getLinePixels(int x1, int y1, int x2, int y2, std::vector<INT2>& pixels); // use line drawing algorithm

void drawOutImage(Mat& cvimage, bool use_image, std::string filepath, bool flip,
                  bool draw_point, std::vector<INT2>* point_list, INT3 point_color, int point_size,
                  bool draw_contour, std::vector<std::vector<INT2> >* contour_list, INT3 contour_color,
                  bool draw_rectangle, std::vector<RotatedRect>* rect_list, INT3 rect_color,
                  bool draw_ellipse, std::vector<RotatedRect>* ellipse_list, INT3 ellipse_color,
                  bool use_rect_ellipse_type, std::vector<int>* rect_ellipse_type_list);
void drawOutImage2(Mat& cvimage, std::string filepath, bool flip,
                   bool draw_point, std::vector<INT2>* point_list, INT3 point_color, int point_size,
                   bool draw_contour, std::vector<std::vector<INT2> >* contour_list, INT3 contour_color,
                   bool draw_shortest, std::vector<INT2>* shortest_pt_list, INT3 shortest_color,
                   bool draw_longest, std::vector<INT2>* longest_pt_list, INT3 longest_color);

template <class my_type>
TStatInfo computeStat(std::vector<my_type>& input_list)
{
    std::vector<my_type> input_list2 = input_list;
    std::sort(input_list2.begin(), input_list2.end());

    TStatInfo stat_result;

    int ind1 = (int)(input_list2.size() / 4.0);
    int ind2 = (int)(input_list2.size() / 2.0);
    int ind3 = (int)(input_list2.size() * 3 / 4.0);
    stat_result.percentile25 = input_list2[ind1];
    stat_result.median       = input_list2[ind2];
    stat_result.percentile75 = input_list2[ind3];

    stat_result.mean = 0;
    for (size_t n = 0; n < input_list2.size(); n++) {
        stat_result.mean += input_list2[n];
    }
    stat_result.mean /= input_list2.size();

    stat_result.stdev = 0;
    for (size_t n = 0; n < input_list2.size(); n++) {
        float diff = stat_result.mean - input_list2[n];
        stat_result.stdev += (diff * diff);
    }
    stat_result.stdev /= input_list2.size();
    stat_result.stdev = sqrt(stat_result.stdev);
    stat_result.division = 0;

    return stat_result;
}



#endif
