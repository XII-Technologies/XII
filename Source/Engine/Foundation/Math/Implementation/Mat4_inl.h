/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Mat3.h>

template <typename Type>
xiiMat4Template<Type>::xiiMat4Template()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  for (xiiUInt32 i = 0; i < 16; ++i)
    m_fElementsCM[i] = TypeNaN;
#endif
}

template <typename Type>
xiiMat4Template<Type>::xiiMat4Template(const Type* const pData, xiiMatrixLayout::Enum layout)
{
  if (layout == xiiMatrixLayout::ColumnMajor)
  {
    xiiMemoryUtils::Copy(m_fElementsCM, pData, 16);
  }
  else
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      Element(0, i) = pData[i * 4 + 0];
      Element(1, i) = pData[i * 4 + 1];
      Element(2, i) = pData[i * 4 + 2];
      Element(3, i) = pData[i * 4 + 3];
    }
  }
}

template <typename Type>
xiiMat4Template<Type>::xiiMat4Template(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3, Type c1r4, Type c2r4, Type c3r4, Type c4r4)
{
  Element(0, 0) = c1r1;
  Element(1, 0) = c2r1;
  Element(2, 0) = c3r1;
  Element(3, 0) = c4r1;
  Element(0, 1) = c1r2;
  Element(1, 1) = c2r2;
  Element(2, 1) = c3r2;
  Element(3, 1) = c4r2;
  Element(0, 2) = c1r3;
  Element(1, 2) = c2r3;
  Element(2, 2) = c3r3;
  Element(3, 2) = c4r3;
  Element(0, 3) = c1r4;
  Element(1, 3) = c2r4;
  Element(2, 3) = c3r4;
  Element(3, 3) = c4r4;
}

template <typename Type>
xiiMat4Template<Type>::xiiMat4Template(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation)
{
  SetTransformationMatrix(mRotation, vTranslation);
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeZero()
{
  xiiMat4Template<Type> res;

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(res.m_fElementsCM); ++i)
    res.m_fElementsCM[i] = 0.0f;

  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeIdentity()
{
  xiiMat4Template<Type> res;
  res.m_fElementsCM[0]  = 1.0f;
  res.m_fElementsCM[1]  = 0.0f;
  res.m_fElementsCM[2]  = 0.0f;
  res.m_fElementsCM[3]  = 0.0f;
  res.m_fElementsCM[4]  = 0.0f;
  res.m_fElementsCM[5]  = 1.0f;
  res.m_fElementsCM[6]  = 0.0f;
  res.m_fElementsCM[7]  = 0.0f;
  res.m_fElementsCM[8]  = 0.0f;
  res.m_fElementsCM[9]  = 0.0f;
  res.m_fElementsCM[10] = 1.0f;
  res.m_fElementsCM[11] = 0.0f;
  res.m_fElementsCM[12] = 0.0f;
  res.m_fElementsCM[13] = 0.0f;
  res.m_fElementsCM[14] = 0.0f;
  res.m_fElementsCM[15] = 1.0f;
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeFromRowMajorArray(const Type* const pData)
{
  xiiMat4Template<Type> res;
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    res.Element(0, i) = pData[i * 4 + 0];
    res.Element(1, i) = pData[i * 4 + 1];
    res.Element(2, i) = pData[i * 4 + 2];
    res.Element(3, i) = pData[i * 4 + 3];
  }
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeFromColumnMajorArray(const Type* const pData)
{
  xiiMat4Template<Type> res;
  xiiMemoryUtils::Copy(res.m_fElementsCM, pData, 16);
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3, Type c1r4, Type c2r4, Type c3r4, Type c4r4)
{
  xiiMat4Template<Type> res;
  res.Element(0, 0) = c1r1;
  res.Element(1, 0) = c2r1;
  res.Element(2, 0) = c3r1;
  res.Element(3, 0) = c4r1;
  res.Element(0, 1) = c1r2;
  res.Element(1, 1) = c2r2;
  res.Element(2, 1) = c3r2;
  res.Element(3, 1) = c4r2;
  res.Element(0, 2) = c1r3;
  res.Element(1, 2) = c2r3;
  res.Element(2, 2) = c3r3;
  res.Element(3, 2) = c4r3;
  res.Element(0, 3) = c1r4;
  res.Element(1, 3) = c2r4;
  res.Element(2, 3) = c3r4;
  res.Element(3, 3) = c4r4;
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeTranslation(const xiiVec3Template<Type>& vTranslation)
{
  return xiiMat4Template<Type>::MakeFromValues(1, 0, 0, vTranslation.x, 0, 1, 0, vTranslation.y, 0, 0, 1, vTranslation.z, 0, 0, 0, 1);
}


template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeTransformation(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation)
{
  xiiMat4Template<Type> res;
  res.SetTransformationMatrix(mRotation, vTranslation);
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeScaling(const xiiVec3Template<Type>& vScale)
{
  xiiMat4Template<Type> res;
  res.Element(0, 0) = vScale.x;
  res.Element(1, 0) = 0;
  res.Element(2, 0) = 0;
  res.Element(3, 0) = 0;
  res.Element(0, 1) = 0;
  res.Element(1, 1) = vScale.y;
  res.Element(2, 1) = 0;
  res.Element(3, 1) = 0;
  res.Element(0, 2) = 0;
  res.Element(1, 2) = 0;
  res.Element(2, 2) = vScale.z;
  res.Element(3, 2) = 0;
  res.Element(0, 3) = 0;
  res.Element(1, 3) = 0;
  res.Element(2, 3) = 0;
  res.Element(3, 3) = 1;
  return res;
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeRotationX(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);

  return xiiMat4Template<Type>::MakeFromValues(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, fCos, -fSin, 0.0f, 0.0f, fSin, fCos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeRotationY(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);

  return xiiMat4Template<Type>::MakeFromValues(fCos, 0.0f, fSin, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -fSin, 0.0f, fCos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
xiiMat4Template<Type> xiiMat4Template<Type>::MakeRotationZ(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);

  return xiiMat4Template<Type>::MakeFromValues(fCos, -fSin, 0.0f, 0.0f, fSin, fCos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
void xiiMat4Template<Type>::SetTransformationMatrix(const xiiMat3Template<Type>& mRotation, const xiiVec3Template<Type>& vTranslation)
{
  SetRotationalPart(mRotation);
  SetTranslationVector(vTranslation);
  SetRow(3, xiiVec4Template<Type>(0, 0, 0, 1));
}

template <typename Type>
void xiiMat4Template<Type>::GetAsArray(Type* out_pData, xiiMatrixLayout::Enum layout) const
{
  XII_NAN_ASSERT(this);

  if (layout == xiiMatrixLayout::ColumnMajor)
  {
    xiiMemoryUtils::Copy(out_pData, m_fElementsCM, 16);
  }
  else
  {
    for (int i = 0; i < 4; ++i)
    {
      out_pData[i * 4 + 0] = Element(0, i);
      out_pData[i * 4 + 1] = Element(1, i);
      out_pData[i * 4 + 2] = Element(2, i);
      out_pData[i * 4 + 3] = Element(3, i);
    }
  }
}

template <typename Type>
void xiiMat4Template<Type>::SetZero()
{
  *this = MakeZero();
}

template <typename Type>
void xiiMat4Template<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
void xiiMat4Template<Type>::Transpose()
{
  xiiMath::Swap(Element(0, 1), Element(1, 0));
  xiiMath::Swap(Element(0, 2), Element(2, 0));
  xiiMath::Swap(Element(0, 3), Element(3, 0));
  xiiMath::Swap(Element(1, 2), Element(2, 1));
  xiiMath::Swap(Element(1, 3), Element(3, 1));
  xiiMath::Swap(Element(2, 3), Element(3, 2));
}

template <typename Type>
const xiiMat4Template<Type> xiiMat4Template<Type>::GetTranspose() const
{
  XII_NAN_ASSERT(this);

  return xiiMat4Template::MakeFromRowMajorArray(m_fElementsCM);
}

template <typename Type>
const xiiMat4Template<Type> xiiMat4Template<Type>::GetInverse(Type fEpsilon) const
{
  xiiMat4Template<Type> Inverse = *this;
  xiiResult             res     = Inverse.Invert(fEpsilon);
  XII_ASSERT_DEBUG(res.Succeeded(), "Could not invert the given Mat4.");
  XII_IGNORE_UNUSED(res);
  return Inverse;
}

template <typename Type>
xiiVec4Template<Type> xiiMat4Template<Type>::GetRow(xiiUInt32 uiRow) const
{
  XII_NAN_ASSERT(this);
  XII_ASSERT_DEBUG(uiRow <= 3, "Invalid Row Index {0}", uiRow);

  xiiVec4Template<Type> r;
  r.x = Element(0, uiRow);
  r.y = Element(1, uiRow);
  r.z = Element(2, uiRow);
  r.w = Element(3, uiRow);

  return r;
}

template <typename Type>
void xiiMat4Template<Type>::SetRow(xiiUInt32 uiRow, const xiiVec4Template<Type>& vRow)
{
  XII_ASSERT_DEBUG(uiRow <= 3, "Invalid Row Index {0}", uiRow);

  Element(0, uiRow) = vRow.x;
  Element(1, uiRow) = vRow.y;
  Element(2, uiRow) = vRow.z;
  Element(3, uiRow) = vRow.w;
}

template <typename Type>
xiiVec4Template<Type> xiiMat4Template<Type>::GetColumn(xiiUInt32 uiColumn) const
{
  XII_NAN_ASSERT(this);
  XII_ASSERT_DEBUG(uiColumn <= 3, "Invalid Column Index {0}", uiColumn);

  xiiVec4Template<Type> r;
  r.x = Element(uiColumn, 0);
  r.y = Element(uiColumn, 1);
  r.z = Element(uiColumn, 2);
  r.w = Element(uiColumn, 3);

  return r;
}

template <typename Type>
void xiiMat4Template<Type>::SetColumn(xiiUInt32 uiColumn, const xiiVec4Template<Type>& vColumn)
{
  XII_ASSERT_DEBUG(uiColumn <= 3, "Invalid Column Index {0}", uiColumn);

  Element(uiColumn, 0) = vColumn.x;
  Element(uiColumn, 1) = vColumn.y;
  Element(uiColumn, 2) = vColumn.z;
  Element(uiColumn, 3) = vColumn.w;
}

template <typename Type>
xiiVec4Template<Type> xiiMat4Template<Type>::GetDiagonal() const
{
  XII_NAN_ASSERT(this);

  return xiiVec4Template<Type>(Element(0, 0), Element(1, 1), Element(2, 2), Element(3, 3));
}

template <typename Type>
void xiiMat4Template<Type>::SetDiagonal(const xiiVec4Template<Type>& vDiag)
{
  Element(0, 0) = vDiag.x;
  Element(1, 1) = vDiag.y;
  Element(2, 2) = vDiag.z;
  Element(3, 3) = vDiag.w;
}

template <typename Type>
const xiiVec3Template<Type> xiiMat4Template<Type>::TransformPosition(const xiiVec3Template<Type>& v) const
{
  xiiVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z + Element(3, 0);
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z + Element(3, 1);
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z + Element(3, 2);

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat4Template<Type>::TransformPosition(xiiVec3Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride /* = sizeof(xiiVec3Template) */) const
{
  XII_ASSERT_DEBUG(pV != nullptr, "Array must not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Data must not overlap.");

  xiiVec3Template<Type>* pCur = pV;

  for (xiiUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformPosition(*pCur);
    pCur  = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
const xiiVec3Template<Type> xiiMat4Template<Type>::TransformDirection(const xiiVec3Template<Type>& v) const
{
  xiiVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z;

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat4Template<Type>::TransformDirection(xiiVec3Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride /* = sizeof(xiiVec3Template<Type>) */) const
{
  XII_ASSERT_DEBUG(pV != nullptr, "Array must not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec3Template<Type>), "Data must not overlap.");

  xiiVec3Template<Type>* pCur = pV;

  for (xiiUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformDirection(*pCur);
    pCur  = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
const xiiVec4Template<Type> xiiMat4Template<Type>::Transform(const xiiVec4Template<Type>& v) const
{
  xiiVec4Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z + Element(3, 0) * v.w;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z + Element(3, 1) * v.w;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z + Element(3, 2) * v.w;
  r.w = Element(0, 3) * v.x + Element(1, 3) * v.y + Element(2, 3) * v.z + Element(3, 3) * v.w;

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat4Template<Type>::Transform(xiiVec4Template<Type>* pV, xiiUInt32 uiNumVectors, xiiUInt32 uiStride /* = sizeof(xiiVec4Template) */) const
{
  XII_ASSERT_DEBUG(pV != nullptr, "Array must not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiVec4Template<Type>), "Data must not overlap.");

  xiiVec4Template<Type>* pCur = pV;

  for (xiiUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = Transform(*pCur);
    pCur  = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiMat4Template<Type>::GetTranslationVector() const
{
  XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(Element(3, 0), Element(3, 1), Element(3, 2));
}

template <typename Type>
XII_ALWAYS_INLINE void xiiMat4Template<Type>::SetTranslationVector(const xiiVec3Template<Type>& v)
{
  Element(3, 0) = v.x;
  Element(3, 1) = v.y;
  Element(3, 2) = v.z;
}

template <typename Type>
void xiiMat4Template<Type>::SetRotationalPart(const xiiMat3Template<Type>& mRotation)
{
  for (xiiUInt32 col = 0; col < 3; ++col)
  {
    for (xiiUInt32 row = 0; row < 3; ++row)
    {
      Element(col, row) = mRotation.Element(col, row);
    }
  }
}

template <typename Type>
const xiiMat3Template<Type> xiiMat4Template<Type>::GetRotationalPart() const
{
  xiiMat3Template<Type> r;

  for (xiiUInt32 col = 0; col < 3; ++col)
  {
    for (xiiUInt32 row = 0; row < 3; ++row)
    {
      r.Element(col, row) = Element(col, row);
    }
  }

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat4Template<Type>::operator*=(Type f)
{
  for (xiiInt32 i = 0; i < 16; ++i)
    m_fElementsCM[i] *= f;

  XII_NAN_ASSERT(this);
}

template <typename Type>
void xiiMat4Template<Type>::operator/=(Type f)
{
  const Type fInv = xiiMath::Invert(f);

  operator*=(fInv);
}

template <typename Type>
const xiiMat4Template<Type> operator*(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2)
{
  xiiMat4Template<Type> r;
  for (xiiInt32 i = 0; i < 4; ++i)
  {
    r.Element(0, i) = m1.Element(0, i) * m2.Element(0, 0) + m1.Element(1, i) * m2.Element(0, 1) + m1.Element(2, i) * m2.Element(0, 2) +
      m1.Element(3, i) * m2.Element(0, 3);
    r.Element(1, i) = m1.Element(0, i) * m2.Element(1, 0) + m1.Element(1, i) * m2.Element(1, 1) + m1.Element(2, i) * m2.Element(1, 2) +
      m1.Element(3, i) * m2.Element(1, 3);
    r.Element(2, i) = m1.Element(0, i) * m2.Element(2, 0) + m1.Element(1, i) * m2.Element(2, 1) + m1.Element(2, i) * m2.Element(2, 2) +
      m1.Element(3, i) * m2.Element(2, 3);
    r.Element(3, i) = m1.Element(0, i) * m2.Element(3, 0) + m1.Element(1, i) * m2.Element(3, 1) + m1.Element(2, i) * m2.Element(3, 2) +
      m1.Element(3, i) * m2.Element(3, 3);
  }

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> operator*(const xiiMat4Template<Type>& m, const xiiVec3Template<Type>& v)
{
  return m.TransformPosition(v);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec4Template<Type> operator*(const xiiMat4Template<Type>& m, const xiiVec4Template<Type>& v)
{
  return m.Transform(v);
}



// *** Stuff needed for matrix inversion ***

template <typename Type>
XII_FORCE_INLINE Type GetDeterminantOf3x3SubMatrix(const xiiMat4Template<Type>& m, xiiInt32 i, xiiInt32 j)
{
  const xiiInt32 si0 = 0 + ((i <= 0) ? 1 : 0);
  const xiiInt32 si1 = 1 + ((i <= 1) ? 1 : 0);
  const xiiInt32 si2 = 2 + ((i <= 2) ? 1 : 0);

  const xiiInt32 sj0 = 0 + ((j <= 0) ? 1 : 0);
  const xiiInt32 sj1 = 1 + ((j <= 1) ? 1 : 0);
  const xiiInt32 sj2 = 2 + ((j <= 2) ? 1 : 0);

  Type fDet2 = ((m.Element(sj0, si0) * m.Element(sj1, si1) * m.Element(sj2, si2) + m.Element(sj1, si0) * m.Element(sj2, si1) * m.Element(sj0, si2) +
                 m.Element(sj2, si0) * m.Element(sj0, si1) * m.Element(sj1, si2)) -
                (m.Element(sj0, si2) * m.Element(sj1, si1) * m.Element(sj2, si0) + m.Element(sj1, si2) * m.Element(sj2, si1) * m.Element(sj0, si0) +
                 m.Element(sj2, si2) * m.Element(sj0, si1) * m.Element(sj1, si0)));

  return fDet2;
}

template <typename Type>
XII_FORCE_INLINE Type GetDeterminantOf4x4Matrix(const xiiMat4Template<Type>& m)
{
  Type det = 0.0;

  det += m.Element(0, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 0);
  det += -m.Element(1, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 1);
  det += m.Element(2, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 2);
  det += -m.Element(3, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 3);

  return det;
}


// *** free functions ***

template <typename Type>
XII_ALWAYS_INLINE const xiiMat4Template<Type> operator*(Type f, const xiiMat4Template<Type>& m1)
{
  return operator*(m1, f);
}

template <typename Type>
const xiiMat4Template<Type> operator*(const xiiMat4Template<Type>& m1, Type f)
{
  xiiMat4Template<Type> r;

  for (xiiUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
const xiiMat4Template<Type> operator/(const xiiMat4Template<Type>& m1, Type f)
{
  return operator*(m1, xiiMath::Invert(f));
}

template <typename Type>
const xiiMat4Template<Type> operator+(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2)
{
  xiiMat4Template<Type> r;

  for (xiiUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
const xiiMat4Template<Type> operator-(const xiiMat4Template<Type>& m1, const xiiMat4Template<Type>& m2)
{
  xiiMat4Template<Type> r;

  for (xiiUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
bool xiiMat4Template<Type>::IsIdentical(const xiiMat4Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat4Template<Type>::IsEqual(const xiiMat4Template<Type>& rhs, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  XII_ASSERT_DEBUG(fEpsilon >= 0.0f, "Epsilon may not be negative.");

  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (!xiiMath::IsEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiMat4Template<Type>& lhs, const xiiMat4Template<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
bool xiiMat4Template<Type>::IsZero(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (!xiiMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat4Template<Type>::IsIdentity(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  if (!xiiMath::IsEqual(Element(0, 0), (Type)1, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(0, 1), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(0, 2), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(0, 3), (Type)0, fEpsilon))
    return false;

  if (!xiiMath::IsEqual(Element(1, 0), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(1, 1), (Type)1, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(1, 2), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(1, 3), (Type)0, fEpsilon))
    return false;

  if (!xiiMath::IsEqual(Element(2, 0), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(2, 1), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(2, 2), (Type)1, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(2, 3), (Type)0, fEpsilon))
    return false;

  if (!xiiMath::IsEqual(Element(3, 0), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(3, 1), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(3, 2), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(3, 3), (Type)1, fEpsilon))
    return false;

  return true;
}

template <typename Type>
bool xiiMat4Template<Type>::IsValid() const
{
  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (!xiiMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat4Template<Type>::IsNaN() const
{
  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (xiiMath::IsNaN(m_fElementsCM[i]))
      return true;
  }

  return false;
}

template <typename Type>
const xiiVec3Template<Type> xiiMat4Template<Type>::GetScalingFactors() const
{
  xiiVec3Template<Type> v;

  v.x = xiiVec3Template<Type>(Element(0, 0), Element(0, 1), Element(0, 2)).GetLength();
  v.y = xiiVec3Template<Type>(Element(1, 0), Element(1, 1), Element(1, 2)).GetLength();
  v.z = xiiVec3Template<Type>(Element(2, 0), Element(2, 1), Element(2, 2)).GetLength();

  XII_NAN_ASSERT(&v);
  return v;
}

template <typename Type>
xiiResult xiiMat4Template<Type>::SetScalingFactors(const xiiVec3Template<Type>& vXYZ, Type fEpsilon /* = xiiMath::DefaultEpsilon<Type>() */)
{
  xiiVec3Template<Type> tx(Element(0, 0), Element(0, 1), Element(0, 2));
  xiiVec3Template<Type> ty(Element(1, 0), Element(1, 1), Element(1, 2));
  xiiVec3Template<Type> tz(Element(2, 0), Element(2, 1), Element(2, 2));

  if (tx.SetLength(vXYZ.x, fEpsilon) == XII_FAILURE)
    return XII_FAILURE;
  if (ty.SetLength(vXYZ.y, fEpsilon) == XII_FAILURE)
    return XII_FAILURE;
  if (tz.SetLength(vXYZ.z, fEpsilon) == XII_FAILURE)
    return XII_FAILURE;


  Element(0, 0) = tx.x;
  Element(0, 1) = tx.y;
  Element(0, 2) = tx.z;
  Element(1, 0) = ty.x;
  Element(1, 1) = ty.y;
  Element(1, 2) = ty.z;
  Element(2, 0) = tz.x;
  Element(2, 1) = tz.y;
  Element(2, 2) = tz.z;

  return XII_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
