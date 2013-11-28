//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
 //   2. Altered source versions must be plainly marked as such, and must not be
 //   misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.
// 
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Vector operations
//
//-----------------------------------------------------------------------------

#include <math.h>
#include "mathlib.h"

const kexVec3 kexVec3::vecRight(1, 0, 0);
const kexVec3 kexVec3::vecUp(0, 1, 0);
const kexVec3 kexVec3::vecForward(0, 0, 1);

//
// kexVec3::kexVec3
//

kexVec3::kexVec3(void) {
    Clear();
}

//
// kexVec3::kexVec3
//

kexVec3::kexVec3(const float x, const float y, const float z) {
    Set(x, y, z);
}

//
// kexVec3::Set
//

void kexVec3::Set(const float x, const float y, const float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

//
// kexVec3::Clear
//

void kexVec3::Clear(void) {
    x = y = z = 0.0f;
}

//
// kexVec3::Dot
//

float kexVec3::Dot(const kexVec3 &vec) const {
    return (x * vec.x + y * vec.y + z * vec.z);
}

//
// kexVec3::Dot
//

float kexVec3::Dot(const kexVec3 &vec1, const kexVec3 &vec2) {
    return (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z);
}

//
// kexVec3::Cross
//

kexVec3 kexVec3::Cross(const kexVec3 &vec) const {
    return kexVec3(
        vec.z * y - z * vec.y,
        vec.x * z - x * vec.z,
        x * vec.y - vec.x * y
    );
}

//
// kexVec3::Cross
//

kexVec3 &kexVec3::Cross(const kexVec3 &vec1, const kexVec3 &vec2) {
    x = vec2.z * vec1.y - vec1.z * vec2.y;
    y = vec2.x * vec1.z - vec1.x * vec2.z;
    z = vec1.x * vec2.y - vec2.x * vec1.y;

    return *this;
}

//
// kexVec3::UnitSq
//

float kexVec3::UnitSq(void) const {
    return x * x + y * y + z * z;
}

//
// kexVec3::Unit
//

float kexVec3::Unit(void) const {
    return kexMath::Sqrt(UnitSq());
}

//
// kexVec3::DistanceSq
//

float kexVec3::DistanceSq(const kexVec3 &vec) const {
    return (
        (x - vec.x) * (x - vec.x) +
        (y - vec.y) * (y - vec.y) +
        (z - vec.z) * (z - vec.z)
    );
}

//
// kexVec3::Distance
//

float kexVec3::Distance(const kexVec3 &vec) const {
    return kexMath::Sqrt(DistanceSq(vec));
}

//
// kexVec3::Normalize
//

kexVec3 &kexVec3::Normalize(void) {
    float d = Unit();
    if(d != 0.0f) {
        d = 1.0f / d;
        *this *= d;
    }
    return *this;
}

//
// kexVec3::PointAt
//

kexVec3 kexVec3::PointAt(kexVec3 &location) const {
    float an1 = (float)atan2(location.x - x, location.y - y);
    float an2 = (float)atan2(location.Distance(*this), location.z - z);

    return kexVec3(
        kexMath::Sin(an1),
        kexMath::Cos(an2),
        kexMath::Cos(an1)
    );
}

//
// kexVec3::Lerp
//

kexVec3 kexVec3::Lerp(const kexVec3 &next, float movement) const {
    return (next - *this) * movement + *this;
}

//
// kexVec3::Lerp
//

kexVec3 &kexVec3::Lerp(const kexVec3 &start, const kexVec3 &next, float movement) {
    *this = (next - start) * movement + start;
    return *this;
}

//
// kexVec3::ToQuat
//

kexQuat kexVec3::ToQuat(void) {
    float d = Unit();

    if(d == 0.0f)
        return kexQuat();

    kexVec3 scv = *this * (1.0f / d);
    float angle = kexMath::ACos(scv.y);

    return kexQuat(angle, vecForward.Cross(scv).Normalize());
}

//
// kexVec3::ToYaw
//

float kexVec3::ToYaw(void) const {
    float d = x * x + y * y;

    if(d == 0.0f)
        return 0.0f;

    float an = -(y / kexMath::Sqrt(d));

    if(an >  1.0f) an =  1.0f;
    if(an < -1.0f) an = -1.0f;

    if(-x <= 0.0f) {
        return -kexMath::ACos(an);
    }

    return kexMath::ACos(an);
}

//
// kexVec3::ToPitch
//

float kexVec3::ToPitch(void) const {
    float d = UnitSq();
    
    if(d == 0.0f)
        return 0.0f;
        
    return kexMath::ACos(z / kexMath::Sqrt(d));
}

//
// kexVec3::ToString
//

kexStr kexVec3::ToString(void) const {
    kexStr str;
    str = str + x + " " + y + " " + z;
    return str;
}

//
// kexVec3::ToFloatPtr
//

float *kexVec3::ToFloatPtr(void) {
    return reinterpret_cast<float*>(&x);
}

//
// kexVec3::ScreenProject
//

kexVec3 kexVec3::ScreenProject(kexMatrix &proj, kexMatrix &model,
                               const int width, const int height,
                               const int wx, const int wy) {
    kexVec4 projVec;
    kexVec4 modelVec;

    modelVec.ToVec3() = *this;
    modelVec |= model;

    projVec = (modelVec | proj);
    projVec.x *= modelVec.w;
    projVec.y *= modelVec.w;
    projVec.z *= modelVec.w;

    if(projVec.w != 0) {
        projVec.w = 1.0f / projVec.w;
        projVec.x *= projVec.w;
        projVec.y *= projVec.w;
        projVec.z *= projVec.w;

        return kexVec3(
            (projVec.x * 0.5f + 0.5f) * width + wx,
            (-projVec.y * 0.5f + 0.5f) * height + wy,
            (1.0f + projVec.z) * 0.5f);
    }

    return kexVec3(*this);
}

//
// kexVec3::operator+
//

kexVec3 kexVec3::operator+(const kexVec3 &vec) {
    return kexVec3(x + vec.x, y + vec.y, z + vec.z);
}

//
// kexVec3::operator+
//

kexVec3 kexVec3::operator+(const kexVec3 &vec) const {
    return kexVec3(x + vec.x, y + vec.y, z + vec.z);
}

//
// kexVec3::operator+
//

kexVec3 kexVec3::operator+(kexVec3 &vec) {
    return kexVec3(x + vec.x, y + vec.y, z + vec.z);
}

//
// kexVec3::operator+
//

kexVec3 kexVec3::operator+(const float val) {
    return kexVec3(x + val, y + val, z + val);
}

//
// kexVec3::operator+=
//

kexVec3 &kexVec3::operator+=(const kexVec3 &vec) {
    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

//
// kexVec3::operator+=
//

kexVec3 &kexVec3::operator+=(const float val) {
    x += val;
    y += val;
    z += val;
    return *this;
}

//
// kexVec3::operator-
//

kexVec3 kexVec3::operator-(const kexVec3 &vec) const {
    return kexVec3(x - vec.x, y - vec.y, z - vec.z);
}

//
// kexVec3::operator-
//

kexVec3 kexVec3::operator-(void) const {
    return kexVec3(-x, -y, -z);
}

//
// kexVec3::operator-=
//

kexVec3 &kexVec3::operator-=(const kexVec3 &vec) {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

//
// kexVec3::operator*
//

kexVec3 kexVec3::operator*(const kexVec3 &vec) {
    return kexVec3(x * vec.x, y * vec.y, z * vec.z);
}

//
// kexVec3::operator*=
//

kexVec3 &kexVec3::operator*=(const kexVec3 &vec) {
    x *= vec.x;
    y *= vec.y;
    z *= vec.z;
    return *this;
}

//
// kexVec3::operator*
//

kexVec3 kexVec3::operator*(const float val) {
    return kexVec3(x * val, y * val, z * val);
}

//
// kexVec3::operator*
//

kexVec3 kexVec3::operator*(const float val) const {
    return kexVec3(x * val, y * val, z * val);
}

//
// kexVec3::operator*=
//

kexVec3 &kexVec3::operator*=(const float val) {
    x *= val;
    y *= val;
    z *= val;
    return *this;
}

//
// kexVec3::operator/
//

kexVec3 kexVec3::operator/(const kexVec3 &vec) {
    return kexVec3(x / vec.x, y / vec.y, z / vec.z);
}

//
// kexVec3::operator/=
//

kexVec3 &kexVec3::operator/=(const kexVec3 &vec) {
    x /= vec.x;
    y /= vec.y;
    z /= vec.z;
    return *this;
}

//
// kexVec3::operator/
//

kexVec3 kexVec3::operator/(const float val) {
    return kexVec3(x / val, y / val, z / val);
}

//
// kexVec3::operator/=
//

kexVec3 &kexVec3::operator/=(const float val) {
    x /= val;
    y /= val;
    z /= val;
    return *this;
}

//
// kexVec3::operator|
//

kexVec3 kexVec3::operator|(const kexQuat &quat) {
    float xx = quat.x * quat.x;
    float yx = quat.y * quat.x;
    float zx = quat.z * quat.x;
    float wx = quat.w * quat.x;
    float yy = quat.y * quat.y;
    float zy = quat.z * quat.y;
    float wy = quat.w * quat.y;
    float zz = quat.z * quat.z;
    float wz = quat.w * quat.z;
    float ww = quat.w * quat.w;

    return kexVec3(
        ((yx + yx) - (wz + wz)) * y +
        ((wy + wy + zx + zx)) * z +
        (((ww + xx) - yy) - zz) * x,
        ((yy + (ww - xx)) - zz) * y +
        ((zy + zy) - (wx + wx)) * z +
        ((wz + wz) + (yx + yx)) * x,
        ((wx + wx) + (zy + zy)) * y +
        (((ww - xx) - yy) + zz) * z +
        ((zx + zx) - (wy + wy)) * x
    );
}

//
// kexVec3::operator|
//

kexVec3 kexVec3::operator|(const kexMatrix &mtx) {
    return kexVec3(
        mtx.vectors[1].x * y + mtx.vectors[2].x * z + mtx.vectors[0].x * x + mtx.vectors[3].x,
        mtx.vectors[1].y * y + mtx.vectors[2].y * z + mtx.vectors[0].y * x + mtx.vectors[3].y,
        mtx.vectors[1].z * y + mtx.vectors[2].z * z + mtx.vectors[0].z * x + mtx.vectors[3].z);
}

//
// kexVec3::operator|=
//

kexVec3 &kexVec3::operator|=(const kexQuat &quat) {
    float xx = quat.x * quat.x;
    float yx = quat.y * quat.x;
    float zx = quat.z * quat.x;
    float wx = quat.w * quat.x;
    float yy = quat.y * quat.y;
    float zy = quat.z * quat.y;
    float wy = quat.w * quat.y;
    float zz = quat.z * quat.z;
    float wz = quat.w * quat.z;
    float ww = quat.w * quat.w;
    float vx = x;
    float vy = y;
    float vz = z;

    x = ((yx + yx) - (wz + wz)) * vy +
        ((wy + wy + zx + zx)) * vz +
        (((ww + xx) - yy) - zz) * vx;
    y = ((yy + (ww - xx)) - zz) * vy +
        ((zy + zy) - (wx + wx)) * vz +
        ((wz + wz) + (yx + yx)) * vx;
    z = ((wx + wx) + (zy + zy)) * vy +
        (((ww - xx) - yy) + zz) * vz +
        ((zx + zx) - (wy + wy)) * vx;

    return *this;
}

//
// kexVec3::operator|=
//

kexVec3 &kexVec3::operator|=(const kexMatrix &mtx) {
    float _x = x;
    float _y = y;
    float _z = z;
    
    x = mtx.vectors[1].x * _y + mtx.vectors[2].x * _z + mtx.vectors[0].x * _x + mtx.vectors[3].x;
    y = mtx.vectors[1].y * _y + mtx.vectors[2].y * _z + mtx.vectors[0].y * _x + mtx.vectors[3].y;
    z = mtx.vectors[1].z * _y + mtx.vectors[2].z * _z + mtx.vectors[0].z * _x + mtx.vectors[3].z;

    return *this;
}

//
// kexVec3::operator=
//

kexVec3 &kexVec3::operator=(const kexVec3 &vec) {
    x = vec.x;
    y = vec.y;
    z = vec.z;
    return *this;
}

//
// kexVec3::operator=
//

kexVec3 &kexVec3::operator=(const float *vecs) {
    x = vecs[0];
    y = vecs[1];
    z = vecs[2];
    return *this;
}

//
// kexVec3::operator[]
//

float kexVec3::operator[](int index) const {
    assert(index >= 0 && index < 3);
    return (&x)[index];
}

//
// kexVec3::operator[]
//

float &kexVec3::operator[](int index) {
    assert(index >= 0 && index < 3);
    return (&x)[index];
}

//
// kexVec4::kexVec4
//

kexVec4::kexVec4(void) {
    Clear();
}

//
// kexVec4::kexVec4
//

kexVec4::kexVec4(const float x, const float y, const float z, const float w) {
    Set(x, y, z, w);
}

//
// kexVec4::Set
//

void kexVec4::Set(const float x, const float y, const float z, const float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

//
// kexVec4::Clear
//

void kexVec4::Clear(void) {
    x = y = z = w = 0.0f;
}

//
// kexVec4::ToVec3
//

kexVec3 const &kexVec4::ToVec3(void) const {
    return *reinterpret_cast<const kexVec3*>(this);
}

//
// kexVec4::ToVec3
//

kexVec3 &kexVec4::ToVec3(void) {
    return *reinterpret_cast<kexVec3*>(this);
}

//
// kexVec4::ToFloatPtr
//

float *kexVec4::ToFloatPtr(void) {
    return reinterpret_cast<float*>(&x);
}

//
// kexVec4::operator|
//

kexVec4 kexVec4::operator|(const kexMatrix &mtx) {
    return kexVec4(
        mtx.vectors[1].x * y + mtx.vectors[2].x * z + mtx.vectors[0].x * x + mtx.vectors[3].x,
        mtx.vectors[1].y * y + mtx.vectors[2].y * z + mtx.vectors[0].y * x + mtx.vectors[3].y,
        mtx.vectors[1].z * y + mtx.vectors[2].z * z + mtx.vectors[0].z * x + mtx.vectors[3].z,
        mtx.vectors[1].w * y + mtx.vectors[2].w * z + mtx.vectors[0].w * x + mtx.vectors[3].w);
}

//
// kexVec4::operator|=
//

kexVec4 &kexVec4::operator|=(const kexMatrix &mtx) {
    float _x = x;
    float _y = y;
    float _z = z;
    float _w = w;
    
    x = mtx.vectors[1].x * _y + mtx.vectors[2].x * _z + mtx.vectors[0].x * _x + mtx.vectors[3].x;
    y = mtx.vectors[1].y * _y + mtx.vectors[2].y * _z + mtx.vectors[0].y * _x + mtx.vectors[3].y;
    z = mtx.vectors[1].z * _y + mtx.vectors[2].z * _z + mtx.vectors[0].z * _x + mtx.vectors[3].z;
    w = mtx.vectors[1].w * _y + mtx.vectors[2].w * _z + mtx.vectors[0].w * _x + mtx.vectors[3].w;

    return *this;
}

//
// kexVec4::operator[]
//

float kexVec4::operator[](int index) const {
    assert(index >= 0 && index < 3);
    return (&x)[index];
}

//
// kexVec4::operator[]
//

float &kexVec4::operator[](int index) {
    assert(index >= 0 && index < 3);
    return (&x)[index];
}
