#pragma once

XII_ALWAYS_INLINE xiiSimdQuatd::xiiSimdQuatd() = default;

XII_ALWAYS_INLINE xiiSimdQuatd::xiiSimdQuatd(const xiiSimdVec4d& v)
{
  m_v = v;
}

// static
XII_ALWAYS_INLINE xiiSimdQuatd xiiSimdQuatd::IdentityQuaternion()
{
  return xiiSimdQuatd(xiiSimdVec4d(0.0, 0.0, 0.0, 1.0));
}

XII_ALWAYS_INLINE void xiiSimdQuatd::SetIdentity()
{
  m_v.Set(0.0, 0.0, 0.0, 1.0);
}

XII_ALWAYS_INLINE void xiiSimdQuatd::SetFromAxisAndAngle(const xiiSimdVec4d& vRotationAxis, const xiiSimdDouble& fAngle)
{
  ///\todo optimize
  const xiiAngled halfAngle = xiiAngled::Radian(fAngle) * 0.5;
  double          s         = xiiMath::Sin(halfAngle);
  double          c         = xiiMath::Cos(halfAngle);

  m_v = vRotationAxis * s;
  m_v.SetW(c);
}

XII_ALWAYS_INLINE void xiiSimdQuatd::Normalize()
{
  m_v.Normalize<4>();
}

inline xiiResult xiiSimdQuatd::GetRotationAxisAndAngle(xiiSimdVec4d& ref_vAxis, xiiSimdDouble& ref_fAngle, const xiiSimdDouble& fEpsilon) const
{
  ///\todo optimize
  const xiiAngled acos = xiiMath::ACos((double)m_v.w());
  const double    d    = xiiMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0, 0.0, 0.0, 0.0);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = acos * 2.0;

  return XII_SUCCESS;
}

XII_ALWAYS_INLINE xiiSimdMat4d xiiSimdQuatd::GetAsMat4() const
{
  const xiiSimdVec4d xyz       = m_v;
  const xiiSimdVec4d x2y2z2    = xyz + xyz;
  const xiiSimdVec4d xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const xiiSimdVec4d yy2_xx2_xx2 = xx2yy2zz2.Get<xiiSwizzle::YXXX>();
  const xiiSimdVec4d zz2_zz2_yy2 = xx2yy2zz2.Get<xiiSwizzle::ZZYX>();
  xiiSimdVec4d       diagonal    = xiiSimdVec4d(1.0) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(xiiSimdDouble::Zero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const xiiSimdVec4d x_y_x    = xyz.Get<xiiSwizzle::XYXX>();
  const xiiSimdVec4d y2_z2_z2 = x2y2z2.Get<xiiSwizzle::YZZX>();
  const xiiSimdVec4d base     = x_y_x.CompMul(y2_z2_z2);

  const xiiSimdVec4d z2_x2_y2 = x2y2z2.Get<xiiSwizzle::ZXYX>();
  const xiiSimdVec4d offset   = z2_x2_y2 * m_v.w();

  const xiiSimdVec4d adds = base + offset;
  const xiiSimdVec4d subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const xiiSimdVec4d addX_u_diaX_u = adds.GetCombined<xiiSwizzle::XXXX>(diagonal);
  const xiiSimdVec4d subZ_u_diaW_u = subs.GetCombined<xiiSwizzle::ZXWX>(diagonal);
  const xiiSimdVec4d col0          = addX_u_diaX_u.GetCombined<xiiSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const xiiSimdVec4d subX_u_diaY_u = subs.GetCombined<xiiSwizzle::XXYX>(diagonal);
  const xiiSimdVec4d addY_u_diaW_u = adds.GetCombined<xiiSwizzle::YXWX>(diagonal);
  const xiiSimdVec4d col1          = subX_u_diaY_u.GetCombined<xiiSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const xiiSimdVec4d addZ_u_subY_u = adds.GetCombined<xiiSwizzle::ZXYX>(subs);
  const xiiSimdVec4d col2          = addZ_u_subY_u.GetCombined<xiiSwizzle::XZZW>(diagonal);

  return xiiSimdMat4d(col0, col1, col2, xiiSimdVec4d(0, 0, 0, 1));
}

XII_ALWAYS_INLINE bool xiiSimdQuatd::IsValid(const xiiSimdDouble& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdQuatd::IsNaN() const
{
  return m_v.IsNaN<4>();
}

XII_ALWAYS_INLINE xiiSimdQuatd xiiSimdQuatd::operator-() const
{
  return m_v.FlipSign(xiiSimdVec4b(true, true, true, false));
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdQuatd::operator*(const xiiSimdVec4d& v) const
{
  xiiSimdVec4d t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

XII_ALWAYS_INLINE xiiSimdQuatd xiiSimdQuatd::operator*(const xiiSimdQuatd& q2) const
{
  xiiSimdQuatd q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

XII_ALWAYS_INLINE bool xiiSimdQuatd::operator==(const xiiSimdQuatd& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

XII_ALWAYS_INLINE bool xiiSimdQuatd::operator!=(const xiiSimdQuatd& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
