//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Segmentation class (.h, .cpp)
//*****************************************************************************/

#include "segmenter.h"

#include <assert.h>
#include <algorithm>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <cstring>

#ifdef __LIB_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
using namespace cv;
#endif



//------------------------------------------------------------------------------
// CImage class
//------------------------------------------------------------------------------

CImage::CImage()
{
    m_Dim = MAKE_INT4(0, 0, 0, 0);
    m_Allocated = false;
    m_Pixels = NULL;
}

CImage::CImage(const CImage& Other)
{
    m_Dim = MAKE_INT4(0, 0, 0, 0);
    m_Allocated = false;
    m_Pixels = NULL;
    this->init(Other.m_Dim.x, Other.m_Dim.y, Other.m_Dim.z, Other.m_Dim.w,
               Other.m_Pixels, Other.m_Allocated);
}

CImage::CImage(int Width, int Height, int Depth, int Channel, PixelType* Pixels, bool AllocateImage)
{
    m_Dim = MAKE_INT4(0, 0, 0, 0);
    m_Allocated = false;
    m_Pixels = NULL;
    this->init(Width, Height, Depth, Channel, Pixels, AllocateImage);
}

CImage::~CImage()
{
    this->free();
}

void CImage::init(const CImage& Other)
{
    this->free();
    this->init(Other.m_Dim.x, Other.m_Dim.y, Other.m_Dim.z, Other.m_Dim.w,
               Other.m_Pixels, Other.m_Allocated);
}

void CImage::init(int Width, int Height, int Depth, int Channel, PixelType* Pixels, bool AllocateImage)
{
    this->free();
    
    m_Dim = MAKE_INT4(Width, Height, Depth, Channel);
    m_Allocated = AllocateImage;
    if (m_Allocated) {
        int count = m_Dim.x * m_Dim.y * m_Dim.z * m_Dim.w;
        m_Pixels = new PixelType[count];
        if (Pixels)
            memcpy(m_Pixels, Pixels, sizeof(PixelType) * count);
    }
    else {
        m_Pixels = Pixels;
    }
}

void CImage::free()
{
    if (m_Allocated && m_Pixels) {
        delete [] m_Pixels;
        m_Pixels = NULL;
    }
}

CImage& CImage::operator = (const CImage& Other)
{
    this->free();
    this->init(Other);
    return *this;
}

int CImage::getWidth()
{
    return m_Dim.x;
}

int CImage::getHeight()
{
    return m_Dim.y;
}

int CImage::getDepth()
{
    return m_Dim.z;
}

int CImage::getNumChannels()
{
    return m_Dim.w;
}

int CImage::getNumPixels()
{
    return (m_Dim.x * m_Dim.y * m_Dim.z);
}

INT4 CImage::getDimension()
{
    return m_Dim;
}

PixelType* CImage::getPixel(int x)
{
    assert(x >= 0 && x < m_Dim.x * m_Dim.y * m_Dim.z);
    return &m_Pixels[x * m_Dim.w];
}

PixelType* CImage::getPixel(int x, int y)
{
    assert(x >= 0 && x < m_Dim.x && y >= 0 && y < m_Dim.y);
    return &m_Pixels[(y * m_Dim.x + x) * m_Dim.w];
}

PixelType* CImage::getPixel(int x, int y, int z)
{
    assert(x >= 0 && x < m_Dim.x && y >= 0 && y < m_Dim.y && z >= 0 && z < m_Dim.z);
    return &m_Pixels[(z * m_Dim.y * m_Dim.x + y * m_Dim.x + x) * m_Dim.w];
}

PixelType* CImage::getPixels()
{
    return m_Pixels;
}

void CImage::setPixel(int x, PixelType* value)
{
    assert(x >= 0 && x < m_Dim.x * m_Dim.y * m_Dim.z);
    PixelType* p = &m_Pixels[x * m_Dim.w];
    for (int n = 0; n < m_Dim.w; n++)
        p[n] = value[n];
}

void CImage::setPixel(int x, int y, PixelType* value)
{
    assert(x >= 0 && x < m_Dim.x && y >= 0 && y < m_Dim.y);
    PixelType* p = &m_Pixels[(y * m_Dim.x + x) * m_Dim.w];
    for (int n = 0; n < m_Dim.w; n++)
        p[n] = value[n];
}

void CImage::setPixel(int x, int y, int z, PixelType* value)
{
    assert(x >= 0 && x < m_Dim.x && y >= 0 && y < m_Dim.y && z >= 0 && z < m_Dim.z);
    PixelType* p = &m_Pixels[(z * m_Dim.y * m_Dim.x + y * m_Dim.x + x) * m_Dim.w];
    for (int n = 0; n < m_Dim.w; n++)
        p[n] = value[n];
}

bool CImage::getHistogram(int NumBins, int* Bins)
{
    if (m_Dim.w != 1)
        return false;

    // initialize bins
    for (int n = 0; n < NumBins; n++)
        Bins[n] = 0;

    int numpixels = m_Dim.x * m_Dim.y * m_Dim.z;
    for (int n = 0; n < numpixels; n++) {
        int npixel = (int)(m_Pixels[n] * (NumBins - 1));
        Bins[npixel]++;
    }
    return true;
}



//------------------------------------------------------------------------------
// CSegment class
//------------------------------------------------------------------------------

CSegment::CSegment(CSegmenter* Segmenter) : m_Sid(-1), m_Tag(0), m_Segmenter(Segmenter)
{
    m_BoundMin = 0;
    m_BoundMax = 0;
}

CSegment::CSegment(const CSegment& Other) : m_Sid(Other.m_Sid), m_Tag(Other.m_Tag), m_Segmenter(Other.m_Segmenter)
{
    m_BoundMin = Other.m_BoundMin;
    m_BoundMax = Other.m_BoundMax;
    m_Pixels = Other.m_Pixels;
}

CSegment& CSegment::operator = (const CSegment& Other)
{
    m_Segmenter = Other.m_Segmenter;
    m_Sid = Other.m_Sid;
    m_Tag = Other.m_Tag;
    m_BoundMin = Other.m_BoundMin;
    m_BoundMax = Other.m_BoundMax;
    m_Pixels = Other.m_Pixels;
    return *this;
}

void CSegment::init(int Sid, int NumPixels, int* Pixels)
{
    m_Sid = Sid;
    m_Pixels.clear();

    // add pixels, recompute bounding box
    m_BoundMin = m_Segmenter->getImage()->getWidth();
    m_BoundMax = -1;
    for (int n = 0; n < NumPixels; n++) {
        int pixel = Pixels[n];
        m_Pixels.push_back(pixel);
        m_BoundMin = __MIN(m_BoundMin, pixel);
        m_BoundMax = __MAX(m_BoundMax, pixel);
    }
}

void CSegment::init(int Sid, int NumPixels, INT2* Pixels)
{
    m_Sid = Sid;
    m_Pixels.clear();

    // add pixels, recompute bounding box
    int width = m_Segmenter->getImage()->getWidth();
    int xmin = m_Segmenter->getImage()->getWidth();
    int ymin = m_Segmenter->getImage()->getHeight();
    int xmax = -1;
    int ymax = -1;
    for (int n = 0; n < NumPixels; n++) {
        int pindex = Pixels[n].y * width + Pixels[n].x;
        m_Pixels.push_back(pindex);

        xmin = __MIN(xmin, Pixels[n].x);
        ymin = __MIN(ymin, Pixels[n].y);
        xmax = __MAX(xmax, Pixels[n].x);
        ymax = __MAX(ymax, Pixels[n].y);
    }
    m_BoundMin = ymin * width + xmin;
    m_BoundMax = ymax * width + xmax;
}

void CSegment::init(int Sid, int NumPixels, INT3* Pixels)
{
    m_Sid = Sid;
    m_Pixels.clear();

    // add pixels, recompute bounding box
    int width = m_Segmenter->getImage()->getWidth();
    int height = m_Segmenter->getImage()->getHeight();
    int xmin = m_Segmenter->getImage()->getWidth();
    int ymin = m_Segmenter->getImage()->getHeight();
    int zmin = m_Segmenter->getImage()->getDepth();
    int xmax = -1;
    int ymax = -1;
    int zmax = -1;
    for (int n = 0; n < NumPixels; n++) {
        int pindex = Pixels[n].z * width * height + Pixels[n].y * width + Pixels[n].x;
        m_Pixels.push_back(pindex);

        xmin = __MIN(xmin, Pixels[n].x);
        ymin = __MIN(ymin, Pixels[n].y);
        zmin = __MIN(zmin, Pixels[n].z);
        xmax = __MAX(xmax, Pixels[n].x);
        ymax = __MAX(ymax, Pixels[n].y);
        zmax = __MAX(zmax, Pixels[n].z);
    }
    m_BoundMin = zmin * width * height + ymin * width + xmin;
    m_BoundMax = zmax * width * height + ymax * width + xmax;
}

int CSegment::getNumPixels()
{
    return (int)m_Pixels.size();
}

int CSegment::getPixels(std::vector<int>& Pixels)
{
    Pixels = m_Pixels;
    return (int)m_Pixels.size();
}

int CSegment::getPixels(std::vector<INT2>& Pixels)
{
    Pixels.resize(m_Pixels.size());

    int width = m_Segmenter->getImage()->getWidth();
    for (size_t n = 0; n < m_Pixels.size(); n++) {
        int pindex = m_Pixels[n];
        Pixels[n].x = pindex % width;
        Pixels[n].y = pindex / width;
    }
    return (int)m_Pixels.size();
}

int CSegment::getPixels(std::vector<INT3>& Pixels)
{
    Pixels.resize(m_Pixels.size());

    int width = m_Segmenter->getImage()->getWidth();
    int height = m_Segmenter->getImage()->getHeight();
    for (size_t n = 0; n < m_Pixels.size(); n++) {
        int pindex = m_Pixels[n];
        int z = pindex / (width * height);
        int y = (pindex - z * width * height) / width;
        int x = (pindex - z * width * height - y * width) % width;
        Pixels[n] = MAKE_INT3(x, y, z);
    }
    return (int)m_Pixels.size();
}

void CSegment::getBoundBox(int& XMin, int& XMax)
{
    XMin = m_BoundMin;
    XMax = m_BoundMax;
}

void CSegment::getBoundBox(int& XMin, int& YMin, int& XMax, int& YMax)
{
    int width = m_Segmenter->getImage()->getWidth();
    XMin = m_BoundMin % width;
    YMin = m_BoundMin / width;
    XMax = m_BoundMax % width;
    YMax = m_BoundMax / width;
}

void CSegment::getBoundBox(int& XMin, int& YMin, int& ZMin, int& XMax, int& YMax, int& ZMax)
{
    int width = m_Segmenter->getImage()->getWidth();
    int height = m_Segmenter->getImage()->getHeight();
    ZMin = m_BoundMin / (width * height);
    YMin = (m_BoundMin - ZMin * width * height) / width;
    XMin = (m_BoundMin - ZMin * width * height - YMin * width) % width;
    ZMax = m_BoundMax / (width * height);
    YMax = (m_BoundMax - ZMax * width * height) / width;
    XMax = (m_BoundMax - ZMax * width * height - YMax * width) % width;
}

int CSegment::getNumBoundPixels()
{
    int height = m_Segmenter->getImage()->getHeight();
    int depth = m_Segmenter->getImage()->getDepth();

    int bcount = 0;
    if (height == 1 && depth == 1) {
        for (size_t n = 0; n < m_Pixels.size(); n++) {
            int x = m_Pixels[n];
            int sid1 = m_Segmenter->getSegmentId(x - 1);
            int sid2 = m_Segmenter->getSegmentId(x + 1);
            if (sid1 != m_Sid || sid2 != m_Sid)
                bcount++;
        }
    }
    else if (depth == 1) {
        std::vector<INT2> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1);
            if (sid1 != m_Sid || sid2 != m_Sid || sid3 != m_Sid || sid4 != m_Sid)
                bcount++;
        }
    }
    else {
        std::vector<INT3> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int z = pixels[n].z;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y, z);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y, z);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1, z);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1, z);
            int sid5 = m_Segmenter->getSegmentId(x, y, z - 1);
            int sid6 = m_Segmenter->getSegmentId(x, y, z + 1);
            if (sid1 != m_Sid || sid2 != m_Sid || sid3 != m_Sid ||
                sid4 != m_Sid || sid5 != m_Sid || sid6 != m_Sid)
                bcount++;
        }
    }
    return bcount;
}

int CSegment::getBoundPixels(std::vector<int>& BoundPixels)
{
    for (size_t n = 0; n < m_Pixels.size(); n++) {
        int x = m_Pixels[n];
        int sid1 = m_Segmenter->getSegmentId(x - 1);
        int sid2 = m_Segmenter->getSegmentId(x + 1);
        if (sid1 != m_Sid || sid2 != m_Sid)
            BoundPixels.push_back(x);
    }
    return (int)BoundPixels.size();
}

int CSegment::getBoundPixels(std::vector<INT2>& BoundPixels)
{
    std::vector<INT2> pixels;
    this->getPixels(pixels);
    for (size_t n = 0; n < pixels.size(); n++) {
        int x = pixels[n].x;
        int y = pixels[n].y;
        int sid1 = m_Segmenter->getSegmentId(x - 1, y);
        int sid2 = m_Segmenter->getSegmentId(x + 1, y);
        int sid3 = m_Segmenter->getSegmentId(x, y - 1);
        int sid4 = m_Segmenter->getSegmentId(x, y + 1);
        if (sid1 != m_Sid || sid2 != m_Sid || sid3 != m_Sid || sid4 != m_Sid)
            BoundPixels.push_back(MAKE_INT2(x, y));
    }
    return (int)BoundPixels.size();
}

int CSegment::getBoundPixels(std::vector<INT3>& BoundPixels)
{
    std::vector<INT3> pixels;
    this->getPixels(pixels);
    for (size_t n = 0; n < pixels.size(); n++) {
        int x = pixels[n].x;
        int y = pixels[n].y;
        int z = pixels[n].z;
        int sid1 = m_Segmenter->getSegmentId(x - 1, y, z);
        int sid2 = m_Segmenter->getSegmentId(x + 1, y, z);
        int sid3 = m_Segmenter->getSegmentId(x, y - 1, z);
        int sid4 = m_Segmenter->getSegmentId(x, y + 1, z);
        int sid5 = m_Segmenter->getSegmentId(x, y, z - 1);
        int sid6 = m_Segmenter->getSegmentId(x, y, z + 1);
        if (sid1 != m_Sid || sid2 != m_Sid || sid3 != m_Sid ||
            sid4 != m_Sid || sid5 != m_Sid || sid6 != m_Sid)
            BoundPixels.push_back(MAKE_INT3(x, y, z));
    }
    return (int)BoundPixels.size();
}

int CSegment::getNumBoundPixels(CSegment& Other)
{
    int height = m_Segmenter->getImage()->getHeight();
    int depth = m_Segmenter->getImage()->getDepth();
    int asid = Other.getSid();

    int bcount = 0;
    if (height == 1 && depth == 1) {
        for (size_t n = 0; n < m_Pixels.size(); n++) {
            int x = m_Pixels[n];
            int sid1 = m_Segmenter->getSegmentId(x - 1);
            int sid2 = m_Segmenter->getSegmentId(x + 1);
            if (sid1 == asid || sid2 == asid)
                bcount++;
        }
    }
    else if (depth == 1) {
        std::vector<INT2> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1);
            if (sid1 == asid || sid2 == asid || sid3 == asid || sid4 == asid)
                bcount++;
        }
    }
    else {
        std::vector<INT3> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int z = pixels[n].z;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y, z);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y, z);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1, z);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1, z);
            int sid5 = m_Segmenter->getSegmentId(x, y, z - 1);
            int sid6 = m_Segmenter->getSegmentId(x, y, z + 1);
            if (sid1 == asid || sid2 == asid || sid3 == asid ||
                sid4 == asid || sid5 == asid || sid6 == asid)
                bcount++;
        }
    }
    return bcount;
}

int CSegment::getBoundPixels(CSegment& Other, std::vector<int>& BoundPixels)
{
    int asid = Other.getSid();
    for (size_t n = 0; n < m_Pixels.size(); n++) {
        int x = m_Pixels[n];
        int sid1 = m_Segmenter->getSegmentId(x - 1);
        int sid2 = m_Segmenter->getSegmentId(x + 1);
        if (sid1 == asid || sid2 == asid)
            BoundPixels.push_back(x);
    }
    return (int)BoundPixels.size();
}

int CSegment::getBoundPixels(CSegment& Other, std::vector<INT2>& BoundPixels)
{
    int asid = Other.getSid();
    std::vector<INT2> pixels;
    this->getPixels(pixels);
    for (size_t n = 0; n < pixels.size(); n++) {
        int x = pixels[n].x;
        int y = pixels[n].y;
        int sid1 = m_Segmenter->getSegmentId(x - 1, y);
        int sid2 = m_Segmenter->getSegmentId(x + 1, y);
        int sid3 = m_Segmenter->getSegmentId(x, y - 1);
        int sid4 = m_Segmenter->getSegmentId(x, y + 1);
        if (sid1 == asid || sid2 == asid || sid3 == asid || sid4 == asid)
            BoundPixels.push_back(MAKE_INT2(x, y));
    }
    return (int)BoundPixels.size();
}

int CSegment::getBoundPixels(CSegment& Other, std::vector<INT3>& BoundPixels)
{
    int asid = Other.getSid();
    std::vector<INT3> pixels;
    this->getPixels(pixels);
    for (size_t n = 0; n < pixels.size(); n++) {
        int x = pixels[n].x;
        int y = pixels[n].y;
        int z = pixels[n].z;
        int sid1 = m_Segmenter->getSegmentId(x - 1, y, z);
        int sid2 = m_Segmenter->getSegmentId(x + 1, y, z);
        int sid3 = m_Segmenter->getSegmentId(x, y - 1, z);
        int sid4 = m_Segmenter->getSegmentId(x, y + 1, z);
        int sid5 = m_Segmenter->getSegmentId(x, y, z - 1);
        int sid6 = m_Segmenter->getSegmentId(x, y, z + 1);
        if (sid1 == asid || sid2 == asid || sid3 == asid ||
            sid4 == asid || sid5 == asid || sid6 == asid)
            BoundPixels.push_back(MAKE_INT3(x, y, z));
    }
    return (int)BoundPixels.size();
}

int CSegment::getCentroid(int& Centroid)
{
    if (m_Pixels.size() == 0)
        return 0;

    std::vector<int> pixels;
    this->getPixels(pixels);

    Centroid = 0;
    for (size_t n = 0; n < pixels.size(); n++) {
        Centroid += pixels[n];
    }
    Centroid = (int)((float)Centroid / pixels.size());

    if (m_Segmenter->getSegmentId(Centroid) == m_Sid)
        return 1;
    else
        return -1;
}

int CSegment::getCentroid(INT2& Centroid)
{
    if (m_Pixels.size() == 0)
        return 0;

    std::vector<INT2> pixels;
    this->getPixels(pixels);

    Centroid = MAKE_INT2(0, 0);
    for (size_t n = 0; n < pixels.size(); n++) {
        INT2 p = pixels[n];
        Centroid.x += p.x;
        Centroid.y += p.y;
    }
    Centroid.x = (int)((float)Centroid.x / pixels.size());
    Centroid.y = (int)((float)Centroid.y / pixels.size());

    if (m_Segmenter->getSegmentId(Centroid.x, Centroid.y) == m_Sid)
        return 1;
    else
        return -1;
}

int CSegment::getCentroid(INT3& Centroid)
{
    if (m_Pixels.size() == 0)
        return 0;

    std::vector<INT3> pixels;
    this->getPixels(pixels);

    Centroid = MAKE_INT3(0, 0, 0);
    for (size_t n = 0; n < pixels.size(); n++) {
        INT3 p = pixels[n];
        Centroid.x += p.x;
        Centroid.y += p.y;
        Centroid.z += p.z;
    }
    Centroid.x = (int)((float)Centroid.x / pixels.size());
    Centroid.y = (int)((float)Centroid.y / pixels.size());
    Centroid.z = (int)((float)Centroid.z / pixels.size());

    if (m_Segmenter->getSegmentId(Centroid.x, Centroid.y, Centroid.z) == m_Sid)
        return 1;
    else
        return -1;
}

int CSegment::getContours(std::vector<std::vector<INT2> >& Contours)
{
    const int delta8[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}; // 8-way
    //const int delta4[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}; // 4-way
    const int dir8[8] = {7, 7, 1, 1, 3, 3, 5, 5};

    if (m_Pixels.size() == 0)
        return 0;

    // get bounding box
    int xmin, ymin, xmax, ymax;
    this->getBoundBox(xmin, ymin, xmax, ymax);
    int width = xmax - xmin + 1;
    int height = ymax - ymin + 1;

    std::vector<INT2> bpixels;
    this->getBoundPixels(bpixels);
    std::vector<int> btags(height * width, 0); // 0: no boundary, 1: boundary, 2: traced boundary
    for (size_t p = 0; p < bpixels.size(); p++)
        btags[(bpixels[p].y - ymin) * width + (bpixels[p].x - xmin)] = 1;

    // search for any untraced boundary
    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            if (btags[(y - ymin) * width + (x - xmin)] != 1)
                continue;

            std::vector<INT2> contour;
            contour.push_back(MAKE_INT2(x, y));
            int cx = x;  int cy = y;
            int sx = x;  int sy = y;
            int sx2 = x; int sy2 = y;
            int dir = 0;
            bool background = false;

            while (true) {
                for (int n = 0; n <= 8; n++, dir++) {
                    if (dir > 7)
                        dir = 0;

                    int px = cx + delta8[dir][0];
                    int py = cy + delta8[dir][1];
                    if (px < xmin || px > xmax || py < ymin || py > ymax) {
                        background = true;
                    }
                    else if (btags[(py - ymin) * width + (px - xmin)] == 0) {
                        //btags[(py - ymin) * width + (px - xmin)] = -1;
                        background = true;
                    }
                    else if (btags[(py - ymin) * width + (px - xmin)] == 1) { // != -1) {
                        contour.push_back(MAKE_INT2(px, py));

                        if (sx2 == sx && sy2 == sy && dir % 2 == 1) { // diagonal tracing, then store the first movement
                            sx2 = cx;
                            sy2 = cy;
                        }

                        // for next tracing
                        cx = px;
                        cy = py;
                        dir = dir8[dir];
                        break;
                    }
                }

                // this contour ends with the start point (one point contour also possible)
                if ((cx == sx && cy == sy) || (cx == sx2 && cy == sy2))
                    break;
            }

            // tag all traced contour pixels to 2
            for (size_t p = 0; p < contour.size(); p++)
                btags[(contour[p].y - ymin) * width + (contour[p].x - xmin)] = 2;

            Contours.push_back(contour);
        }
    }

    return (int)Contours.size();
}

int CSegment::getAdjacentSegments(std::vector<int>& ABSids)
{
    std::set<int> sidset;

    int height = m_Segmenter->getImage()->getHeight();
    int depth = m_Segmenter->getImage()->getDepth();

    if (height == 1 && depth == 1) {
        for (size_t n = 0; n < m_Pixels.size(); n++) {
            int x = m_Pixels[n];
            int sid1 = m_Segmenter->getSegmentId(x - 1);
            int sid2 = m_Segmenter->getSegmentId(x + 1);
            if (sid1 != -1 && sid1 != m_Sid)
                sidset.insert(sid1);
            if (sid2 != -1 && sid2 != m_Sid)
                sidset.insert(sid2);
        }
    }
    else if (depth == 1) {
        std::vector<INT2> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1);
            if (sid1 != -1 && sid1 != m_Sid)
                sidset.insert(sid1);
            if (sid2 != -1 && sid2 != m_Sid)
                sidset.insert(sid2);
            if (sid3 != -1 && sid3 != m_Sid)
                sidset.insert(sid3);
            if (sid4 != -1 && sid4 != m_Sid)
                sidset.insert(sid4);
        }
    }
    else {
        std::vector<INT3> pixels;
        this->getPixels(pixels);
        for (size_t n = 0; n < pixels.size(); n++) {
            int x = pixels[n].x;
            int y = pixels[n].y;
            int z = pixels[n].z;
            int sid1 = m_Segmenter->getSegmentId(x - 1, y, z);
            int sid2 = m_Segmenter->getSegmentId(x + 1, y, z);
            int sid3 = m_Segmenter->getSegmentId(x, y - 1, z);
            int sid4 = m_Segmenter->getSegmentId(x, y + 1, z);
            int sid5 = m_Segmenter->getSegmentId(x, y, z - 1);
            int sid6 = m_Segmenter->getSegmentId(x, y, z + 1);
            if (sid1 != -1 && sid1 != m_Sid)
                sidset.insert(sid1);
            if (sid2 != -1 && sid2 != m_Sid)
                sidset.insert(sid2);
            if (sid3 != -1 && sid3 != m_Sid)
                sidset.insert(sid3);
            if (sid4 != -1 && sid4 != m_Sid)
                sidset.insert(sid4);
            if (sid5 != -1 && sid5 != m_Sid)
                sidset.insert(sid5);
            if (sid6 != -1 && sid6 != m_Sid)
                sidset.insert(sid6);
        }
    }

    ABSids.clear();
    ABSids.insert(ABSids.end(), sidset.begin(), sidset.end());
    return (int)ABSids.size();
}

bool CSegment::isAdjacent(CSegment& Other)
{
    return (this->getNumBoundPixels(Other) > 0);
}




//------------------------------------------------------------------------------
// CSegmenter class
//------------------------------------------------------------------------------
CSegmenter::CSegmenter(CImage* Image)
{
    m_Image = Image;
    m_Labels = NULL;
    this->init();
}

CSegmenter::CSegmenter(const CSegmenter& Other)
{
    m_Image = Other.m_Image;
    m_Labels = NULL;
    this->assign(Other);
}

CSegmenter::~CSegmenter()
{
    if (m_Labels)
        delete [] m_Labels;
}

void CSegmenter::assign(const CSegmenter& Other)
{
    if (m_Labels)
        delete [] m_Labels;

    m_Image = Other.m_Image;
    m_Labels = new int[m_Image->getNumPixels()];
    memcpy(m_Labels, Other.m_Labels, sizeof(int) * m_Image->getNumPixels());
    m_Segments = Other.m_Segments;
}

CSegmenter& CSegmenter::operator = (const CSegmenter& Other)
{
    this->assign(Other);
    return *this;
}

bool CSegmenter::init()
{
    if (m_Image->getNumPixels() == 0)
        return false;

    if (m_Labels)
        delete [] m_Labels;
    m_Labels = new int[m_Image->getNumPixels()];
    for (int n = 0; n < m_Image->getNumPixels(); n++)
        m_Labels[n] = -1;
    m_Segments.clear();
    return true;
}

bool CSegmenter::loadFromLabels(int* PixelLabels, bool KeepLabel, int NConnectivity)
{
    this->init();

    if (KeepLabel) { // assuming that there's no empty labeled region
        int pixelcount = m_Image->getNumPixels();

        // copy labels
        memcpy(m_Labels, PixelLabels, sizeof(int) * pixelcount);

        // get # segments
        int maxlabel = 0;
        for (int p = 0; p < pixelcount; p++)
            maxlabel = __MAX(maxlabel, m_Labels[p]);
        int numsegments = maxlabel + 1;

        // collect each segment's pixels
        std::vector<INT3>* segmentpixels = new std::vector<INT3>[numsegments];
        for (int z = 0; z < m_Image->getDepth(); z++) {
            for (int y = 0; y < m_Image->getHeight(); y++) {
                for (int x = 0; x < m_Image->getWidth(); x++) {
                    int offset = z * m_Image->getWidth() * m_Image->getHeight() + y * m_Image->getWidth() + x;
                    int sid = m_Labels[offset];
                    INT3 pixel = MAKE_INT3(x, y, z);
                    segmentpixels[sid].push_back(pixel);
                }
            }
        }

        // setup segment list
        for (int sid = 0; sid < numsegments; sid++) {
            CSegment segment(this);
            segment.init(sid, (int)segmentpixels[sid].size(), &segmentpixels[sid][0]);
            m_Segments.push_back(segment);
        }

        delete [] segmentpixels;
    }
    else { // separate any labeled region if part of the region is disconnected
        // set 4- or 8-neighbors (in 2D), 6- or 26-neighbors (in 3D)
        INT3 neighbors[27];
        int numneighbors = getNeighborConnectivity(NConnectivity, neighbors);

        // allocate search queue
        INT3* searchlist = new INT3[m_Image->getNumPixels()];
        memset(searchlist, 0, sizeof(INT3) * m_Image->getNumPixels());

        // traverse labels
        int sid = 0;
        for (int z = 0; z < m_Image->getDepth(); z++) {
            for (int y = 0; y < m_Image->getHeight(); y++) {
                for (int x = 0; x < m_Image->getWidth(); x++) {

                    int offset = z * m_Image->getWidth() * m_Image->getHeight() + y * m_Image->getWidth() + x;
                    if (m_Labels[offset] != -1) // if already assigned
                        continue;

                    // set segment id
                    m_Labels[offset] = sid;

                    // read integer label
                    int label = PixelLabels[offset];

                    // initialize segment list
                    searchlist[0].x = x;
                    searchlist[0].y = y;
                    searchlist[0].z = z;
                    int searchlist_current = 0;
                    int searchlist_count = 1;

                    // do until all voxels in the list are evaluated
                    while (searchlist_current < searchlist_count) {

                        // check all neighbors
                        for (int n = 0; n < numneighbors; n++) {

                            int nx = searchlist[searchlist_current].x + neighbors[n].x;
                            int ny = searchlist[searchlist_current].y + neighbors[n].y;
                            int nz = searchlist[searchlist_current].z + neighbors[n].z;
                            if (nx < 0 || nx >= m_Image->getWidth() ||
                                ny < 0 || ny >= m_Image->getHeight() ||
                                nz < 0 || nz >= m_Image->getDepth())
                                continue;

                            int noffset = nz * m_Image->getWidth() * m_Image->getHeight() + ny * m_Image->getWidth() + nx;
                            if (m_Labels[noffset] != -1) // if already assigned
                                continue;
                            if (PixelLabels[noffset] != label)
                                continue;

                            // set label
                            m_Labels[noffset] = sid;
                            searchlist[searchlist_count].x = nx;
                            searchlist[searchlist_count].y = ny;
                            searchlist[searchlist_count].z = nz;
                            searchlist_count++;
                        }

                        searchlist_current++;
                    }

                    // set segment info
                    CSegment segment(this);
                    segment.init(sid, searchlist_count, searchlist);
                    m_Segments.push_back(segment);
                    sid++;
                }
            }
        }

        delete [] searchlist;
    }

    return true;
}

bool CSegmenter::loadFromLabelFile(std::string FilePath)
{
    std::ifstream file(FilePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;

    file.seekg(0, std::ios::end);
    int filesize = (int)file.tellg();

    file.seekg(0, std::ios::beg);
    std::vector<int> labels(m_Image->getNumPixels());
    if (filesize == m_Image->getNumPixels()) {
        uchar* clabels = new uchar[m_Image->getNumPixels()];
        file.read((char*)clabels, sizeof(uchar) * m_Image->getNumPixels());
        for (int n = 0; n < m_Image->getNumPixels(); n++)
            labels[n] = clabels[n];
        delete [] clabels;
    }
    else if (filesize == m_Image->getNumPixels() * 2) {
        ushort* clabels = new ushort[m_Image->getNumPixels()];
        file.read((char*)clabels, sizeof(ushort) * m_Image->getNumPixels());
        for (int n = 0; n < m_Image->getNumPixels(); n++)
            labels[n] = clabels[n];
        delete [] clabels;
    }
    else {
        file.read((char*)&labels[0], sizeof(int) * m_Image->getNumPixels());
    }
    file.close();

    return this->loadFromLabels(&labels[0], true);
}

bool CSegmenter::saveToLabelFile(std::string FilePath, bool Compact)
{
    std::ofstream file(FilePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;

    int maxsid = this->getMaxSegmentId();
    if (Compact && maxsid <= 0xff) {
        uchar* clabels = new uchar[m_Image->getNumPixels()];
        for (int n = 0; n < m_Image->getNumPixels(); n++)
            clabels[n] = (uchar)m_Labels[n];
        file.write((char*)clabels, sizeof(uchar) * m_Image->getNumPixels());
        delete [] clabels;
    }
    else if (Compact && maxsid <= 0xffff) {
        ushort* clabels = new ushort[m_Image->getNumPixels()];
        for (int n = 0; n < m_Image->getNumPixels(); n++)
            clabels[n] = (ushort)m_Labels[n];
        file.write((char*)clabels, sizeof(ushort) * m_Image->getNumPixels());
        delete [] clabels;
    }
    else {
        file.write((char*)m_Labels, sizeof(int) * m_Image->getNumPixels());
    }

    file.close();
    return true;
}

bool CSegmenter::saveToImageFile(std::string ImageFilePrefix, int ImageMode, bool Flip, UCHAR3 BoundColor)
{
    if (!m_Labels)
        return false;

    for (int slice = 0; slice < m_Image->getDepth(); slice++) {
        bool ret = this->saveToImageFile(ImageFilePrefix, slice, ImageMode, Flip, BoundColor);
        if (!ret)
            return false;
    }
    return true;
}

bool CSegmenter::saveToImageFile(std::string ImageFilePrefix, int SliceIndex, int ImageMode, bool Flip, UCHAR3 BoundColor)
{
    if (!m_Labels)
        return false;

#ifdef __LIB_OPENCV
    Mat image = Mat(m_Image->getHeight(), m_Image->getWidth(), CV_8UC3);
    if (!this->setToImage(image.data, SliceIndex, ImageMode, Flip, BoundColor))
        return false;

    char imagefile[512];
    if (m_Image->getDepth() <= 1)
        sprintf(imagefile, "%s.png", ImageFilePrefix.c_str());
    else
        sprintf(imagefile, "%s_slice%d.png", ImageFilePrefix.c_str(), SliceIndex);

    return imwrite(imagefile, image);
#else
    return false;
#endif
}

// ImageMode
// -1: original image
// 0: original image color + boundary (boundary color)
// 1: original image + boundary (segment color)
// 2: segment color + boundary (boundary color)
// 3: segment color only
// 4: original image + segment color (blended)
bool CSegmenter::setToImage(uchar* Image, int SliceIndex, int ImageMode, bool Flip, UCHAR3 BoundColor)
{
    const UCHAR3 __color[18] = {{0, 0, 0}, {32, 32, 32}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 64, 0}, {255, 0, 64}, {64, 0, 255}, {0, 64, 255}, {64, 255, 0}, {0, 255, 64}, {255, 128, 128}, {128, 64, 32}, {64, 128, 32}, {64, 32, 128}};
    
    if (!m_Labels)
        return false;

    if (m_Image->getDepth() > 1 && (SliceIndex < 0 || SliceIndex >= m_Image->getDepth())) {
        printf("Invalid slice index");
        return false;
    }

    for (int y = 0; y < m_Image->getHeight(); y++) {
        for (int x = 0; x < m_Image->getWidth(); x++) {

            int sid;
            bool bound;
            PixelType* pixel;
            if (m_Image->getDepth() == 1) {
                sid = this->getSegmentId(x, y);
                bound = this->isBoundPixel(x, y);
                pixel = m_Image->getPixel(x, y);
            }
            else {
                sid = this->getSegmentId(x, y, SliceIndex);
                bound = this->isBoundPixelSlice(x, y, SliceIndex);
                pixel = m_Image->getPixel(x, y, SliceIndex);
            }

			int cind = (sid + 1) % 18;
			UCHAR3 segcolor = MAKE_UCHAR3(__color[cind].x, __color[cind].y, __color[cind].z);
            UCHAR3 orgcolor;
            if (m_Image->getNumChannels() >= 3)
                orgcolor = MAKE_UCHAR3((uchar)(pixel[0] * 255), (uchar)(pixel[1] * 255), (uchar)(pixel[2] * 255));
            else
                orgcolor = MAKE_UCHAR3((uchar)(pixel[0] * 255), (uchar)(pixel[0] * 255), (uchar)(pixel[0] * 255));

            UCHAR3 color;
            if (ImageMode == -1) {
                color = orgcolor;
            }
            else if (ImageMode == 0) {
                color = ((bound) ? BoundColor : orgcolor);
            }
            else if (ImageMode == 1) {
                color = ((bound) ? segcolor : orgcolor);
            }
            else if (ImageMode == 2) {
                color = ((bound) ? BoundColor : segcolor);
            }
            else if (ImageMode == 3) {
                color = segcolor;
            }
            else if (ImageMode == 4) {
                color.x = (uchar)__MIN(orgcolor.x * 0.6 + segcolor.x * 0.4, 255);
                color.y = (uchar)__MIN(orgcolor.y * 0.6 + segcolor.y * 0.4, 255);
                color.z = (uchar)__MIN(orgcolor.z * 0.6 + segcolor.z * 0.4, 255);
            }

            int y2 = (Flip) ? (m_Image->getHeight() - 1 - y) : y;
            Image[(y2 * m_Image->getWidth() + x) * 3 + 0] = color.x;
            Image[(y2 * m_Image->getWidth() + x) * 3 + 1] = color.y;
            Image[(y2 * m_Image->getWidth() + x) * 3 + 2] = color.z;
        }
    }

    return true;
}

int CSegmenter::getNeighborConnectivity(int NConnectivity, INT3 neighbors[27])
{
    // initialize neighbor info
    memset(neighbors, 0, sizeof(INT3) * 27);

    // set 4- or 8-neighbors (in 2D), 6- or 26-neighbors (in 3D)
    int numneighbors;
    if (m_Image->getDepth() <= 1) {
        if (NConnectivity == 2) {
            numneighbors = 0;
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    if (x == 0 && y == 0)
                        continue;
                    neighbors[numneighbors].x = x;
                    neighbors[numneighbors].y = y;
                    numneighbors++;
                }
            }
        }
        else {
            numneighbors = 4;
            neighbors[0].x = -1; neighbors[0].y = 0;
            neighbors[1].x = +1; neighbors[1].y = 0;
            neighbors[2].x = 0;  neighbors[2].y = -1;
            neighbors[3].x = 0;  neighbors[3].y = +1;
        }
    }
    else {
        if (NConnectivity == 2) {
            numneighbors = 0;
            for (int z = -1; z <= 1; z++) {
                for (int y = -1; y <= 1; y++) {
                    for (int x = -1; x <= 1; x++) {
                        if (x == 0 && y == 0 && z == 0)
                            continue;
                        neighbors[numneighbors].x = x;
                        neighbors[numneighbors].y = y;
                        neighbors[numneighbors].z = z;
                        numneighbors++;
                    }
                }
            }
        }
        else {
            numneighbors = 6;
            neighbors[0].x = -1; neighbors[0].y = 0;  neighbors[0].z = 0;
            neighbors[1].x = +1; neighbors[1].y = 0;  neighbors[1].z = 0;
            neighbors[2].x = 0;  neighbors[2].y = -1; neighbors[2].z = 0;
            neighbors[3].x = 0;  neighbors[3].y = +1; neighbors[3].z = 0;
            neighbors[4].x = 0;  neighbors[4].y = 0;  neighbors[4].z = -1;
            neighbors[5].x = 0;  neighbors[5].y = 0;  neighbors[5].z = +1;
        }
    }

    return numneighbors;
}

bool CSegmenter::segmentSimpleRegionGrowing(int NConnectivity, int MinSegment, int MaxSegment, double Threshold)
{
    this->init();

    // set 4- or 8-neighbors (in 2D), 6- or 26-neighbors (in 3D)
    INT3 neighbors[27];
    int numneighbors = getNeighborConnectivity(NConnectivity, neighbors);

    // initialize search queue
    int count = m_Image->getNumPixels();
    INT3* searchlist = new INT3[count];
    memset(searchlist, 0, sizeof(INT3) * count);

    // for every pixel
    int sid = 0;
    for (int z = 0; z < m_Image->getDepth(); z++) {
        for (int y = 0; y < m_Image->getHeight(); y++) {
            for (int x = 0; x < m_Image->getWidth(); x++) {

                int offset = z * m_Image->getWidth() * m_Image->getHeight() + y * m_Image->getWidth() + x;
                if (m_Labels[offset] != -1) // if already assigned
                    continue;

                // set segment id
                m_Labels[offset] = sid;

                // read pixel
                std::vector<double> vsum(4);
                float* pvalue = m_Image->getPixel(x, y, z);
                for (int c = 0; c < m_Image->getNumChannels(); c++)
                    vsum[c] = pvalue[c];

                // initialize segment list
                searchlist[0].x = x;
                searchlist[0].y = y;
                searchlist[0].z = z;
                int searchlist_current = 0;
                int searchlist_count = 1;

                // do until all voxels in the list are evaluated
                while (searchlist_current < searchlist_count) {

                    // check all neighbors
                    for (int n = 0; n < numneighbors; n++) {
                        int nx = searchlist[searchlist_current].x + neighbors[n].x;
                        int ny = searchlist[searchlist_current].y + neighbors[n].y;
                        int nz = searchlist[searchlist_current].z + neighbors[n].z;
                        if (nx < 0 || nx >= m_Image->getWidth() ||
                            ny < 0 || ny >= m_Image->getHeight() ||
                            nz < 0 || nz >= m_Image->getDepth())
                            continue;
                        if (MaxSegment > 0) {
                            if (abs(nx - x) > MaxSegment || abs(ny - y) > MaxSegment || abs(nz - z) > MaxSegment)
                                continue;
                        }

                        int noffset = nz * m_Image->getWidth() * m_Image->getHeight() + ny * m_Image->getWidth() + nx;
                        if (m_Labels[noffset] != -1) // if already assigned
                            continue;

                        // adjust threshold
                        double thres = Threshold;
                        if (searchlist_count < MinSegment)
                            thres = (2 * Threshold) - (Threshold * searchlist_count / MinSegment);

                        // check whether or not difference between the current pixel/voxel value and the mean value is above threshold
                        bool nextneighbor = false;
                        float* npvalue = m_Image->getPixel(nx, ny, nz);
                        for (int c = 0; c < m_Image->getNumChannels(); c++) {
                            double vmean = (double)vsum[c] / searchlist_count;
                            if (fabs(npvalue[c] - vmean) > thres) {
                                nextneighbor = true;
                                break;
                            }
                        }
                        if (nextneighbor)
                            continue;

                        // update sum
                        for (int c = 0; c < m_Image->getNumChannels(); c++)
                            vsum[c] += npvalue[c];

                        // set label
                        m_Labels[noffset] = sid;
                        searchlist[searchlist_count].x = nx;
                        searchlist[searchlist_count].y = ny;
                        searchlist[searchlist_count].z = nz;
                        searchlist_count++;
                    }

                    searchlist_current++;
                }

                // set segment info
                CSegment segment(this);
                segment.init(sid, searchlist_count, searchlist);
                m_Segments.push_back(segment);
                sid++;
            }
        }
    }

    delete [] searchlist;
    return true;
}

bool CSegmenter::pruneBySegmentId(std::vector<int>& Sids)
{
    if (m_Segments.size() == 0)
        return false;

    std::vector<int> segment_labels(m_Segments.size());
    for (size_t sid = 0; sid < m_Segments.size(); sid++)
        segment_labels[sid] = sid;

    for (size_t s = 0; s < Sids.size(); s++) {
        int sid = Sids[s];
        CSegment* segment = &m_Segments[sid];

        std::vector<int> asids;
        segment->getAdjacentSegments(asids);

        int asid_max = asids[0];
        int asize_max = 0;
        for (size_t n = 0; n < asids.size(); n++) {
            int asid = asids[n];
            CSegment* asegment = &m_Segments[asid];
            int asize = asegment->getNumPixels();
            if (asize_max < asize) {
                asize_max = asize;
                asid_max = asid;
            }
        }
        segment_labels[sid] = asid_max;
    }

    std::vector<int> labels(m_Image->getNumPixels());
    for (size_t sid = 0; sid < m_Segments.size(); sid++) {
        CSegment* segment = &m_Segments[sid];
        int seglabel = segment_labels[sid];

        std::vector<int> segpixels;
        segment->getPixels(segpixels);
        for (size_t p = 0; p < segpixels.size(); p++) {
            int pindex = segpixels[p];
            labels[pindex] = seglabel;
        }
    }
    this->loadFromLabels(&labels[0], false);
    return true;
}

bool CSegmenter::pruneBySegmentSize(int SegmentSize)
{
    if (m_Segments.size() == 0)
        return false;

    std::vector<int> segment_labels(m_Segments.size());
    for (size_t sid = 0; sid < m_Segments.size(); sid++) {
        CSegment* segment = &m_Segments[sid];
        if (segment->getNumPixels() <= SegmentSize) {
            std::vector<int> asids;
            segment->getAdjacentSegments(asids);

            int asid_max = asids[0];
            int asize_max = 0;
            for (size_t n = 0; n < asids.size(); n++) {
                int asid = asids[n];
                CSegment* asegment = &m_Segments[asid];
                int asize = asegment->getNumPixels();
                if (asize_max < asize) {
                    asize_max = asize;
                    asid_max = asid;
                }
            }
            segment_labels[sid] = asid_max;
        }
        else {
            segment_labels[sid] = sid;
        }
    }

    std::vector<int> labels(m_Image->getNumPixels());
    for (size_t sid = 0; sid < m_Segments.size(); sid++) {
        CSegment* segment = &m_Segments[sid];
        int seglabel = segment_labels[sid];

        std::vector<int> segpixels;
        segment->getPixels(segpixels);
        for (size_t p = 0; p < segpixels.size(); p++) {
            int pindex = segpixels[p];
            labels[pindex] = seglabel;
        }
    }
    this->loadFromLabels(&labels[0], false);
    return true;
}

bool CSegmenter::isBoundPixel(int x)
{
    int sid  = this->getSegmentId(x);
    int sid1 = this->getSegmentId(x - 1);
    int sid2 = this->getSegmentId(x + 1);
    if ((sid1 != -1 && sid1 != sid) || (sid2 != -1 && sid2 != sid))
        return true;
    else
        return false;
}

bool CSegmenter::isBoundPixel(int x, int y)
{
    int sid  = this->getSegmentId(x, y);
    int sid1 = this->getSegmentId(x - 1, y);
    int sid2 = this->getSegmentId(x + 1, y);
    int sid3 = this->getSegmentId(x, y - 1);
    int sid4 = this->getSegmentId(x, y + 1);
    if ((sid1 != -1 && sid1 != sid) || (sid2 != -1 && sid2 != sid) ||
        (sid3 != -1 && sid3 != sid) || (sid4 != -1 && sid4 != sid))
        return true;
    else
        return false;
}

bool CSegmenter::isBoundPixel(int x, int y, int z)
{
    int sid  = this->getSegmentId(x, y, z);
    int sid1 = this->getSegmentId(x - 1, y, z);
    int sid2 = this->getSegmentId(x + 1, y, z);
    int sid3 = this->getSegmentId(x, y - 1, z);
    int sid4 = this->getSegmentId(x, y + 1, z);
    int sid5 = this->getSegmentId(x, y, z - 1);
    int sid6 = this->getSegmentId(x, y, z + 1);
    if ((sid1 != -1 && sid1 != sid) || (sid2 != -1 && sid2 != sid) ||
        (sid3 != -1 && sid3 != sid) || (sid4 != -1 && sid4 != sid) ||
        (sid5 != -1 && sid5 != sid) || (sid6 != -1 && sid6 != sid))
        return true;
    else
        return false;
}

bool CSegmenter::isBoundPixel(int x, std::vector<int>& Sids)
{
    int sid[2];

    sid[0] = this->getSegmentId(x - 1);
    sid[1] = this->getSegmentId(x + 1);

    for (int n = 0; n < 2; n++) {
        if (sid[n] == -1)
            continue;

        bool inside = false;
        for (size_t s = 0; s < Sids.size(); s++) {
            if (sid[n] == Sids[s]) {
                inside = true;
                break;
            }
        }
        if (!inside)
            return true;
    }

    return false;
}

bool CSegmenter::isBoundPixel(int x, int y, std::vector<int>& Sids)
{
    int sid[4];

    sid[0] = this->getSegmentId(x - 1, y);
    sid[1] = this->getSegmentId(x + 1, y);
    sid[2] = this->getSegmentId(x, y - 1);
    sid[3] = this->getSegmentId(x, y + 1);

    for (int n = 0; n < 4; n++) {
        if (sid[n] == -1)
            continue;

        bool inside = false;
        for (size_t s = 0; s < Sids.size(); s++) {
            if (sid[n] == Sids[s]) {
                inside = true;
                break;
            }
        }
        if (!inside)
            return true;
    }

    return false;
}

bool CSegmenter::isBoundPixel(int x, int y, int z, std::vector<int>& Sids)
{
    int sid[6];

    sid[0] = this->getSegmentId(x - 1, y, z);
    sid[1] = this->getSegmentId(x + 1, y, z);
    sid[2] = this->getSegmentId(x, y - 1, z);
    sid[3] = this->getSegmentId(x, y + 1, z);
    sid[4] = this->getSegmentId(x, y, z - 1);
    sid[5] = this->getSegmentId(x, y, z + 1);

    for (int n = 0; n < 6; n++) {
        if (sid[n] == -1)
            continue;

        bool inside = false;
        for (size_t s = 0; s < Sids.size(); s++) {
            if (sid[n] == Sids[s]) {
                inside = true;
                break;
            }
        }
        if (!inside)
            return true;
    }

    return false;
}

bool CSegmenter::isBoundPixelSlice(int x, int y, int z)
{
    int sid  = this->getSegmentId(x, y, z);
    int sid1 = this->getSegmentId(x - 1, y, z);
    int sid2 = this->getSegmentId(x + 1, y, z);
    int sid3 = this->getSegmentId(x, y - 1, z);
    int sid4 = this->getSegmentId(x, y + 1, z);
    if ((sid1 != -1 && sid1 != sid) || (sid2 != -1 && sid2 != sid) ||
        (sid3 != -1 && sid3 != sid) || (sid4 != -1 && sid4 != sid))
        return true;
    else
        return false;
}

bool CSegmenter::isBoundPixelSlice(int x, int y, int z, std::vector<int>& Sids)
{
    int sid[4];

    sid[0] = this->getSegmentId(x - 1, y, z);
    sid[1] = this->getSegmentId(x + 1, y, z);
    sid[2] = this->getSegmentId(x, y - 1, z);
    sid[3] = this->getSegmentId(x, y + 1, z);

    for (int n = 0; n < 4; n++) {
        if (sid[n] == -1)
            continue;

        bool inside = false;
        for (size_t s = 0; s < Sids.size(); s++) {
            if (sid[n] == Sids[s]) {
                inside = true;
                break;
            }
        }
        if (!inside)
            return true;
    }

    return false;
}

int CSegmenter::getSegmentId(int x)
{
    if (x < 0 || x >= m_Image->getWidth())
        return -1;
    return m_Labels[x];
}

int CSegmenter::getSegmentId(int x, int y)
{
    if (x < 0 || x >= m_Image->getWidth() ||
        y < 0 || y >= m_Image->getHeight())
        return -1;
    return m_Labels[y * m_Image->getWidth() + x];
}

int CSegmenter::getSegmentId(int x, int y, int z)
{
    if (x < 0 || x >= m_Image->getWidth() ||
        y < 0 || y >= m_Image->getHeight() ||
        z < 0 || z >= m_Image->getDepth())
        return -1;
    return m_Labels[z * m_Image->getWidth() * m_Image->getHeight() + y * m_Image->getWidth() + x];
}

int CSegmenter::getMaxSegmentId()
{
    int sid = 0;
    for (int n = 0; n < m_Image->getNumPixels(); n++)
        sid = __MAX(sid, m_Labels[n]);

    return sid;
}

int CSegmenter::getNumSegments()
{
    return (int)m_Segments.size();
}

int* CSegmenter::getLabels()
{
    return m_Labels;
}

CSegment* CSegmenter::getSegment(int Sid)
{
    assert(Sid >= 0 && Sid < (int)m_Segments.size());
    return &m_Segments[Sid];
}

CSegment* CSegmenter::getSegment(int x, int y)
{
    int sid = this->getSegmentId(x, y);
    assert(sid >= 0 && sid < (int)m_Segments.size());
    return &m_Segments[sid];
}

CSegment* CSegmenter::getSegment(int x, int y, int z)
{
    int sid = this->getSegmentId(x, y, z);
    assert(sid >= 0 && sid < (int)m_Segments.size());
    return &m_Segments[sid];
}

std::vector<CSegment>& CSegmenter::getSegments()
{
    return m_Segments;
}

CImage* CSegmenter::getImage()
{
    return m_Image;
}
