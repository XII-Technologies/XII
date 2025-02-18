#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

// xiiVec2Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiVec2Template<Type>& vValue)
{
  ref_stream.WriteBytes(&vValue, sizeof(xiiVec2Template<Type>)).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiVec2Template<Type>& ref_vValue)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_vValue, sizeof(xiiVec2Template<Type>)) == sizeof(xiiVec2Template<Type>), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiVec2Template<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiVec2Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiVec2Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec2Template<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiVec3Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiVec3Template<Type>& vValue)
{
  ref_stream.WriteBytes(&vValue, sizeof(xiiVec3Template<Type>)).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiVec3Template<Type>& ref_vValue)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_vValue, sizeof(xiiVec3Template<Type>)) == sizeof(xiiVec3Template<Type>), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiVec3Template<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiVec3Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiVec3Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec3Template<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiVec4Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiVec4Template<Type>& vValue)
{
  ref_stream.WriteBytes(&vValue, sizeof(xiiVec4Template<Type>)).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiVec4Template<Type>& ref_vValue)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_vValue, sizeof(xiiVec4Template<Type>)) == sizeof(xiiVec4Template<Type>), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiVec4Template<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiVec4Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiVec4Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec4Template<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiMat3Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiMat3Template<Type>& mValue)
{
  ref_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiMat3Template<Type>& ref_mValue)
{
  XII_VERIFY(ref_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 9) == sizeof(Type) * 9, "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiMat3Template<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiMat3Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiMat3Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiMat3Template<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiMat4Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiMat4Template<Type>& mValue)
{
  ref_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiMat4Template<Type>& ref_mValue)
{
  XII_VERIFY(ref_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 16) == sizeof(Type) * 16, "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiMat4Template<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiMat4Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiMat4Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiMat4Template<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiTransformTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiTransformTemplate<Type>& value)
{
  ref_stream << value.m_qRotation;
  ref_stream << value.m_vPosition;
  ref_stream << value.m_vScale;

  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiTransformTemplate<Type>& ref_value)
{
  ref_stream >> ref_value.m_qRotation;
  ref_stream >> ref_value.m_vPosition;
  ref_stream >> ref_value.m_vScale;

  return ref_stream;
}

// xiiPlaneTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiPlaneTemplate<Type>& value)
{
  ref_stream.WriteBytes(&value, sizeof(xiiPlaneTemplate<Type>)).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiPlaneTemplate<Type>& ref_value)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_value, sizeof(xiiPlaneTemplate<Type>)) == sizeof(xiiPlaneTemplate<Type>), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiPlaneTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiPlaneTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiPlaneTemplate<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiQuatTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiQuatTemplate<Type>& qValue)
{
  ref_stream.WriteBytes(&qValue, sizeof(xiiQuatTemplate<Type>)).AssertSuccess();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiQuatTemplate<Type>& ref_qValue)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_qValue, sizeof(xiiQuatTemplate<Type>)) == sizeof(xiiQuatTemplate<Type>), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiQuatTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiQuatTemplate<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiQuatTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiQuatTemplate<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiBoundingBoxTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiBoundingBoxTemplate<Type>& value)
{
  ref_stream << value.m_vMax;
  ref_stream << value.m_vMin;
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiBoundingBoxTemplate<Type>& ref_value)
{
  ref_stream >> ref_value.m_vMax;
  ref_stream >> ref_value.m_vMin;
  return ref_stream;
}

// xiiBoundingSphereTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiBoundingSphereTemplate<Type>& value)
{
  ref_stream << value.m_vCenter;
  ref_stream << value.m_fRadius;
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiBoundingSphereTemplate<Type>& ref_value)
{
  ref_stream >> ref_value.m_vCenter;
  ref_stream >> ref_value.m_fRadius;
  return ref_stream;
}

// xiiBoundingBoxSphereTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiBoundingBoxSphereTemplate<Type>& value)
{
  ref_stream << value.m_vCenter;
  ref_stream << value.m_fSphereRadius;
  ref_stream << value.m_vBoxHalfExtents;
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiBoundingBoxSphereTemplate<Type>& ref_value)
{
  ref_stream >> ref_value.m_vCenter;
  ref_stream >> ref_value.m_fSphereRadius;
  ref_stream >> ref_value.m_vBoxHalfExtents;
  return ref_stream;
}

// xiiColor
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiColor& value)
{
  ref_stream.WriteBytes(&value, sizeof(xiiColor)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiColor& ref_value)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_value, sizeof(xiiColor)) == sizeof(xiiColor), "End of stream reached.");
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiColor* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiColor) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiColor* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColor) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiColorGammaUB
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiColorGammaUB& value)
{
  ref_stream.WriteBytes(&value, sizeof(xiiColorGammaUB)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiColorGammaUB& ref_value)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_value, sizeof(xiiColorGammaUB)) == sizeof(xiiColorGammaUB), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiColorGammaUB* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiColorGammaUB) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiColorGammaUB* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColorGammaUB) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiAngleTemplate
template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiAngleTemplate<Type>& value)
{
  ref_stream << value.GetRadian();
  return ref_stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiAngleTemplate<Type>& ref_value)
{
  Type fRadian;
  ref_stream >> fRadian;
  ref_value.SetRadian(fRadian);
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiAngleTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiAngleTemplate<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiAngleTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiAngleTemplate<Type>) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiColor8Unorm
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiColorLinearUB& value)
{
  ref_stream.WriteBytes(&value, sizeof(xiiColorLinearUB)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiColorLinearUB& ref_value)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_value, sizeof(xiiColorLinearUB)) == sizeof(xiiColorLinearUB), "End of stream reached.");
  return ref_stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiColorLinearUB* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiColorLinearUB) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiColorLinearUB* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColorLinearUB) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}
