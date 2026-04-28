/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxSphereTemplate<Type>::xiiBoundingBoxSphereTemplate()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vOrigin and m_vBoxHalfExtents are already initialized to NaN by their own constructor.
  const Type TypeNaN = xiiMath::NaN<Type>();
  m_fSphereRadius    = TypeNaN;
#endif
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxSphereTemplate<Type>::xiiBoundingBoxSphereTemplate(const xiiBoundingBoxSphereTemplate& rhs)
{
  m_vCenter         = rhs.m_vCenter;
  m_fSphereRadius   = rhs.m_fSphereRadius;
  m_vBoxHalfExtents = rhs.m_vBoxHalfExtents;
}

template <typename Type>
void xiiBoundingBoxSphereTemplate<Type>::operator=(const xiiBoundingBoxSphereTemplate& rhs)
{
  m_vCenter         = rhs.m_vCenter;
  m_fSphereRadius   = rhs.m_fSphereRadius;
  m_vBoxHalfExtents = rhs.m_vBoxHalfExtents;
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type>::xiiBoundingBoxSphereTemplate(const xiiBoundingBoxTemplate<Type>& box) :
  m_vCenter(box.GetCenter())
{
  m_vBoxHalfExtents = box.GetHalfExtents();
  m_fSphereRadius   = m_vBoxHalfExtents.GetLength();
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type>::xiiBoundingBoxSphereTemplate(const xiiBoundingSphereTemplate<Type>& sphere) :
  m_vCenter(sphere.m_vCenter), m_fSphereRadius(sphere.m_fRadius)
{
  m_vBoxHalfExtents.Set(m_fSphereRadius);
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeZero()
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = 0;
  res.m_vBoxHalfExtents.SetZero();
  return res;
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeInvalid()
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = -xiiMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  res.m_vBoxHalfExtents.Set(-xiiMath::MaxValue<Type>());
  return res;
}

template <typename Type>
XII_FORCE_INLINE xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeFromCenterExtents(const xiiVec3Template<Type>& vCenter, const xiiVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius)
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter         = vCenter;
  res.m_fSphereRadius   = fSphereRadius;
  res.m_vBoxHalfExtents = vBoxHalfExtents;
  return res;
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeFromPoints(const xiiVec3Template<Type>* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiVec3Template<Type>)*/)
{
  xiiBoundingBoxTemplate<Type> box = xiiBoundingBoxTemplate<Type>::MakeFromPoints(pPoints, uiNumPoints, uiStride);

  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter         = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();

  xiiBoundingSphereTemplate<Type> sphere = xiiBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(res.m_vCenter, 0.0f);
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  res.m_fSphereRadius = sphere.m_fRadius;
  return res;
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeFromBox(const xiiBoundingBoxTemplate<Type>& box)
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter         = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();
  res.m_fSphereRadius   = res.m_vBoxHalfExtents.GetLength();
  return res;
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeFromSphere(const xiiBoundingSphereTemplate<Type>& sphere)
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter       = sphere.m_vCenter;
  res.m_fSphereRadius = sphere.m_fRadius;
  res.m_vBoxHalfExtents.Set(res.m_fSphereRadius);
  return res;
}

template <typename Type>
xiiBoundingBoxSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::MakeFromBoxAndSphere(const xiiBoundingBoxTemplate<Type>& box, const xiiBoundingSphereTemplate<Type>& sphere)
{
  xiiBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter         = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();
  res.m_fSphereRadius   = xiiMath::Min(res.m_vBoxHalfExtents.GetLength(), (sphere.m_vCenter - res.m_vCenter).GetLength() + sphere.m_fRadius);
  return res;
}

template <typename Type>
XII_FORCE_INLINE bool xiiBoundingBoxSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fSphereRadius >= 0.0f && m_vBoxHalfExtents.IsValid() && (m_vBoxHalfExtents.x >= 0) && (m_vBoxHalfExtents.y >= 0) && (m_vBoxHalfExtents.z >= 0));
}

template <typename Type>
XII_FORCE_INLINE bool xiiBoundingBoxSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || xiiMath::IsNaN(m_fSphereRadius) || m_vBoxHalfExtents.IsNaN());
}

template <typename Type>
XII_FORCE_INLINE const xiiBoundingBoxTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::GetBox() const
{
  return xiiBoundingBoxTemplate<Type>::MakeFromMinMax(m_vCenter - m_vBoxHalfExtents, m_vCenter + m_vBoxHalfExtents);
}

template <typename Type>
XII_FORCE_INLINE const xiiBoundingSphereTemplate<Type> xiiBoundingBoxSphereTemplate<Type>::GetSphere() const
{
  return xiiBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(m_vCenter, m_fSphereRadius);
}

template <typename Type>
void xiiBoundingBoxSphereTemplate<Type>::ExpandToInclude(const xiiBoundingBoxSphereTemplate& rhs)
{
  xiiBoundingBoxTemplate<Type> box;
  box.m_vMin = m_vCenter - m_vBoxHalfExtents;
  box.m_vMax = m_vCenter + m_vBoxHalfExtents;
  box.ExpandToInclude(rhs.GetBox());

  xiiBoundingBoxSphereTemplate<Type> result = xiiBoundingBoxSphereTemplate<Type>::MakeFromBox(box);

  const Type fSphereRadiusA = (m_vCenter - result.m_vCenter).GetLength() + m_fSphereRadius;
  const Type fSphereRadiusB = (rhs.m_vCenter - result.m_vCenter).GetLength() + rhs.m_fSphereRadius;

  m_vCenter         = result.m_vCenter;
  m_fSphereRadius   = xiiMath::Min(result.m_fSphereRadius, xiiMath::Max(fSphereRadiusA, fSphereRadiusB));
  m_vBoxHalfExtents = result.m_vBoxHalfExtents;
}

template <typename Type>
void xiiBoundingBoxSphereTemplate<Type>::Transform(const xiiMat4Template<Type>& mTransform)
{
  m_vCenter                         = mTransform.TransformPosition(m_vCenter);
  const xiiVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fSphereRadius *= xiiMath::Max(Scale.x, Scale.y, Scale.z);

  xiiMat3Template<Type> mAbsRotation = mTransform.GetRotationalPart();
  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    mAbsRotation.m_fElementsCM[i] = xiiMath::Abs(mAbsRotation.m_fElementsCM[i]);
  }

  m_vBoxHalfExtents = mAbsRotation.TransformDirection(m_vBoxHalfExtents).CompMin(xiiVec3(m_fSphereRadius));
}

template <typename Type>
XII_FORCE_INLINE bool operator==(const xiiBoundingBoxSphereTemplate<Type>& lhs, const xiiBoundingBoxSphereTemplate<Type>& rhs)
{
  return lhs.m_vCenter == rhs.m_vCenter && lhs.m_vBoxHalfExtents == rhs.m_vBoxHalfExtents && lhs.m_fSphereRadius == rhs.m_fSphereRadius;
}
