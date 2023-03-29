#pragma once

template <typename T>
XII_ALWAYS_INLINE xiiAmbientCube<T>::xiiAmbientCube()
{
  xiiMemoryUtils::ZeroFillArray(m_Values);
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiAmbientCube<T>::xiiAmbientCube(const xiiAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
XII_FORCE_INLINE void xiiAmbientCube<T>::operator=(const xiiAmbientCube<U>& other)
{
  for (xiiUInt32 i = 0; i < xiiAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
XII_FORCE_INLINE bool xiiAmbientCube<T>::operator==(const xiiAmbientCube& other) const
{
  return xiiMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
XII_ALWAYS_INLINE bool xiiAmbientCube<T>::operator!=(const xiiAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void xiiAmbientCube<T>::AddSample(const xiiVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += xiiMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += xiiMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += xiiMath::Abs(vDir.z) * value;
}

template <typename T>
T xiiAmbientCube<T>::Evaluate(const xiiVec3& vNormal) const
{
  xiiVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
    vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}

template <typename T>
xiiResult xiiAmbientCube<T>::Serialize(xiiStreamWriter& stream) const
{
  return stream.WriteArray(m_Values);
}

template <typename T>
xiiResult xiiAmbientCube<T>::Deserialize(xiiStreamReader& stream)
{
  return stream.ReadArray(m_Values);
}
