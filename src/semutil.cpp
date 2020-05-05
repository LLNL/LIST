//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// SEM image utility functions (.h, .cpp)
//*****************************************************************************/

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

#include "persistence1d.hpp"



const std::string g_ShapeType[3] = {"unknown", "ellipse", "rectangle"};



///////////////////////////////////////////////////////////////////////////////
// common utility functions
///////////////////////////////////////////////////////////////////////////////

bool getImageObject(Mat& cvimage_input, CImage& image, int num_channels)
{
    Mat cvimage = cvimage_input.clone();

    // flip image
    flip(cvimage_input, cvimage, 0);
    //IplImage cvimage_ipl = cvimage;
    //cvFlip(&cvimage_ipl);

    // normalize image
    Mat cvnimage = Mat(cvimage.rows, cvimage.cols, CV_32F);
    cvimage.convertTo(cvnimage, CV_32F, 1.0/255.0, 0);

    // initialize image object
    image.init(cvimage.cols, cvimage.rows, 1, num_channels, (float*)cvnimage.data, true);

    return true;
}

bool computeImageStat(CImage& image, TStatInfo& stat_info, int hist_bin_size)
{
    int proportion = 256 / hist_bin_size;

    // get histogram and dynamic range
    std::vector<int> histogram(hist_bin_size);
    image.getHistogram(hist_bin_size, &histogram[0]);
    std::vector<float> hist_data(histogram.begin(), histogram.end());

    stat_info.mean = 0;
    for (int n = 0; n < image.getNumPixels(); n++) {
        stat_info.mean += *(image.getPixel(n));
    }
    stat_info.mean /= image.getNumPixels();

    stat_info.stdev = 0;
    for (int n = 0; n < image.getNumPixels(); n++) {
        float diff = stat_info.mean - *(image.getPixel(n));
        stat_info.stdev += diff * diff;
    }
    stat_info.stdev = sqrt(stat_info.stdev / image.getNumPixels());

    stat_info.division = 0;
    stat_info.median = 0;
    stat_info.percentile25 = 0;
    stat_info.percentile75 = 0;

    // find global minimum to separate two peaks
    p1d::Persistence1D p;
    p.RunPersistence(hist_data);

    std::vector<int> minima;
    std::vector<int> maxima;
    p.GetExtremaIndices(minima, maxima, 0);
    if (minima.size() <= 0 || maxima.size() <= 1)
        return true;

    std::sort(minima.begin(), minima.end());
    std::sort(maxima.begin(), maxima.end());

    // find maxima from darkness
    int max_ind_low = -1;
    int max_count_low = 0;
    for (size_t n = 0; n < maxima.size(); n++) {
        if (maxima[n] * proportion > 128)
            break;

        if (max_count_low < histogram[maxima[n]]) {
            max_count_low = histogram[maxima[n]];
            max_ind_low = maxima[n];
        }
    }

    // find maxima from brightness
    int max_ind_high = -1;
    int max_count_high = 0;
    for (size_t n = maxima.size()-1; n >= 0; n--) {
        if (maxima[n] * proportion < 128)
            break;
        if (max_count_high < histogram[maxima[n]]) {
            max_count_high = histogram[maxima[n]];
            max_ind_high = maxima[n];
        }
    }
    //for (size_t n = 0; n < maxima.size(); n++) {
    //    printf("maxima: %d (count: %d)\n", maxima[n] * proportion, histogram[maxima[n]]); // temp
    //}
    //printf("maxima_low, maxima_high: %d %d \n", max_ind_low * proportion, max_ind_high * proportion); // temp

    // find global minimum
    if (max_ind_low >= 0 && max_ind_high >= 0) {
        int min_ind = -1;
        int min_count = 100000000;
        for (size_t n = 0; n < minima.size(); n++) {
            //printf("minima: %d (count: %d)\n", minima[n] * proportion, histogram[minima[n]]); // temp
            if (minima[n] > max_ind_low && minima[n] < max_ind_high) {
                if (min_count > histogram[minima[n]]) {
                    min_count = histogram[minima[n]];
                    min_ind = minima[n];
                }
            }
        }

        if (min_count < max_count_low * 0.7 && min_count < max_count_high * 0.7) {
            stat_info.division = (min_ind * proportion) / 255.0;
        }
        else {
            stat_info.division = stat_info.mean;
        }
    }

    return true;
}

float computeFeatureDist(float* feat1, float* feat2, int feat_size)
{
    float dist = 0;
    for (int i = 0; i < feat_size; i++) {
        float diff = feat1[i] - feat2[i];
        float sum  = feat1[i] + feat2[i];
        if (fabs(sum) > 1e-4)
            dist += (pow(diff, 2) / sum);
    }
    return dist;
}


double safe_acos(double x)
{
    if (x < -1.0)
        x = -1.0;
    else if (x > 1.0)
        x = 1.0;
  return acos (x);
}

// measure core size if csid = ssid or ssid = -1
bool measureSize(CSegmenter& bsegmenter, INT2 center, int csid, int ssid, std::set<int>& csids, int& dS, int &dL, \
                 INT2 endS[2], INT2 endL[2], int angle_interval, int max_radius, int delta_p, float delta_r)
{
    dS = max_radius*2;
    dL = 0;
    for (int deg = 0; deg < 180; deg+=angle_interval) {
        float rad = __DEG2RAD(deg);
        float x1 = center.x + max_radius * cos(rad);
        float y1 = center.y + max_radius * sin(rad);
        float x2 = center.x - max_radius * cos(rad);
        float y2 = center.y - max_radius * sin(rad);
        //printf("center: %d, %d,  %f %f %f %f\n", center.x, center.y, x1, y1, x2, y2);

        std::vector<INT2> line1, line2;
        getLinePixels(center.x, center.y, x1, y1, line1);
        getLinePixels(center.x, center.y, x2, y2, line2);

        bool valid = true;
        int p1, p2;
        for (p1 = 1; p1 < (int)line1.size();) {
            int sid = bsegmenter.getSegmentId(line1[p1].x, line1[p1].y);
            if (sid != csid && sid != ssid) {
                if (csids.size() > 0 && csids.find(sid) != csids.end()) { // if sid is found in the set
                    valid = false;
                }
                break;
            }
            p1++;
        }
        if (!valid)
            continue;

        for (p2 = 1; p2 < (int)line2.size();) {
            int sid = bsegmenter.getSegmentId(line2[p2].x, line2[p2].y);
            if (sid != csid && sid != ssid) {
                if (csids.size() > 0 && csids.find(sid) != csids.end()) { // if sid is found in the set
                    valid = false;
                }
                break;
            }
            p2++;
        }
        if (!valid)
            continue;

        float r1 = __LENGTH_2D(center.x, center.y, line1[p1].x, line1[p1].y);
        float r2 = __LENGTH_2D(center.x, center.y, line2[p2].x, line2[p2].y);
        //if (fabs(r1-r2) >= delta_p) {
        //    continue;
        //}
        if ((r1 / r2) < delta_r || (r2 / r1) < delta_r)
            continue;

        if (dS > (r1+r2)) {
            dS = r1+r2;
            endS[0] = line1[p1];
            endS[1] = line2[p2];
        }
        if (dL < (r1+r2)) {
            dL = r1+r2;
            endL[0] = line1[p1];
            endL[1] = line2[p2];
        }
    }

    if (dS != max_radius*2 && dL != 0)
        return true;
    else
        return false;
}

// cvimage contains watershed segmentation labels. label=-1 is boundary
bool measureSizeCV(Mat& cvimage, INT2 center, int& dS, int &dL, INT2 endS[2], INT2 endL[2], \
                   int angle_interval, int max_radius, int delta_p, float delta_r)
{
    int sid = cvimage.at<int>(cvimage.rows-1-center.y, center.x);

    dS = max_radius*2;
    dL = 0;
    for (int deg = 0; deg < 180; deg+=angle_interval) {
        float rad = __DEG2RAD(deg);
        float x1 = center.x + max_radius * cos(rad);
        float y1 = center.y + max_radius * sin(rad);
        float x2 = center.x - max_radius * cos(rad);
        float y2 = center.y - max_radius * sin(rad);
        //printf("center: %d, %d,  %f %f %f %f\n", center.x, center.y, x1, y1, x2, y2);

        std::vector<INT2> line1, line2;
        getLinePixels(center.x, center.y, x1, y1, line1);
        getLinePixels(center.x, center.y, x2, y2, line2);

        bool valid = true;
        int p1, p2;
        for (p1 = 1; p1 < (int)line1.size();) {
            int sid2 = cvimage.at<int>(cvimage.rows-1-line1[p1].y, line1[p1].x);
            if (sid2 == -1) { // if boundary
                printf("boundary\n");
                break;
            }
            if (sid2 != sid) { // cross without boundary
                //printf("crossed:%d, %d\n", sid, sid2);
                valid = false;
                break;
            }
            p1++;
        }
        if (!valid)
            continue;

        for (p2 = 1; p2 < (int)line2.size();) {
            int sid2 = cvimage.at<int>(cvimage.rows-1-line2[p2].y, line2[p2].x);
            if (sid2 == -1) { // if boundary
                break;
            }
            if (sid2 != sid) { // cross without boundary
                valid = false;
                break;
            }
            p2++;
        }
        if (!valid)
            continue;

        float r1 = __LENGTH_2D(center.x, center.y, line1[p1].x, line1[p1].y);
        float r2 = __LENGTH_2D(center.x, center.y, line2[p2].x, line2[p2].y);
        //if (fabs(r1-r2) >= delta_p) {
        //    continue;
        //}
        if ((r1 / r2) < delta_r || (r2 / r1) < delta_r)
            continue;

        if (dS > (r1+r2)) {
            dS = r1+r2;
            endS[0] = line1[p1];
            endS[1] = line2[p2];
        }
        if (dL < (r1+r2)) {
            dL = r1+r2;
            endL[0] = line1[p1];
            endL[1] = line2[p2];
        }
    }

    if (dS != max_radius*2 && dL != 0)
        return true;
    else
        return false;
}


uchar getCVImagePixel1(void* Image, bool Flip, int px, int py)
{
    Mat* img = (Mat*)Image;
    double x = px;
    double y = ((Flip) ? img->rows - 1 - py : py);

    if (x < 0 || x >= img->cols || y < 0 || y >= img->rows)
        return 0;

    uchar pixel = img->at<uchar>(y, x);
    return pixel;
}

UCHAR3 getCVImagePixel3(void* Image, bool Flip, int px, int py)
{
    Mat* img = (Mat*)Image;
    double x = px;
    double y = ((Flip) ? img->rows - 1 - py : py);

    if (x < 0 || x >= img->cols || y < 0 || y >= img->rows) {
        UCHAR3 color = MAKE_UCHAR3(0, 0, 0);
        return color;
    }

    Vec3b pixel = img->at<Vec3b>(y, x);
    UCHAR3 color = MAKE_UCHAR3(pixel[0], pixel[1], pixel[2]);
    return color;
}

void setCVImagePixel1(void* Image, bool Flip, int px, int py, int value)
{
    Mat* img = (Mat*)Image;
    double x = px;
    double y = ((Flip) ? img->rows - 1 - py : py);

    if (x < 0 || x >= img->cols || y < 0 || y >= img->rows)
        return;

    img->at<uchar>(y, x) = (uchar)value;
}

bool pointInPolygon(INT2 pt, std::vector<INT2> polygon)
{
    bool result = false;
    size_t j = polygon.size() - 1;
    for (size_t i = 0; i < polygon.size(); i++) {
        if ((polygon[j].y <= pt.y && pt.y < polygon[i].y && __ORIENTATION(polygon[i].x, polygon[i].y, polygon[j].x, polygon[j].y, pt.x, pt.y) > 0.0) ||
            (polygon[i].y <= pt.y && pt.y < polygon[j].y && __ORIENTATION(polygon[j].x, polygon[j].y, polygon[i].x, polygon[i].y, pt.x, pt.y) > 0.0)) {
            result = !result;
        }
        j = i;
    }

    return result;
}

bool smoothContour(std::vector<INT2> contour, int window_size, std::vector<INT2>& out_contour)
{
    out_contour.resize(contour.size());
    for (int cp = 0; cp < (int)contour.size(); cp++) {
        float sumx = 0;
        float sumy = 0;
        for (int cpp = cp - window_size; cpp <= cp + window_size; cpp++) {
            int cpp2 = cpp;
            if (cpp < 0)
                cpp2 = contour.size() + cpp;
            else if (cpp >= (int)contour.size())
                cpp2 = cpp - contour.size();
            sumx += contour[cpp2].x;
            sumy += contour[cpp2].y;
        }
        sumx /= (window_size * 2 + 1);
        sumy /= (window_size * 2 + 1);
        out_contour[cp] = MAKE_INT2(sumx, sumy);
    }
    return true;
}

bool extractValidContour(std::vector<INT2> contour, INT2 center, std::vector<INT2>& out_contour, int angle_threshold, int angle_threshold2)
{
    // loop over each contour point to get the cloest point
    float short_dist = FLT_MAX;
    int short_ind = -1;
    INT2 short_pt;
    for (size_t cp = 0; cp < contour.size(); cp++) {
        INT2 p = contour[cp];
        float dist = __LENGTH_2D(center.x, center.y, p.x, p.y);
        if (short_dist > dist) {
            short_dist = dist;
            short_ind = cp;
            short_pt = p;
        }
    }

    // extend contour (duplicate it three times)
    std::vector<INT2> contour_ext(contour.begin(), contour.end());
    contour_ext.insert(contour_ext.end(), contour.begin(), contour.end());
    contour_ext.insert(contour_ext.end(), contour.begin(), contour.end());
    short_ind += contour.size();
    int short_ind2 = short_ind - contour.size();
    int short_ind1 = short_ind + contour.size();

    // from the cloest point check before and after until there is abrupt curvature (also should have consistent curvature)
    int contour_interval = __MIN(20, contour.size() / 20);
    int contour_interval2 = contour_interval / 2;
    size_t cp1 = short_ind;
    float orientation1 = __ORIENTATION(center.x, center.y, contour_ext[cp1].x, contour_ext[cp1].y,
                                       contour_ext[cp1+contour_interval2].x, contour_ext[cp1+contour_interval2].y);
    for (; cp1 < contour_ext.size() - contour_interval; cp1++) {
        float angle = getLineAngle(contour_ext[cp1],
                                   contour_ext[cp1+contour_interval2],
                                   contour_ext[cp1+contour_interval]);
        if (angle < angle_threshold)
            break;
        angle = getLineAngle(contour_ext[cp1], center, contour_ext[cp1+contour_interval]);
        if (angle < angle_threshold2)
            break;

        float orientation = __ORIENTATION(center.x, center.y, contour_ext[cp1].x, contour_ext[cp1].y,
                                          contour_ext[cp1+contour_interval].x, contour_ext[cp1+contour_interval].y);
        if (orientation1 * orientation < 0) // different direction
            break;

        if (cp1 >= short_ind1)
            break;
    }
    cp1 += contour_interval2;
    int cp2 = short_ind - contour_interval;
    float orientation2 = __ORIENTATION(center.x, center.y, contour_ext[cp2].x, contour_ext[cp2].y,
                                       contour_ext[cp2+contour_interval2].x, contour_ext[cp2+contour_interval2].y);
    for (; cp2 >= 0; cp2--) {
        float angle = getLineAngle(contour_ext[cp2],
                                   contour_ext[cp2+contour_interval2],
                                   contour_ext[cp2+contour_interval]);
        if (angle < angle_threshold)
            break;
        angle = getLineAngle(contour_ext[cp2], center, contour_ext[cp2+contour_interval]);
        if (angle < angle_threshold2)
            break;

        float orientation = __ORIENTATION(center.x, center.y, contour_ext[cp2].x, contour_ext[cp2].y,
                                          contour_ext[cp2+contour_interval].x, contour_ext[cp2+contour_interval].y);
        if (orientation2 * orientation < 0) // different direction
            break;

        if (cp2 <= short_ind2)
            break;
    }
    cp2 += contour_interval2;
    out_contour.insert(out_contour.end(), &contour_ext[cp2], &contour_ext[short_ind]);
    out_contour.insert(out_contour.end(), &contour_ext[short_ind], &contour_ext[cp1]);

    return true;
}

float computeEllipse(float r1, float r2, float cx, float cy, float angle, float x, float y)
{
    float angle_rad = __DEG2RAD(angle);
    float first  = ((x - cx) * cos(angle_rad) + (y - cy) * sin(angle_rad));
    float second = ((x - cx) * sin(angle_rad) - (y - cy) * cos(angle_rad));
    float ret = (first * first) / (r1 * r1) + (second * second) / (r2 * r2) - 1;
    return ret;
}

int getEllipsePixels(float r1, float r2, float cx, float cy, float angle,
                      int xmin, int ymin, int xmax, int ymax, std::vector<INT2>& ellipse_pixels) // get all pixels to draw filled ellipse
{
    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            float ret = computeEllipse(r1, r2, cx, cy, angle, x, y);
            if (ret <= 0) {
                ellipse_pixels.push_back(MAKE_INT2(x, y));
            }
        }
    }

    return (int)ellipse_pixels.size();
}

FLOAT2 getNearestEllipse(float r1, float r2, float cx, float cy, float angle, float x, float y)
{
    // parametric eq
    float min_ret = r1 * r1 + r2 * r2;
    float min_x = x;
    float min_y = y;

    for (float t = 0; t <= 10; t += 0.05) {
        float x1 = cx + t * (x - cx);
        float y1 = cy + t * (y - cy);

        float ret = computeEllipse(r1, r2, cx, cy, angle, x1, y1);
        ret = fabs(ret);
        if (min_ret > ret) {
            min_ret = ret;
            min_x = x1;
            min_y = y1;
        }
    }

    FLOAT2 ret = MAKE_FLOAT2(min_x, min_y);
    return ret;
}

FLOAT2 rotatePoint(FLOAT2 cpoint, float angle, FLOAT2 point)
{
    float s = sin(__DEG2RAD(angle));
    float c = cos(__DEG2RAD(angle));

    point.x -= cpoint.x;
    point.y -= cpoint.y;

    FLOAT2 point_new;
    point_new.x = point.x * c - point.y * s;
    point_new.y = point.x * s + point.y * c;

    point_new.x += cpoint.x;
    point_new.y += cpoint.y;

    return point_new;
}

float pointToLineDistance(FLOAT2 p, FLOAT2 a, FLOAT2 b)
{
    float p1, a1, b1;
    float cos_pab, cos_pba;

    p1 = __LENGTH_2D(a.x, a.y, b.x, b.y);
    b1 = __LENGTH_2D(p.x, p.y, a.x, a.y);
    a1 = __LENGTH_2D(p.x, p.y, b.x, b.y);

    if (p1 == 0.0)
        return a1;

    if (a1 == 0.0 || b1 == 0.0)
        return 0.0;

    cos_pab = __COS2TH(p1, a1, b1);
    cos_pba = __COS2TH(p1, b1, a1);

    if (cos_pab >= 0 && cos_pba >= 0)
        return fabs(__ORIENTATION(a.x, a.y, b.x, b.y, p.x, p.y)) / p1;
    else
        return __MIN(a1, b1);
}

float getLineAngle(INT2 a, INT2 p, INT2 b)
{
    float pd = __LENGTH_2D(a.x, a.y, b.x, b.y);
    float ad = __LENGTH_2D(p.x, p.y, b.x, b.y);
    float bd = __LENGTH_2D(p.x, p.y, a.x, a.y);
    float cos_angle = __COS2TH(ad, pd, bd);
    float angle = __RAD2DEG(safe_acos(cos_angle));
    return angle;
}

float check_overlap(INT4& bbox1, INT4& bbox2)
{
    int xmin1 = bbox1.x;
    int ymin1 = bbox1.y;
    int xmax1 = bbox1.z;
    int ymax1 = bbox1.w;
    int xmin2 = bbox2.x;
    int ymin2 = bbox2.y;
    int xmax2 = bbox2.z;
    int ymax2 = bbox2.w;
    if (xmin1 > xmax2 || xmax1 < xmin2 || ymin1 > ymax2 || ymax1 < ymin2)
        return 0;

    if (xmin1 >= xmin2 && xmax1 <= xmax2 && ymin1 >= ymin2 && ymax1 <= ymax2)
        return 1; // right contains left
    else if (xmin2 >= xmin1 && xmax2 <= xmax1 && ymin2 >= ymin1 && ymax2 <= ymax1)
        return -1; // left contains right

    int xmin = std::max(xmin1, xmin2);
    int ymin = std::max(ymin1, ymin2);
    int xmax = std::min(xmax1, xmax2);
    int ymax = std::min(ymax1, ymax2);
    float area = (ymax - ymin + 1) * (xmax - xmin + 1);
    float area2 = (ymax2 - ymin2 + 1) * (xmax2 - xmin2 + 1);
    return area / area2;
}

float distanceContourToRectangle(INT2 rect_pts[4], std::vector<INT2>& contour_pixels)
{
    float sum_dist = 0;
    for (size_t cp = 0; cp < contour_pixels.size(); cp++) {
        FLOAT2 pt = MAKE_FLOAT2(contour_pixels[cp].x, contour_pixels[cp].y);

        int p1 = 3;
        float short_dist = FLT_MAX;
        for (int p2 = 0; p2 < 4; p2++) {
            FLOAT2 a = MAKE_FLOAT2(rect_pts[p1].x, rect_pts[p1].y);
            FLOAT2 b = MAKE_FLOAT2(rect_pts[p2].x, rect_pts[p2].y);
            float dist = pointToLineDistance(pt, a, b);
            short_dist = __MIN(short_dist, dist);
            p1 = p2;
        }
        sum_dist += short_dist;
    }
    sum_dist /= contour_pixels.size();
    return sum_dist;
}

float distanceContourToEllipse(float r1, float r2, float cx, float cy, float angle,
                               std::vector<INT2>& contour_pixels, std::vector<INT2>& fit_pixels)
{
    float sum_dist = 0;
    for (size_t cp = 0; cp < contour_pixels.size(); cp++) {
        float x = contour_pixels[cp].x;
        float y = contour_pixels[cp].y;
        FLOAT2 contour_nearest = getNearestEllipse(r1, r2, cx, cy, angle, x, y);
        fit_pixels.push_back(MAKE_INT2(contour_nearest.x, contour_nearest.y));
        sum_dist += __LENGTH_2D(x, y, contour_nearest.x, contour_nearest.y);
    }
    sum_dist /= contour_pixels.size();
    return sum_dist;
}

void getCentroid(std::vector<INT2>& pixels, INT2& centroid)
{
    if (pixels.size() == 1) {
        centroid = pixels[0];
        return;
    }

    centroid = MAKE_INT2(0, 0);
    for (size_t n = 0; n < pixels.size(); n++) {
        INT2 p = pixels[n];
        centroid.x += p.x;
        centroid.y += p.y;
    }
    centroid.x = (int)((float)centroid.x / pixels.size());
    centroid.y = (int)((float)centroid.y / pixels.size());
}

void getLinePixels(int x1, int y1, int x2, int y2, std::vector<INT2>& pixels)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    float count = sqrt(dx * dx + dy * dy);
    float t_interval = 1.0 / count;
    for (float t = 0; t <= 1; t+= t_interval) {
        float px = dx * t + x1;
        float py = dy * t + y1;
        pixels.push_back(MAKE_INT2((int)px, (int)py));
    }


    /*
    // use line drawing algorithm
    int dx, dy, incx, incy, d, e, ne;
    int x, y, start, end;

    if (x1 == x2 && y1 == y2) { // one pixel
        return;
    }
    if (x1 == x2) { // vertical
        start = __MIN(y1, y2);
        end = __MAX(y1, y2);
        for (y = start; y <= end; y++)
            pixels.push_back(MAKE_INT2(x1, y));
        return;
    }
    if (y1 == y2) { // horizontal
        start = __MIN(x1, x2);
        end = __MAX(x1, x2);
        for (x = start; x <= end; x++)
            pixels.push_back(MAKE_INT2(x, y1));
        return;
    }

    if (abs(y2 - y1) <= abs(x2 - x1)) { // slope is less than 45 degree (m <= 1)
        if (x1 > x2) { // swap (x1, y1) and (x2, y2)
            x = x1; x1 = x2; x2 = x;
            y = y1; y1 = y2; y2 = y;
        }

        dx = x2 - x1;
        dy = y2 - y1;
        incy = 1;
        if (dy < 0) { // line goes up or down
            dy = -dy;
            incy = -1;
        }
        d = 2 * dy - dx; // just same as the lecture note
        ne = 2 * (dy - dx);
        e = 2 * dy;

        for (x = x1, y = y1; x <= x2; x++) {
            pixels.push_back(MAKE_INT2(x, y));
            if (d > 0) {
                d += ne;
                y += incy;
            }
            else {
                d += e;
            }
        }
    }
    else { // slope is larger than 45 degree (m > 1, steep)
        if (y1 > y2) { // swap (x1, y1) and (x2, y2)
            x = x1; x1 = x2; x2 = x;
            y = y1; y1 = y2; y2 = y;
        }

        dx = x2 - x1;
        dy = y2 - y1;
        incx = 1;
        if (dx < 0) { // line goes left
            dx = -dx;
            incx = -1;
        }
        d = 2 * dx - dy; // just same as the lecture note
        ne = 2 * (dx - dy);
        e = 2 * dx;

        for (y = y1, x = x1; y <= y2; y++) {
            pixels.push_back(MAKE_INT2(x, y));
            if (d > 0) {
                d += ne;
                x += incx;
            }
            else {
                d += e;
            }
        }
    }*/
}

void drawOutImage(Mat& cvimage, bool use_image, std::string filepath, bool flip,
                  bool draw_point, std::vector<INT2>* point_list, INT3 point_color, int point_size,
                  bool draw_contour, std::vector<std::vector<INT2> >* contour_list, INT3 contour_color,
                  bool draw_rectangle, std::vector<RotatedRect>* rect_list, INT3 rect_color,
                  bool draw_ellipse, std::vector<RotatedRect>* ellipse_list, INT3 ellipse_color,
                  bool use_rect_ellipse_type, std::vector<int>* rect_ellipse_type_list)
{
    Mat cvimage_rgb;
    Mat* outimage;
    if (use_image) {
        outimage = &cvimage;
    }
    else {
        cvtColor(cvimage, cvimage_rgb, COLOR_GRAY2RGB);
        outimage = &cvimage_rgb;
    }

    if (draw_point) {
        for (size_t n = 0; n < point_list->size(); n++) {
            INT2 point = (*point_list)[n];
            if (flip) {
                circle(*outimage, Point(point.x, cvimage.rows - 1 - point.y), point_size, \
                       Scalar(point_color.x, point_color.y, point_color.z), -1);
            }
            else {
                circle(*outimage, Point(point.x, point.y), point_size, \
                       Scalar(point_color.x, point_color.y, point_color.z), -1);
            }
        }
    }

    if (draw_contour) {
        for (size_t n = 0; n < contour_list->size(); n++) {
            std::vector<INT2> contour = (*contour_list)[n];
            for (int c = 0; c < contour.size(); c++) {
                int x = contour[c].x;
                int y = contour[c].y;
                if (flip)
                    y = cvimage.rows - 1 - y;
                Vec3b color;
                color[0] = contour_color.x; color[1] = contour_color.y; color[2] = contour_color.z;
                for (int cy = -1; cy <= 1; cy++)
                    for (int cx = -1; cx <= 1; cx++)
                        outimage->at<Vec3b>(y+cy, x+cx) = color;
            }
        }
    }

    if (draw_rectangle && !use_rect_ellipse_type) {
        for (size_t n = 0; n < rect_list->size(); n++) {
            RotatedRect rect = (*rect_list)[n];
            Point2f rect_pts[4];
            rect.points(rect_pts);
            for (int i = 0; i < 4; i++) {
                Point2f a = rect_pts[i];
                Point2f b = rect_pts[(i+1) % 4];
                if (!flip) {
                    a.y = cvimage.rows - 1 - a.y;
                    b.y = cvimage.rows - 1 - b.y;
                }
                line(*outimage, a, b, Scalar(rect_color.x, rect_color.y, rect_color.z), 2);
            }
        }
    }

    if (draw_ellipse && !use_rect_ellipse_type) {
        for (size_t n = 0; n < ellipse_list->size(); n++) {
            RotatedRect segment_ellipse = (*ellipse_list)[n];
            ellipse(*outimage, segment_ellipse, Scalar(ellipse_color.x, ellipse_color.y, ellipse_color.z), 2, 8);
        }
    }

    if (draw_rectangle && draw_ellipse && use_rect_ellipse_type) {
        for (size_t n = 0; n < rect_ellipse_type_list->size(); n++) {
            int shape_type = (*rect_ellipse_type_list)[n];
            if (shape_type == 1) {
                RotatedRect rect = (*rect_list)[n];
                Point2f rect_pts[4];
                rect.points(rect_pts);
                for (int i = 0; i < 4; i++) {
                    Point2f a = rect_pts[i];
                    Point2f b = rect_pts[(i+1) % 4];
                    if (!flip) {
                        a.y = cvimage.rows - 1 - a.y;
                        b.y = cvimage.rows - 1 - b.y;
                    }
                    line(*outimage, a, b, Scalar(rect_color.x, rect_color.y, rect_color.z), 2);
                }
            }
            else if (shape_type == 2) {
                RotatedRect segment_ellipse = (*ellipse_list)[n];
                ellipse(*outimage, segment_ellipse, Scalar(ellipse_color.x, ellipse_color.y, ellipse_color.z), 2, 8);
            }
        }
    }

    // old code
    /*
        cvtColor(cvimage, outimage, COLOR_GRAY2RGB);
        for (size_t n = 0; n < center_list.size(); n++) {
            INT2 center = center_list[n];
            circle(outimage, Point(center.x, cvimage.rows - 1 - center.y), 3, Scalar(0, 255, 255), -1);

            vector<INT2> contour = contour_list[n];
            for (int c = 0; c < contour.size(); c++) {
                int x = contour[c].x;
                int y = cvimage.rows - 1 - contour[c].y;
                Vec3b color;
                color[0] = 0; color[1] = 255; color[2] = 0;
                for (int cy = -1; cy <= 1; cy++)
                    for (int cx = -1; cx <= 1; cx++)
                        outimage.at<Vec3b>(y+cy, x+cx) = color;
            }

            float dist = dist_list[n];
            float dist_color = __MIN(255, dist / 2 * 255);
            RotatedRect segment_ellipse = ellipse_list[n];
            ellipse(outimage, segment_ellipse, Scalar(0, 0, dist_color), 2, 8);
        }
        imwrite(out_image_prefix + "_centers_contour_ellipse.png", outimage);
    */

    if (filepath.length() > 0)
        imwrite(filepath, *outimage);
}

void drawOutImage2(Mat& cvimage, std::string filepath, bool flip,
                   bool draw_point, std::vector<INT2>* point_list, INT3 point_color, int point_size,
                   bool draw_contour, std::vector<std::vector<INT2> >* contour_list, INT3 contour_color,
                   bool draw_shortest, std::vector<INT2>* shortest_pt_list, INT3 shortest_color,
                   bool draw_longest, std::vector<INT2>* longest_pt_list, INT3 longest_color)
{
    Mat outimage;
    cvtColor(cvimage, outimage, COLOR_GRAY2RGB);

    if (draw_point) {
        for (size_t n = 0; n < point_list->size(); n++) {
            INT2 point = (*point_list)[n];
            if (flip) {
                circle(outimage, Point(point.x, cvimage.rows - 1 - point.y), point_size, \
                       Scalar(point_color.x, point_color.y, point_color.z), -1);
            }
            else {
                circle(outimage, Point(point.x, point.y), point_size, \
                       Scalar(point_color.x, point_color.y, point_color.z), -1);
            }
        }
    }
    if (draw_contour) {
        for (size_t n = 0; n < contour_list->size(); n++) {
            std::vector<INT2> contour = (*contour_list)[n];
            for (int c = 0; c < contour.size(); c++) {
                int x = contour[c].x;
                int y = contour[c].y;
                if (flip)
                    y = cvimage.rows - 1 - y;
                Vec3b color;
                color[0] = contour_color.x; color[1] = contour_color.y; color[2] = contour_color.z;
                for (int cy = -1; cy <= 1; cy++)
                    for (int cx = -1; cx <= 1; cx++)
                        outimage.at<Vec3b>(y+cy, x+cx) = color;
            }
        }
    }
    if (draw_shortest) {
        for (size_t n = 0; n < shortest_pt_list->size(); n++) {
            INT2 point = (*shortest_pt_list)[n];
            if (flip) {
                circle(outimage, Point(point.x, cvimage.rows - 1 - point.y), point_size, \
                       Scalar(shortest_color.x, shortest_color.y, shortest_color.z), -1);
            }
            else {
                circle(outimage, Point(point.x, point.y), point_size, \
                       Scalar(shortest_color.x, shortest_color.y, shortest_color.z), -1);
            }
        }
    }
    if (draw_longest) {
        for (size_t n = 0; n < shortest_pt_list->size(); n++) {
            INT2 point = (*longest_pt_list)[n];
            if (flip) {
                circle(outimage, Point(point.x, cvimage.rows - 1 - point.y), point_size, \
                       Scalar(longest_color.x, longest_color.y, longest_color.z), -1);
            }
            else {
                circle(outimage, Point(point.x, point.y), point_size, \
                       Scalar(longest_color.x, longest_color.y, longest_color.z), -1);
            }
        }
    }

    imwrite(filepath, outimage);
}
