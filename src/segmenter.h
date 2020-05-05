//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Segmentation class (.h, .cpp)
//*****************************************************************************/

#ifndef __SEGMENTER_H
#define __SEGMENTER_H

#include "datatype.h"
#include <vector>
#include <set>
#include <queue>
#include <string>



typedef float PixelType;


class CImage
{
public:
	CImage();
	CImage(const CImage& Other);
	CImage(int Width, int Height, int Depth, int Channel, PixelType* Pixels, bool AllocateImage);
	~CImage();
	
	void init(const CImage& Other);
	void init(int Width, int Height, int Depth, int Channel, PixelType* Pixels, bool AllocateImage);
	void free();
	CImage& operator = (const CImage& Other);
	
	int getWidth();
	int getHeight();
	int getDepth();
	int getNumChannels();
	int getNumPixels();
	INT4 getDimension();
	
	PixelType* getPixel(int x);
	PixelType* getPixel(int x, int y);
	PixelType* getPixel(int x, int y, int z);
	PixelType* getPixels();

	void setPixel(int x, PixelType* value);
	void setPixel(int x, int y, PixelType* value);
	void setPixel(int x, int y, int z, PixelType* value);
	
	bool getHistogram(int NumBins, int* Bins);

private:
	INT4			m_Dim;
	bool			m_Allocated;
	PixelType*		m_Pixels;
};



/// segment class
class CSegmenter;
class CSegment
{
public:
	CSegment(CSegmenter* Segmenter);
	CSegment(const CSegment& Other);
	CSegment& operator = (const CSegment& Other);

	void setSid(int Sid) { m_Sid = Sid; }
	void setTag(int Tag) { m_Tag = Tag; }
	int  getSid() { return m_Sid; }
	int  getTag() { return m_Tag; }
	CSegmenter* getSegmenter() { return m_Segmenter; }
	bool operator < (const CSegment& Other) const { return (m_Sid < Other.m_Sid); }

	void init(int Sid, int NumPixels, int* Pixels);
	void init(int Sid, int NumPixels, INT2* Pixels);
	void init(int Sid, int NumPixels, INT3* Pixels);

	int getNumPixels();
	int getPixels(std::vector<int>& Pixels);
	int getPixels(std::vector<INT2>& Pixels);
	int getPixels(std::vector<INT3>& Pixels);

	void getBoundBox(int& XMin, int& XMax);
	void getBoundBox(int& XMin, int& YMin, int& XMax, int& YMax);
	void getBoundBox(int& XMin, int& YMin, int& ZMin, int& XMax, int& YMax, int& ZMax);

	int getNumBoundPixels();
	int getBoundPixels(std::vector<int>& BoundPixels);
	int getBoundPixels(std::vector<INT2>& BoundPixels);
	int getBoundPixels(std::vector<INT3>& BoundPixels);
	int getNumBoundPixels(CSegment& Other);
	int getBoundPixels(CSegment& Other, std::vector<int>& BoundPixels);
	int getBoundPixels(CSegment& Other, std::vector<INT2>& BoundPixels);
	int getBoundPixels(CSegment& Other, std::vector<INT3>& BoundPixels);

	int getCentroid(int& Centroid);
	int getCentroid(INT2& Centroid);
	int getCentroid(INT3& Centroid);
	int getContours(std::vector<std::vector<INT2> >& Contours);

	int  getAdjacentSegments(std::vector<int>& ABSids);
	bool isAdjacent(CSegment& Other);

private:
	int 					m_Sid; 			// segment id
	int         			m_Tag;          // tag for internal use
	CSegmenter*				m_Segmenter;	// segmentation (input, labels, etc)

	int      				m_BoundMin; 	// min bounding box (indices)
	int      				m_BoundMax;		// max bounding box (indices)
	std::vector<int> 		m_Pixels;		// pixels/voxels
	
};



class CSegmenter
{
public:
	CSegmenter(CImage* Image);
	CSegmenter(const CSegmenter& Other);
	~CSegmenter();
	
	void assign(const CSegmenter& Other);
	CSegmenter& operator = (const CSegmenter& Other);
	
	bool init();
	bool loadFromLabels(int* PixelLabels, bool KeepLabel, int NConnectivity=1);
	bool loadFromLabelFile(std::string FilePath);
	bool saveToLabelFile(std::string FilePath, bool Compact=false);

	bool saveToImageFile(std::string ImageFilePrefix, int ImageMode=0, bool Flip=false, UCHAR3 BoundColor=MAKE_UCHAR3(0,255,255));
	bool saveToImageFile(std::string ImageFilePrefix, int SliceIndex, int ImageMode=0, bool Flip=false, UCHAR3 BoundColor=MAKE_UCHAR3(0,255,255));
	bool setToImage(uchar* Image, int SliceIndex, int ImageMode=0, bool Flip=false, UCHAR3 BoundColor=MAKE_UCHAR3(0,255,255));

	bool segmentSimpleRegionGrowing(int NConnectivity=1, int MinSegment=120, int MaxSegment=0, double Threshold=0.05);

	bool pruneBySegmentId(std::vector<int>& Sids);
	bool pruneBySegmentSize(int SegmentSize);
	
	bool isBoundPixel(int x);
	bool isBoundPixel(int x, int y);
	bool isBoundPixel(int x, int y, int z);
	bool isBoundPixel(int x, std::vector<int>& Sids);
	bool isBoundPixel(int x, int y, std::vector<int>& Sids);
	bool isBoundPixel(int x, int y, int z, std::vector<int>& Sids);
	bool isBoundPixelSlice(int x, int y, int z);
	bool isBoundPixelSlice(int x, int y, int z, std::vector<int>& Sids);

	int  getSegmentId(int x);
	int  getSegmentId(int x, int y);
	int  getSegmentId(int x, int y, int z);
	int  getMaxSegmentId();
	int  getNumSegments();
	int* getLabels();
	CSegment* getSegment(int Sid);
	CSegment* getSegment(int x, int y);
	CSegment* getSegment(int x, int y, int z);
	std::vector<CSegment>& getSegments();
	CImage* getImage();

private:
	int getNeighborConnectivity(int NConnectivity, INT3 neighbors[27]);

private:
	// image data (input)
	CImage*                 m_Image;
	
	// segmentation data (output)
	std::vector<CSegment>   m_Segments;
	int*                    m_Labels;
	
};



#endif // __SEGMENTER__H

