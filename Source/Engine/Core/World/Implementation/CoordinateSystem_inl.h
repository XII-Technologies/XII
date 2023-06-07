
template <typename Type>
XII_ALWAYS_INLINE xiiCoordinateSystemConversionTemplate<Type>::xiiCoordinateSystemConversionTemplate()
{
  m_mSourceToTarget.SetIdentity();
  m_mTargetToSource.SetIdentity();
}

template <typename Type>
XII_ALWAYS_INLINE void xiiCoordinateSystemConversionTemplate<Type>::SetConversion(const xiiCoordinateSystemTemplate<Type>& source, const xiiCoordinateSystemTemplate<Type>& target)
{
  Type fSourceScale = source.m_vForwardDir.GetLengthSquared();
  XII_ASSERT_DEV(xiiMath::IsEqual(fSourceScale, source.m_vRightDir.GetLengthSquared(), xiiMath::DefaultEpsilon<Type>()),
                 "Only uniformly scaled coordinate systems are supported");

  XII_ASSERT_DEV(xiiMath::IsEqual(fSourceScale, source.m_vUpDir.GetLengthSquared(), xiiMath::DefaultEpsilon<Type>()),
                 "Only uniformly scaled coordinate systems are supported");

  xiiMat3Template<Type> mSourceFromId;
  mSourceFromId.SetColumn(0, source.m_vRightDir);
  mSourceFromId.SetColumn(1, source.m_vUpDir);
  mSourceFromId.SetColumn(2, source.m_vForwardDir);

  Type fTargetScale = target.m_vForwardDir.GetLengthSquared();
  XII_ASSERT_DEV(xiiMath::IsEqual(fTargetScale, target.m_vRightDir.GetLengthSquared(), xiiMath::DefaultEpsilon<Type>()),
                 "Only uniformly scaled coordinate systems are supported");
  XII_ASSERT_DEV(xiiMath::IsEqual(fTargetScale, target.m_vUpDir.GetLengthSquared(), xiiMath::DefaultEpsilon<Type>()),
                 "Only uniformly scaled coordinate systems are supported");

  xiiMat3Template<Type> mTargetFromId;
  mTargetFromId.SetColumn(0, target.m_vRightDir);
  mTargetFromId.SetColumn(1, target.m_vUpDir);
  mTargetFromId.SetColumn(2, target.m_vForwardDir);

  m_mSourceToTarget = mTargetFromId * mSourceFromId.GetInverse();
  m_mSourceToTarget.SetColumn(0, m_mSourceToTarget.GetColumn(0).GetNormalized());
  m_mSourceToTarget.SetColumn(1, m_mSourceToTarget.GetColumn(1).GetNormalized());
  m_mSourceToTarget.SetColumn(2, m_mSourceToTarget.GetColumn(2).GetNormalized());

  m_fWindingSwap         = m_mSourceToTarget.GetDeterminant() < 0 ? static_cast<Type>(-1) : static_cast<Type>(1);
  m_fSourceToTargetScale = static_cast<Type>(1) / xiiMath::Sqrt(fSourceScale) * xiiMath::Sqrt(fTargetScale);
  m_mTargetToSource      = m_mSourceToTarget.GetInverse();
  m_fTargetToSourceScale = static_cast<Type>(1) / m_fSourceToTargetScale;
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type> xiiCoordinateSystemConversionTemplate<Type>::ConvertSourcePosition(const xiiVec3Template<Type>& vPos) const
{
  return m_mSourceToTarget * vPos * m_fSourceToTargetScale;
}

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type> xiiCoordinateSystemConversionTemplate<Type>::ConvertSourceRotation(const xiiQuatTemplate<Type>& qOrientation) const
{
  xiiVec3Template<Type> axis = m_mSourceToTarget * vOrientation.v;
  xiiQuatTemplate<Type> rr(axis.x, axis.y, axis.z, vOrientation.w * m_fWindingSwap);
  return rr;
}

template <typename Type>
XII_ALWAYS_INLINE Type xiiCoordinateSystemConversionTemplate<Type>::ConvertSourceLength(Type fLength) const
{
  return fLength * m_fSourceToTargetScale;
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type> xiiCoordinateSystemConversionTemplate<Type>::ConvertTargetPosition(const xiiVec3Template<Type>& vPos) const
{
  return m_mTargetToSource * vPos * m_fTargetToSourceScale;
}

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type> xiiCoordinateSystemConversionTemplate<Type>::ConvertTargetRotation(const xiiQuatTemplate<Type>& qOrientation) const
{
  xiiVec3Template<Type> axis = m_mTargetToSource * vOrientation.v;
  xiiQuatTemplate<Type> rr(axis.x, axis.y, axis.z, vOrientation.w * m_fWindingSwap);
  return rr;
}

template <typename Type>
XII_ALWAYS_INLINE Type xiiCoordinateSystemConversionTemplate<Type>::ConvertTargetLength(Type fLength) const
{
  return fLength * m_fTargetToSourceScale;
}
