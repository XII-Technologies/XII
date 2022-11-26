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
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiVec2Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiVec2Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiVec2Template<Type>& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiVec2Template<Type>));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiVec2Template<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiVec2Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiVec2Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec2Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiVec3Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiVec3Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiVec3Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiVec3Template<Type>& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiVec3Template<Type>));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiVec3Template<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiVec3Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiVec3Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec3Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiVec4Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiVec4Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiVec4Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiVec4Template<Type>& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiVec4Template<Type>));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiVec4Template<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiVec4Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiVec4Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiVec4Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiMat3Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiMat3Template<Type>& Value)
{
  stream.WriteBytes(Value.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiMat3Template<Type>& Value)
{
  stream.ReadBytes(Value.m_fElementsCM, sizeof(Type) * 9);
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiMat3Template<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiMat3Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiMat3Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiMat3Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiMat4Template

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiMat4Template<Type>& Value)
{
  stream.WriteBytes(Value.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiMat4Template<Type>& Value)
{
  stream.ReadBytes(Value.m_fElementsCM, sizeof(Type) * 16);
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiMat4Template<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiMat4Template<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiMat4Template<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiMat4Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiTransformTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiTransformTemplate<Type>& Value)
{
  stream << Value.m_qRotation;
  stream << Value.m_vPosition;
  stream << Value.m_vScale;

  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiTransformTemplate<Type>& Value)
{
  stream >> Value.m_qRotation;
  stream >> Value.m_vPosition;
  stream >> Value.m_vScale;

  return stream;
}

// xiiPlaneTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiPlaneTemplate<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiPlaneTemplate<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiPlaneTemplate<Type>& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiPlaneTemplate<Type>));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiPlaneTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiPlaneTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiPlaneTemplate<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiQuatTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiQuatTemplate<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiQuatTemplate<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiQuatTemplate<Type>& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiQuatTemplate<Type>));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiQuatTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiQuatTemplate<Type>) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiQuatTemplate<Type>* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiQuatTemplate<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiBoundingBoxTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiBoundingBoxTemplate<Type>& Value)
{
  stream << Value.m_vMax;
  stream << Value.m_vMin;
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiBoundingBoxTemplate<Type>& Value)
{
  stream >> Value.m_vMax;
  stream >> Value.m_vMin;
  return stream;
}

// xiiBoundingSphereTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiBoundingSphereTemplate<Type>& Value)
{
  stream << Value.m_vCenter;
  stream << Value.m_fRadius;
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiBoundingSphereTemplate<Type>& Value)
{
  stream >> Value.m_vCenter;
  stream >> Value.m_fRadius;
  return stream;
}

// xiiBoundingBoxSphereTemplate

template <typename Type>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiBoundingBoxSphereTemplate<Type>& Value)
{
  stream << Value.m_vCenter;
  stream << Value.m_fSphereRadius;
  stream << Value.m_vBoxHalfExtends;
  return stream;
}

template <typename Type>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiBoundingBoxSphereTemplate<Type>& Value)
{
  stream >> Value.m_vCenter;
  stream >> Value.m_fSphereRadius;
  stream >> Value.m_vBoxHalfExtends;
  return stream;
}

// xiiColor
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiColor& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiColor)).AssertSuccess();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiColor& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiColor));
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiColor* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiColor) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiColor* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColor) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiColorGammaUB
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiColorGammaUB& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiColorGammaUB)).AssertSuccess();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiColorGammaUB& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiColorGammaUB));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiColorGammaUB* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiColorGammaUB) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiColorGammaUB* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColorGammaUB) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiAngle
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiAngle& Value)
{
  stream << Value.GetRadian();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiAngle& Value)
{
  float fRadian;
  stream >> fRadian;
  Value.SetRadian(fRadian);
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiAngle* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiAngle) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiAngle* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiAngle) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// xiiColor8Unorm
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiColorLinearUB& Value)
{
  stream.WriteBytes(&Value, sizeof(xiiColorLinearUB)).AssertSuccess();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiColorLinearUB& Value)
{
  stream.ReadBytes(&Value, sizeof(xiiColorLinearUB));
  return stream;
}

template <typename Type>
xiiResult SerializeArray(xiiStreamWriter& stream, const xiiColorLinearUB* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiColorLinearUB) * uiCount);
}

template <typename Type>
xiiResult DeserializeArray(xiiStreamReader& stream, xiiColorLinearUB* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiColorLinearUB) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}
