//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Basic data type, macro, inline functions, originally from geoinfo.h
//*****************************************************************************/

#ifndef __DATATYPE_H
#define __DATATYPE_H

#include <cmath>
#include <cstdio>
#include <string>

#ifndef NULL
#define NULL 0
#endif

#define __PI                                    3.14159265359
#define __PI_SQRT                               1.77245385091

#define __MIN(a, b)                             ((a < b) ? a : b)
#define __MAX(a, b)                             ((a > b) ? a : b)

#define __RAD2DEG(rad)                          ((double)rad * 180 / __PI)
#define __DEG2RAD(deg)                          ((double)deg * __PI / 180)

#define __LOG(a, base)                          (log(double(a)) / log(double(base)))
#define __LOG2(a)                               (log(double(a)) / log(2.0))

#define __OFFSET_2D(x, y, width)                (y * width + x)
#define __OFFSET_3D(x, y, z, width, height)     (z * width * height + y * width + x)

#define __LENGTH_2D(x1, y1, x2, y2)             (sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)))
#define __LENGTH_3D(x1, y1, z1, x2, y2, z2)     (sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2)))
#define __LENGTH_3D_2(x1, y1, z1, x2, y2, z2)   ((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2))

#define __ORIENTATION(x1, y1, x2, y2, x3, y3)   ((x1 - x3) * (y2 - y3) - (y1 - y3) * (x2 - x3))
#define __SLOPE(x1, y1, x2, y2)					((y2 - y1) / (x2 - x1))
#define __COS2TH(a, b, c)						((a * a + c * c - b * b) / (2.0 * a * c))

#ifdef WIN32
#define __DIR_DELIMITER        "\\"
#else
#define __DIR_DELIMITER        "/"
#endif


typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;

typedef struct _CHAR2 { char x; char y; } CHAR2;
typedef struct _CHAR3 { char x; char y; char z; } CHAR3;
typedef struct _CHAR4 { char x; char y; char z; char w; } CHAR4;

typedef struct _UCHAR2 { uchar x; uchar y; } UCHAR2;
typedef struct _UCHAR3 { uchar x; uchar y; uchar z; } UCHAR3;
typedef struct _UCHAR4 { uchar x; uchar y; uchar z; uchar w; } UCHAR4;

typedef struct _SHORT2 { short x; short y; } SHORT2;
typedef struct _SHORT3 { short x; short y; short z; } SHORT3;
typedef struct _SHORT4 { short x; short y; short z; short w; } SHORT4;

typedef struct _USHORT2 { ushort x; ushort y; } USHORT2;
typedef struct _USHORT3 { ushort x; ushort y; ushort z; } USHORT3;
typedef struct _USHORT4 { ushort x; ushort y; ushort z; ushort w; } USHORT4;


typedef struct _INT2 { int x; int y;
    bool operator < ( const _INT2& other) const
    {
        return (this->y == other.y) ? (this->x < other.x) : (this->y < other.y);
    }
} INT2;
typedef struct _INT3 { int x; int y; int z;
    bool operator < ( const _INT3& other) const
    {
        if (this->z == other.z)
            return (this->y == other.y) ? (this->x < other.x) : (this->y < other.y);
        else return (this->z < other.z);
    }
} INT3;
typedef struct _INT4 { int x; int y; int z; int w; } INT4;

typedef struct _FLOAT2 { float x; float y; } FLOAT2;
typedef struct _FLOAT3 { float x; float y; float z; } FLOAT3;
typedef struct _FLOAT4 { float x; float y; float z; float w; } FLOAT4;

typedef struct _DOUBLE2 { double x; double y; } DOUBLE2;
typedef struct _DOUBLE3 { double x; double y; double z; } DOUBLE3;
typedef struct _DOUBLE4 { double x; double y; double z; double w; } DOUBLE4;

inline UCHAR2 MAKE_UCHAR2(uchar x, uchar y) { UCHAR2 value; value.x = x, value.y = y; return value; }
inline UCHAR3 MAKE_UCHAR3(uchar x, uchar y, uchar z) { UCHAR3 value; value.x = x, value.y = y, value.z = z; return value; }
inline UCHAR4 MAKE_UCHAR4(uchar x, uchar y, uchar z, uchar w) { UCHAR4 value; value.x = x, value.y = y, value.z = z, value.w = w; return value; }

inline USHORT2 MAKE_USHORT2(ushort x, ushort y) { USHORT2 value; value.x = x, value.y = y; return value; }
inline USHORT3 MAKE_USHORT3(ushort x, ushort y, ushort z) { USHORT3 value; value.x = x, value.y = y, value.z = z; return value; }
inline USHORT4 MAKE_USHORT4(ushort x, ushort y, ushort z, ushort w) { USHORT4 value; value.x = x, value.y = y, value.z = z, value.w = w; return value; }

inline INT2 MAKE_INT2(int x, int y) { INT2 value; value.x = x, value.y = y; return value; }
inline INT3 MAKE_INT3(int x, int y, int z) { INT3 value; value.x = x, value.y = y, value.z = z; return value; }
inline INT4 MAKE_INT4(int x, int y, int z, int w) { INT4 value; value.x = x, value.y = y, value.z = z, value.w = w; return value; }

inline FLOAT2 MAKE_FLOAT2(float x, float y) { FLOAT2 value; value.x = x, value.y = y; return value; }
inline FLOAT3 MAKE_FLOAT3(float x, float y, float z) { FLOAT3 value; value.x = x, value.y = y, value.z = z; return value; }
inline FLOAT4 MAKE_FLOAT4(float x, float y, float z, float w) { FLOAT4 value; value.x = x, value.y = y, value.z = z, value.w = w; return value; }

inline DOUBLE2 MAKE_DOUBLE2(double x, double y) { DOUBLE2 value; value.x = x, value.y = y; return value; }
inline DOUBLE3 MAKE_DOUBLE3(double x, double y, double z) { DOUBLE3 value; value.x = x, value.y = y, value.z = z; return value; }
inline DOUBLE4 MAKE_DOUBLE4(double x, double y, double z, float w) { DOUBLE4 value; value.x = x, value.y = y, value.z = z, value.w = w; return value; }

inline std::string intToString(int number) { char str[256]; sprintf(str, "%d", number); return std::string(str); }
inline std::string floatToString(double number) { char str[256]; sprintf(str, "%.3lf", number); return std::string(str); }




#endif // __DATATYPE_H
