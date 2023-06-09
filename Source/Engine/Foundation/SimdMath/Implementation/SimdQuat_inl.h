#pragma once

XII_ALWAYS_INLINE xiiSimdQuat::xiiSimdQuat() = default;

XII_ALWAYS_INLINE xiiSimdQuat::xiiSimdQuat(const xiiSimdVec4f& v)
{
  m_v = v;
}

// static
XII_ALWAYS_INLINE xiiSimdQuat xiiSimdQuat::IdentityQuaternion()
{
  return xiiSimdQuat(xiiSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f));
}

XII_ALWAYS_INLINE void xiiSimdQuat::SetIdentity()
{
  m_v.Set(0.0f, 0.0f, 0.0f, 1.0f);
}

XII_ALWAYS_INLINE void xiiSimdQuat::SetFromAxisAndAngle(const xiiSimdVec4f& vRotationAxis, const xiiSimdFloat& fAngle)
{
  ///\todo optimize
  const xiiAngle halfAngle = xiiAngle::Radian(fAngle) * 0.5f;
  float          s         = xiiMath::Sin(halfAngle);
  float          c         = xiiMath::Cos(halfAngle);

  m_v = vRotationAxis * s;
  m_v.SetW(c);
}

XII_ALWAYS_INLINE void xiiSimdQuat::Normalize()
{
  m_v.Normalize<4>();
}

inline xiiResult xiiSimdQuat::GetRotationAxisAndAngle(xiiSimdVec4f& ref_vAxis, xiiSimdFloat& ref_fAngle, const xiiSimdFloat& fEpsilon) const
{
  ///\todo optimize
  const xiiAngle acos = xiiMath::ACos((float)m_v.w());
  const float    d    = xiiMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = acos * 2.0f;

  return XII_SUCCESS;
}

XII_ALWAYS_INLINE xiiSimdMat4f xiiSimdQuat::GetAsMat4() const
{
  const xiiSimdVec4f xyz       = m_v;
  const xiiSimdVec4f x2y2z2    = xyz + xyz;
  const xiiSimdVec4f xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const xiiSimdVec4f yy2_xx2_xx2 = xx2yy2zz2.Get<xiiSwizzle::YXXX>();
  const xiiSimdVec4f zz2_zz2_yy2 = xx2yy2zz2.Get<xiiSwizzle::ZZYX>();
  xiiSimdVec4f       diagonal    = xiiSimdVec4f(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(xiiSimdFloat::Zero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const xiiSimdVec4f x_y_x    = xyz.Get<xiiSwizzle::XYXX>();
  const xiiSimdVec4f y2_z2_z2 = x2y2z2.Get<xiiSwizzle::YZZX>();
  const xiiSimdVec4f base     = x_y_x.CompMul(y2_z2_z2);

  const xiiSimdVec4f z2_x2_y2 = x2y2z2.Get<xiiSwizzle::ZXYX>();
  const xiiSimdVec4f offset   = z2_x2_y2 * m_v.w();

  const xiiSimdVec4f adds = base + offset;
  const xiiSimdVec4f subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const xiiSimdVec4f addX_u_diaX_u = adds.GetCombined<xiiSwizzle::XXXX>(diagonal);
  const xiiSimdVec4f subZ_u_diaW_u = subs.GetCombined<xiiSwizzle::ZXWX>(diagonal);
  const xiiSimdVec4f col0          = addX_u_diaX_u.GetCombined<xiiSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const xiiSimdVec4f subX_u_diaY_u = subs.GetCombined<xiiSwizzle::XXYX>(diagonal);
  const xiiSimdVec4f addY_u_diaW_u = adds.GetCombined<xiiSwizzle::YXWX>(diagonal);
  const xiiSimdVec4f col1          = subX_u_diaY_u.GetCombined<xiiSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const xiiSimdVec4f addZ_u_subY_u = adds.GetCombined<xiiSwizzle::ZXYX>(subs);
  const xiiSimdVec4f col2          = addZ_u_subY_u.GetCombined<xiiSwizzle::XZZW>(diagonal);

  return xiiSimdMat4f(col0, col1, col2, xiiSimdVec4f(0, 0, 0, 1));
}

XII_ALWAYS_INLINE bool xiiSimdQuat::IsValid(const xiiSimdFloat& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdQuat::IsNaN() const
{
  return m_v.IsNaN<4>();
}

XII_ALWAYS_INLINE xiiSimdQuat xiiSimdQuat::operator-() const
{
  return m_v.FlipSign(xiiSimdVec4b(true, true, true, false));
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdQuat::operator*(const xiiSimdVec4f& v) const
{
  xiiSimdVec4f t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

XII_ALWAYS_INLINE xiiSimdQuat xiiSimdQuat::operator*(const xiiSimdQuat& q2) const
{
  xiiSimdQuat q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

XII_ALWAYS_INLINE bool xiiSimdQuat::operator==(const xiiSimdQuat& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

XII_ALWAYS_INLINE bool xiiSimdQuat::operator!=(const xiiSimdQuat& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
