#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>

namespace xiiSimdConversion
{
  XII_ALWAYS_INLINE xiiVec3 ToVec3(const xiiSimdVec4f& v)
  {
    xiiVec4 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<xiiVec3*>(&tmp.x);
  }

  XII_ALWAYS_INLINE xiiSimdVec4f ToVec3(const xiiVec3& v)
  {
    xiiSimdVec4f tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiVec4 ToVec4(const xiiSimdVec4f& v)
  {
    xiiVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdVec4f ToVec4(const xiiVec4& v)
  {
    xiiSimdVec4f tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiQuat ToQuat(const xiiSimdQuat& q)
  {
    xiiQuat tmp;
    q.m_v.Store<4>(&tmp.v.x);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdQuat ToQuat(const xiiQuat& q)
  {
    xiiSimdVec4f tmp;
    tmp.Load<4>(&q.v.x);
    return xiiSimdQuat(tmp);
  }

  XII_ALWAYS_INLINE xiiTransform ToTransform(const xiiSimdTransform& t)
  {
    return xiiTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline xiiSimdTransform ToTransform(const xiiTransform& t)
  {
    return xiiSimdTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  XII_ALWAYS_INLINE xiiMat4 ToMat4(const xiiSimdMat4f& m)
  {
    xiiMat4 tmp;
    m.GetAsArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiSimdMat4f ToMat4(const xiiMat4& m)
  {
    xiiSimdMat4f tmp;
    tmp.SetFromArray(m.m_fElementsCM, xiiMatrixLayout::ColumnMajor);
    return tmp;
  }

  XII_ALWAYS_INLINE xiiBoundingBoxSphere ToBBoxSphere(const xiiSimdBBoxSphere& b)
  {
    xiiVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return xiiBoundingBoxSphere(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiSimdBBoxSphere ToBBoxSphere(const xiiBoundingBoxSphere& b)
  {
    return xiiSimdBBoxSphere(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtends), b.m_fSphereRadius);
  }

  XII_ALWAYS_INLINE xiiBoundingSphere ToBSphere(const xiiSimdBSphere& s)
  {
    xiiVec4 centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return xiiBoundingSphere(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  XII_ALWAYS_INLINE xiiSimdBSphere ToBSphere(const xiiBoundingSphere& s) { return xiiSimdBSphere(ToVec3(s.m_vCenter), s.m_fRadius); }

  XII_ALWAYS_INLINE xiiSimdBBox ToBBox(const xiiBoundingBox& b) { return xiiSimdBBox(ToVec3(b.m_vMin), ToVec3(b.m_vMax)); }

  XII_ALWAYS_INLINE xiiBoundingBox ToBBox(const xiiSimdBBox& b) { return xiiBoundingBox(ToVec3(b.m_Min), ToVec3(b.m_Max)); }

}; // namespace xiiSimdConversion
