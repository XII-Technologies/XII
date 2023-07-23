#pragma once

template <typename Type>
xiiMat3Template<Type>::xiiMat3Template()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  SetElements(TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN);
#endif
}

template <typename Type>
xiiMat3Template<Type>::xiiMat3Template(const Type* const pData, xiiMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

template <typename Type>
xiiMat3Template<Type>::xiiMat3Template(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3)
{
  SetElements(c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiMat3Template<Type> xiiMat3Template<Type>::IdentityMatrix()
{
  return xiiMat3Template<Type>(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiMat3Template<Type> xiiMat3Template<Type>::ZeroMatrix()
{
  return xiiMat3Template<Type>(0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <typename Type>
void xiiMat3Template<Type>::SetFromArray(const Type* const pData, xiiMatrixLayout::Enum layout)
{
  if (layout == xiiMatrixLayout::ColumnMajor)
  {
    xiiMemoryUtils::Copy(m_fElementsCM, pData, 9);
  }
  else
  {
    for (xiiInt32 i = 0; i < 3; ++i)
    {
      Element(0, i) = pData[i * 3 + 0];
      Element(1, i) = pData[i * 3 + 1];
      Element(2, i) = pData[i * 3 + 2];
    }
  }
}

template <typename Type>
void xiiMat3Template<Type>::GetAsArray(Type* out_pData, xiiMatrixLayout::Enum layout) const
{
  XII_NAN_ASSERT(this);

  if (layout == xiiMatrixLayout::ColumnMajor)
  {
    xiiMemoryUtils::Copy(out_pData, m_fElementsCM, 9);
  }
  else
  {
    for (xiiInt32 i = 0; i < 3; ++i)
    {
      out_pData[i * 3 + 0] = Element(0, i);
      out_pData[i * 3 + 1] = Element(1, i);
      out_pData[i * 3 + 2] = Element(2, i);
    }
  }
}

template <typename Type>
void xiiMat3Template<Type>::SetElements(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3)
{
  Element(0, 0) = c1r1;
  Element(1, 0) = c2r1;
  Element(2, 0) = c3r1;
  Element(0, 1) = c1r2;
  Element(1, 1) = c2r2;
  Element(2, 1) = c3r2;
  Element(0, 2) = c1r3;
  Element(1, 2) = c2r3;
  Element(2, 2) = c3r3;
}

template <typename Type>
void xiiMat3Template<Type>::SetZero()
{
  SetElements(0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <typename Type>
void xiiMat3Template<Type>::SetIdentity()
{
  SetElements(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

template <typename Type>
void xiiMat3Template<Type>::SetScalingMatrix(const xiiVec3Template<Type>& s)
{
  SetElements(s.x, 0, 0, 0, s.y, 0, 0, 0, s.z);
}

template <typename Type>
void xiiMat3Template<Type>::SetRotationMatrixX(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);

  SetElements(1.0f, 0.0f, 0.0f, 0.0f, fCos, -fSin, 0.0f, fSin, fCos);
}

template <typename Type>
void xiiMat3Template<Type>::SetRotationMatrixY(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);


  SetElements(fCos, 0.0f, fSin, 0.0f, 1.0f, 0.0f, -fSin, 0.0f, fCos);
}

template <typename Type>
void xiiMat3Template<Type>::SetRotationMatrixZ(xiiAngleTemplate<Type> angle)
{
  const Type fSin = xiiMath::Sin(angle);
  const Type fCos = xiiMath::Cos(angle);

  SetElements(fCos, -fSin, 0.0f, fSin, fCos, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
void xiiMat3Template<Type>::Transpose()
{
  xiiMath::Swap(Element(0, 1), Element(1, 0));
  xiiMath::Swap(Element(0, 2), Element(2, 0));
  xiiMath::Swap(Element(1, 2), Element(2, 1));
}

template <typename Type>
const xiiMat3Template<Type> xiiMat3Template<Type>::GetTranspose() const
{
  return xiiMat3Template(m_fElementsCM, xiiMatrixLayout::RowMajor);
}

template <typename Type>
const xiiMat3Template<Type> xiiMat3Template<Type>::GetInverse(Type fEpsilon) const
{
  xiiMat3Template<Type> Inverse = *this;
  xiiResult             res     = Inverse.Invert(fEpsilon);
  XII_ASSERT_DEBUG(res.Succeeded(), "Could not invert the given Mat3.");
  XII_IGNORE_UNUSED(res);
  return Inverse;
}

template <typename Type>
xiiVec3Template<Type> xiiMat3Template<Type>::GetRow(xiiUInt32 uiRow) const
{
  XII_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  xiiVec3Template<Type> r;
  r.x = Element(0, uiRow);
  r.y = Element(1, uiRow);
  r.z = Element(2, uiRow);

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat3Template<Type>::SetRow(xiiUInt32 uiRow, const xiiVec3Template<Type>& vRow)
{
  XII_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  Element(0, uiRow) = vRow.x;
  Element(1, uiRow) = vRow.y;
  Element(2, uiRow) = vRow.z;
}

template <typename Type>
xiiVec3Template<Type> xiiMat3Template<Type>::GetColumn(xiiUInt32 uiColumn) const
{
  XII_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  xiiVec3Template<Type> r;
  r.x = Element(uiColumn, 0);
  r.y = Element(uiColumn, 1);
  r.z = Element(uiColumn, 2);

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void xiiMat3Template<Type>::SetColumn(xiiUInt32 uiColumn, const xiiVec3Template<Type>& vColumn)
{
  XII_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  Element(uiColumn, 0) = vColumn.x;
  Element(uiColumn, 1) = vColumn.y;
  Element(uiColumn, 2) = vColumn.z;
}

template <typename Type>
xiiVec3Template<Type> xiiMat3Template<Type>::GetDiagonal() const
{
  XII_NAN_ASSERT(this);

  return xiiVec3Template<Type>(Element(0, 0), Element(1, 1), Element(2, 2));
}

template <typename Type>
void xiiMat3Template<Type>::SetDiagonal(const xiiVec3Template<Type>& vDiag)
{
  Element(0, 0) = vDiag.x;
  Element(1, 1) = vDiag.y;
  Element(2, 2) = vDiag.z;
}

template <typename Type>
XII_FORCE_INLINE const xiiVec3Template<Type> xiiMat3Template<Type>::TransformDirection(const xiiVec3Template<Type>& v) const
{
  xiiVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z;

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
XII_FORCE_INLINE void xiiMat3Template<Type>::operator*=(Type f)
{
  for (xiiInt32 i = 0; i < 9; ++i)
    m_fElementsCM[i] *= f;

  XII_NAN_ASSERT(this);
}

template <typename Type>
XII_FORCE_INLINE void xiiMat3Template<Type>::operator/=(Type f)
{
  const Type fInv = xiiMath::Invert(f);

  operator*=(fInv);
}

template <typename Type>
const xiiMat3Template<Type> operator*(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2)
{
  xiiMat3Template<Type> r;
  for (xiiInt32 i = 0; i < 3; ++i)
  {
    r.Element(0, i) = m1.Element(0, i) * m2.Element(0, 0) + m1.Element(1, i) * m2.Element(0, 1) + m1.Element(2, i) * m2.Element(0, 2);
    r.Element(1, i) = m1.Element(0, i) * m2.Element(1, 0) + m1.Element(1, i) * m2.Element(1, 1) + m1.Element(2, i) * m2.Element(1, 2);
    r.Element(2, i) = m1.Element(0, i) * m2.Element(2, 0) + m1.Element(1, i) * m2.Element(2, 1) + m1.Element(2, i) * m2.Element(2, 2);
  }

  XII_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> operator*(const xiiMat3Template<Type>& m, const xiiVec3Template<Type>& v)
{
  return m.TransformDirection(v);
}



// *** free functions ***

template <typename Type>
XII_ALWAYS_INLINE const xiiMat3Template<Type> operator*(Type f, const xiiMat3Template<Type>& m1)
{
  return operator*(m1, f);
}

template <typename Type>
const xiiMat3Template<Type> operator*(const xiiMat3Template<Type>& m1, Type f)
{
  xiiMat3Template<Type> r;

  for (xiiUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  XII_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiMat3Template<Type> operator/(const xiiMat3Template<Type>& m1, Type f)
{
  return operator*(m1, xiiMath::Invert(f));
}

template <typename Type>
const xiiMat3Template<Type> operator+(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2)
{
  xiiMat3Template<Type> r;

  for (xiiUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  XII_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
const xiiMat3Template<Type> operator-(const xiiMat3Template<Type>& m1, const xiiMat3Template<Type>& m2)
{
  xiiMat3Template<Type> r;

  for (xiiUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  XII_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
bool xiiMat3Template<Type>::IsIdentical(const xiiMat3Template<Type>& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat3Template<Type>::IsEqual(const xiiMat3Template<Type>& rhs, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  XII_ASSERT_DEBUG(fEpsilon >= 0.0f, "Epsilon may not be negative.");

  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    if (!xiiMath::IsEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiMat3Template<Type>& lhs, const xiiMat3Template<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
XII_ALWAYS_INLINE bool operator!=(const xiiMat3Template<Type>& lhs, const xiiMat3Template<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
bool xiiMat3Template<Type>::IsZero(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    if (!xiiMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat3Template<Type>::IsIdentity(Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  if (!xiiMath::IsEqual(Element(0, 0), (Type)1, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(0, 1), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(0, 2), (Type)0, fEpsilon))
    return false;

  if (!xiiMath::IsEqual(Element(1, 0), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(1, 1), (Type)1, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(1, 2), (Type)0, fEpsilon))
    return false;

  if (!xiiMath::IsEqual(Element(2, 0), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(2, 1), (Type)0, fEpsilon))
    return false;
  if (!xiiMath::IsEqual(Element(2, 2), (Type)1, fEpsilon))
    return false;

  return true;
}

template <typename Type>
bool xiiMat3Template<Type>::IsValid() const
{
  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    if (!xiiMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}

template <typename Type>
bool xiiMat3Template<Type>::IsNaN() const
{
  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    if (xiiMath::IsNaN(m_fElementsCM[i]))
      return true;
  }

  return false;
}

template <typename Type>
const xiiVec3Template<Type> xiiMat3Template<Type>::GetScalingFactors() const
{
  xiiVec3Template<Type> v;

  v.x = xiiVec3Template<Type>(Element(0, 0), Element(0, 1), Element(0, 2)).GetLength();
  v.y = xiiVec3Template<Type>(Element(1, 0), Element(1, 1), Element(1, 2)).GetLength();
  v.z = xiiVec3Template<Type>(Element(2, 0), Element(2, 1), Element(2, 2)).GetLength();

  XII_NAN_ASSERT(&v);
  return v;
}

template <typename Type>
xiiResult xiiMat3Template<Type>::SetScalingFactors(const xiiVec3Template<Type>& vXYZ, Type fEpsilon /* = xiiMath::DefaultEpsilon<Type>() */)
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

template <typename Type>
Type xiiMat3Template<Type>::GetDeterminant() const
{
  // Using rule of Sarrus
  Type fDeterminant = 0;
  for (xiiInt32 i = 0; i < 3; i++)
  {
    fDeterminant += Element(i, 0) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 2);
    fDeterminant -= Element(i, 2) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 0);
  }
  return fDeterminant;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
