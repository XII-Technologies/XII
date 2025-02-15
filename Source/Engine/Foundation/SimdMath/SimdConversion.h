#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdBBoxSphered.h>
#include <Foundation/SimdMath/SimdBBoxd.h>
#include <Foundation/SimdMath/SimdVec4i.h>

namespace xiiSimdConversion
{
  XII_ALWAYS_INLINE xiiVec3 ToVec3(const xiiSimdVec4f& v)
  {
    xiiVec4 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<xiiVec3*>(&tmp.x);
  }

  XII_ALWAYS_INLINE xiiVec3d ToVec3(const xiiSimdVec4d& v)
  {
    xiiVec4d tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<xiiVec3d*>(&tmp.x);
  }

  XII_ALWAYS_INLINE xiiSimdVec4f ToVec3(const xiiVec3& v)
  {
    xiiSimdVec4f tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdVec4d ToVec3(const xiiVec3d& v)
  {
    xiiSimdVec4d tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiVec3I32 ToVec3i(const xiiSimdVec4i& v)
  {
    xiiVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<xiiVec3I32*>(&tmp.x);
  }

  XII_ALWAYS_INLINE xiiSimdVec4i ToVec3i(const xiiVec3I32& v)
  {
    xiiSimdVec4i tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiVec4 ToVec4(const xiiSimdVec4f& v)
  {
    xiiVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiVec4d ToVec4(const xiiSimdVec4d& v)
  {
    xiiVec4d tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdVec4f ToVec4(const xiiVec4& v)
  {
    xiiSimdVec4f tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdVec4d ToVec4(const xiiVec4d& v)
  {
    xiiSimdVec4d tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiVec4I32 ToVec4i(const xiiSimdVec4i& v)
  {
    xiiVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdVec4i ToVec4i(const xiiVec4I32& v)
  {
    xiiSimdVec4i tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiQuat ToQuat(const xiiSimdQuat& q)
  {
    xiiQuat tmp;
    q.m_v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiQuatd ToQuat(const xiiSimdQuatd& q)
  {
    xiiQuatd tmp;
    q.m_v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdQuat ToQuat(const xiiQuat& q)
  {
    xiiSimdVec4f tmp;
    tmp.Load<4>(&q.x);
    return xiiSimdQuat(tmp);
  }

  XII_ALWAYS_INLINE xiiSimdQuatd ToQuat(const xiiQuatd& q)
  {
    xiiSimdVec4d tmp;
    tmp.Load<4>(&q.x);
    return xiiSimdQuatd(tmp);
  }

  XII_ALWAYS_INLINE xiiTransform ToTransform(const xiiSimdTransform& t)
  {
    return xiiTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  XII_ALWAYS_INLINE xiiTransformd ToTransform(const xiiSimdTransformd& t)
  {
    return xiiTransformd(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline xiiSimdTransform ToTransform(const xiiTransform& t)
  {
    return xiiSimdTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  inline xiiSimdTransformd ToTransform(const xiiTransformd& t)
  {
    return xiiSimdTransformd(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  XII_ALWAYS_INLINE xiiMat4 ToMat4(const xiiSimdMat4f& m)
  {
    xiiMat4 tmp;
    m.GetAsArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiMat4d ToMat4(const xiiSimdMat4d& m)
  {
    xiiMat4d tmp;
    m.GetAsArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdMat4f ToMat4(const xiiMat4& m)
  {
    return xiiSimdMat4f::MakeFromColumnMajorArray(m.m_fElementsCM);
  }

  XII_ALWAYS_INLINE xiiSimdMat4d ToMat4(const xiiMat4d& m)
  {
    return xiiSimdMat4d::MakeFromColumnMajorArray(m.m_fElementsCM);
  }

  XII_ALWAYS_INLINE xiiBoundingBoxSphere ToBBoxSphere(const xiiSimdBBoxSphere& b)
  {
    xiiVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return xiiBoundingBoxSphere::MakeFromCenterExtents(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiBoundingBoxSphered ToBBoxSphere(const xiiSimdBBoxSphered& b)
  {
    xiiVec4d centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return xiiBoundingBoxSphered::MakeFromCenterExtents(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiSimdBBoxSphere ToBBoxSphere(const xiiBoundingBoxSphere& b)
  {
    return xiiSimdBBoxSphere::MakeFromCenterExtents(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtents), b.m_fSphereRadius);
  }

  XII_ALWAYS_INLINE xiiSimdBBoxSphered ToBBoxSphere(const xiiBoundingBoxSphered& b)
  {
    return xiiSimdBBoxSphered::MakeFromCenterExtents(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtents), b.m_fSphereRadius);
  }

  XII_ALWAYS_INLINE xiiBoundingSphere ToBSphere(const xiiSimdBSphere& s)
  {
    xiiVec4 centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return xiiBoundingSphere::MakeFromCenterAndRadius(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiBoundingSphered ToBSphere(const xiiSimdBSphered& s)
  {
    xiiVec4d centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return xiiBoundingSphered::MakeFromCenterAndRadius(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiSimdBSphere ToBSphere(const xiiBoundingSphere& s)
  {
    return xiiSimdBSphere(ToVec3(s.m_vCenter), s.m_fRadius);
  }

  XII_ALWAYS_INLINE xiiSimdBSphered ToBSphere(const xiiBoundingSphered& s)
  {
    return xiiSimdBSphered(ToVec3(s.m_vCenter), s.m_fRadius);
  }

  XII_ALWAYS_INLINE xiiSimdBBox ToBBox(const xiiBoundingBox& b)
  {
    return xiiSimdBBox(ToVec3(b.m_vMin), ToVec3(b.m_vMax));
  }

  XII_ALWAYS_INLINE xiiSimdBBoxd ToBBox(const xiiBoundingBoxd& b)
  {
    return xiiSimdBBoxd(ToVec3(b.m_vMin), ToVec3(b.m_vMax));
  }

  XII_ALWAYS_INLINE xiiBoundingBox ToBBox(const xiiSimdBBox& b)
  {
    return xiiBoundingBox::MakeFromMinMax(ToVec3(b.m_Min), ToVec3(b.m_Max));
  }

  XII_ALWAYS_INLINE xiiBoundingBoxd ToBBox(const xiiSimdBBoxd& b)
  {
    return xiiBoundingBoxd::MakeFromMinMax(ToVec3(b.m_Min), ToVec3(b.m_Max));
  }

}; // namespace xiiSimdConversion
