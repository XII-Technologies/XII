/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/Curve1D.h>

xiiCurve1D::ControlPoint::ControlPoint()
{
  m_Position.SetZero();
  m_LeftTangent.SetZero();
  m_RightTangent.SetZero();
  m_uiOriginalIndex = 0;
}

xiiCurve1D::xiiCurve1D()
{
  Clear();
}

void xiiCurve1D::Clear()
{
  m_fMinX = 0;
  m_fMaxX = 0;
  m_fMinY = 0;
  m_fMaxY = 0;

  m_ControlPoints.Clear();
}

bool xiiCurve1D::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

xiiCurve1D::ControlPoint& xiiCurve1D::AddControlPoint(double x)
{
  auto& cp             = m_ControlPoints.ExpandAndGetRef();
  cp.m_uiOriginalIndex = static_cast<xiiUInt16>(m_ControlPoints.GetCount() - 1);
  cp.m_Position.x      = x;
  cp.m_Position.y      = 0;
  cp.m_LeftTangent.x   = -0.1f;
  cp.m_LeftTangent.y   = 0.0f;
  cp.m_RightTangent.x  = +0.1f;
  cp.m_RightTangent.y  = 0.0f;

  return cp;
}

void xiiCurve1D::QueryExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  ref_fMinx = m_fMinX;
  ref_fMaxx = m_fMaxX;
}

void xiiCurve1D::QueryExtremeValues(double& ref_fMinVal, double& ref_fMaxVal) const
{
  ref_fMinVal = m_fMinY;
  ref_fMaxVal = m_fMaxY;
}

xiiUInt32 xiiCurve1D::GetNumControlPoints() const
{
  return m_ControlPoints.GetCount();
}

void xiiCurve1D::SortControlPoints()
{
  m_ControlPoints.Sort();

  RecomputeExtents();
}

xiiInt32 xiiCurve1D::FindApproxControlPoint(double x) const
{
  xiiUInt32 uiLowIdx  = 0;
  xiiUInt32 uiHighIdx = m_LinearApproximation.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const xiiUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    // doesn't matter whether to use > or >=
    if (m_LinearApproximation[uiMidIdx].x > x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (xiiUInt32 idx = uiLowIdx; idx < uiHighIdx; ++idx)
  {
    if (m_LinearApproximation[idx].x >= x)
    {
      // when m_LinearApproximation[0].x >= x, we want to return -1
      return ((xiiInt32)idx) - 1;
    }
  }

  // return last index
  return (xiiInt32)uiHighIdx - 1;
}

double xiiCurve1D::Evaluate(double x) const
{
  XII_ASSERT_DEBUG(!m_LinearApproximation.IsEmpty(), "Cannot evaluate curve without precomputing curve approximation data first. Call CreateLinearApproximation() on curve before calling Evaluate().");

  if (m_LinearApproximation.GetCount() >= 2)
  {
    const xiiUInt32 numCPs        = m_LinearApproximation.GetCount();
    const xiiInt32  iControlPoint = FindApproxControlPoint(x);

    if (iControlPoint < 0)
    {
      // clamp to left value
      return m_LinearApproximation[0].y;
    }
    else if (xiiUInt32(iControlPoint) == numCPs - 1)
    {
      // clamp to right value
      return m_LinearApproximation[numCPs - 1].y;
    }
    else
    {
      const double v1 = m_LinearApproximation[iControlPoint].y;
      const double v2 = m_LinearApproximation[iControlPoint + 1].y;

      // interpolate
      double       lerpX = x - m_LinearApproximation[iControlPoint].x;
      const double len   = (m_LinearApproximation[iControlPoint + 1].x - m_LinearApproximation[iControlPoint].x);

      if (len <= 0)
        lerpX = 0;
      else
        lerpX /= len; // TODO remove division ?

      return xiiMath::Lerp(v1, v2, lerpX);
    }
  }
  else if (m_LinearApproximation.GetCount() == 1)
  {
    return m_LinearApproximation[0].y;
  }

  return 0;
}

double xiiCurve1D::ConvertNormalizedPos(double fPos) const
{
  double fMin, fMax;
  QueryExtents(fMin, fMax);

  return xiiMath::Lerp(fMin, fMax, fPos);
}


double xiiCurve1D::NormalizeValue(double value) const
{
  double fMin, fMax;
  QueryExtremeValues(fMin, fMax);

  if (fMin >= fMax)
    return 0;

  return (value - fMin) / (fMax - fMin);
}

xiiUInt64 xiiCurve1D::GetHeapMemoryUsage() const
{
  return m_ControlPoints.GetHeapMemoryUsage();
}

void xiiCurve1D::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 4;

  ref_stream << uiVersion;

  const xiiUInt32 numCp = m_ControlPoints.GetCount();

  ref_stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    ref_stream << cp.m_Position;
    ref_stream << cp.m_LeftTangent;
    ref_stream << cp.m_RightTangent;
    ref_stream << cp.m_TangentModeRight;
    ref_stream << cp.m_TangentModeLeft;
  }
}

void xiiCurve1D::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;

  ref_stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion <= 4, "Incorrect version '{0}' for xiiCurve1D", uiVersion);

  xiiUInt32 numCp = 0;

  ref_stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  if (uiVersion <= 2)
  {
    for (auto& cp : m_ControlPoints)
    {
      xiiVec2 pos;
      ref_stream >> pos;
      cp.m_Position.Set(pos.x, pos.y);

      if (uiVersion >= 2)
      {
        ref_stream >> cp.m_LeftTangent;
        ref_stream >> cp.m_RightTangent;
      }
    }
  }
  else
  {
    for (auto& cp : m_ControlPoints)
    {
      ref_stream >> cp.m_Position;
      ref_stream >> cp.m_LeftTangent;
      ref_stream >> cp.m_RightTangent;

      if (uiVersion >= 4)
      {
        ref_stream >> cp.m_TangentModeRight;
        ref_stream >> cp.m_TangentModeLeft;
      }
    }
  }
}

void xiiCurve1D::CreateLinearApproximation(double fMaxError /*= 0.01f*/, xiiUInt8 uiMaxSubDivs /*= 8*/)
{
  m_LinearApproximation.Clear();

  /// \todo Since we do this, we actually don't need the linear approximation anymore and could just evaluate the full curve
  ApplyTangentModes();

  ClampTangents();

  if (m_ControlPoints.IsEmpty())
  {
    m_LinearApproximation.PushBack(xiiVec2d::MakeZero());
    return;
  }

  for (xiiUInt32 i = 1; i < m_ControlPoints.GetCount(); ++i)
  {
    XII_ASSERT_DEBUG(m_ControlPoints[i - 1].m_Position.x <= m_ControlPoints[i].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    double fMinY, fMaxY;
    ApproximateMinMaxValues(m_ControlPoints[i - 1], m_ControlPoints[i], fMinY, fMaxY);

    const double rangeY     = xiiMath::Max(0.1, fMaxY - fMinY);
    const double fMaxErrorY = fMaxError * rangeY;
    const double fMaxErrorX = (m_ControlPoints[i].m_Position.x - m_ControlPoints[i - 1].m_Position.x) * fMaxError;


    m_LinearApproximation.PushBack(m_ControlPoints[i - 1].m_Position);

    ApproximateCurve(m_ControlPoints[i - 1].m_Position,
                     m_ControlPoints[i - 1].m_Position + xiiVec2d(m_ControlPoints[i - 1].m_RightTangent.x, m_ControlPoints[i - 1].m_RightTangent.y),
                     m_ControlPoints[i].m_Position + xiiVec2d(m_ControlPoints[i].m_LeftTangent.x, m_ControlPoints[i].m_LeftTangent.y),
                     m_ControlPoints[i].m_Position,
                     fMaxErrorX,
                     fMaxErrorY,
                     uiMaxSubDivs);
  }

  m_LinearApproximation.PushBack(m_ControlPoints.PeekBack().m_Position);

  RecomputeLinearApproxExtremes();
}

void xiiCurve1D::RecomputeExtents()
{
  m_fMinX = xiiMath::MaxValue<float>();
  m_fMaxX = -xiiMath::MaxValue<float>();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = xiiMath::Min(m_fMinX, cp.m_Position.x);
    m_fMaxX = xiiMath::Max(m_fMaxX, cp.m_Position.x);

    // ignore X values that could go outside the control point range due to Bezier curve interpolation
    // we just assume the curve is always restricted along X by the CPs

    // m_fMinX = xiiMath::Min(m_fMinX, cp.m_Position.x + cp.m_LeftTangent.x);
    // m_fMaxX = xiiMath::Max(m_fMaxX, cp.m_Position.x + cp.m_LeftTangent.x);

    // m_fMinX = xiiMath::Min(m_fMinX, cp.m_Position.x + cp.m_RightTangent.x);
    // m_fMaxX = xiiMath::Max(m_fMaxX, cp.m_Position.x + cp.m_RightTangent.x);
  }
}


void xiiCurve1D::RecomputeLinearApproxExtremes()
{
  m_fMinY = xiiMath::MaxValue<float>();
  m_fMaxY = -xiiMath::MaxValue<float>();

  for (const auto& cp : m_LinearApproximation)
  {
    m_fMinY = xiiMath::Min(m_fMinY, cp.y);
    m_fMaxY = xiiMath::Max(m_fMaxY, cp.y);
  }
}

void xiiCurve1D::ApproximateMinMaxValues(const ControlPoint& lhs, const ControlPoint& rhs, double& fMinY, double& fMaxY)
{
  fMinY = xiiMath::Min(lhs.m_Position.y, rhs.m_Position.y);
  fMaxY = xiiMath::Max(lhs.m_Position.y, rhs.m_Position.y);

  fMinY = xiiMath::Min(fMinY, lhs.m_Position.y + lhs.m_RightTangent.y);
  fMaxY = xiiMath::Max(fMaxY, lhs.m_Position.y + lhs.m_RightTangent.y);

  fMinY = xiiMath::Min(fMinY, rhs.m_Position.y + rhs.m_LeftTangent.y);
  fMaxY = xiiMath::Max(fMaxY, rhs.m_Position.y + rhs.m_LeftTangent.y);
}

void xiiCurve1D::ApproximateCurve(const xiiVec2d& p0, const xiiVec2d& p1, const xiiVec2d& p2, const xiiVec2d& p3, double fMaxErrorX, double fMaxErrorY, xiiInt32 iSubDivLeft)
{
  const xiiVec2d cubicCenter = xiiMath::EvaluateBezierCurve(0.5, p0, p1, p2, p3);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.0f, p0, 0.5, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft);

  // always insert the center point
  // with an S curve the cubicCenter and the linearCenter can be identical even though the rest of the curve is absolutely not linear
  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.5, cubicCenter, 1.0, p3, fMaxErrorX, fMaxErrorY, iSubDivLeft);
}

void xiiCurve1D::ApproximateCurvePiece(const xiiVec2d& p0, const xiiVec2d& p1, const xiiVec2d& p2, const xiiVec2d& p3, double tLeft, const xiiVec2d& pLeft, double tRight, const xiiVec2d& pRight, double fMaxErrorX, double fMaxErrorY, xiiInt32 iSubDivLeft)
{
  // this is a safe guard
  if (iSubDivLeft <= 0)
    return;

  const double tCenter = xiiMath::Lerp(tLeft, tRight, 0.5);

  const xiiVec2d cubicCenter  = xiiMath::EvaluateBezierCurve(tCenter, p0, p1, p2, p3);
  const xiiVec2d linearCenter = xiiMath::Lerp(pLeft, pRight, 0.5);

  // check whether the linear interpolation between pLeft and pRight would already result in a good enough approximation
  // if not, subdivide the curve further

  const double fThisErrorX = xiiMath::Abs(cubicCenter.x - linearCenter.x);
  const double fThisErrorY = xiiMath::Abs(cubicCenter.y - linearCenter.y);

  if (fThisErrorX < fMaxErrorX && fThisErrorY < fMaxErrorY)
    return;

  ApproximateCurvePiece(p0, p1, p2, p3, tLeft, pLeft, tCenter, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);

  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, tCenter, cubicCenter, tRight, pRight, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);
}

void xiiCurve1D::ClampTangents()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (xiiUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    auto&       tCP = m_ControlPoints[i];
    const auto& pCP = m_ControlPoints[i - 1];
    const auto& nCP = m_ControlPoints[i + 1];

    xiiVec2d lpt = tCP.m_Position + xiiVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    xiiVec2d rpt = tCP.m_Position + xiiVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);

    lpt.x = xiiMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);
    rpt.x = xiiMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const xiiVec2d tangentL = lpt - tCP.m_Position;
    const xiiVec2d tangentR = rpt - tCP.m_Position;

    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // first CP
  {
    auto&       tCP = m_ControlPoints[0];
    const auto& nCP = m_ControlPoints[1];

    xiiVec2d rpt = tCP.m_Position + xiiVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);
    rpt.x        = xiiMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const xiiVec2d tangentR = rpt - tCP.m_Position;
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // last CP
  {
    auto&       tCP = m_ControlPoints[m_ControlPoints.GetCount() - 1];
    const auto& pCP = m_ControlPoints[m_ControlPoints.GetCount() - 2];

    xiiVec2d lpt = tCP.m_Position + xiiVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    lpt.x        = xiiMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);

    const xiiVec2d tangentL = lpt - tCP.m_Position;
    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
  }
}

void xiiCurve1D::ApplyTangentModes()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (xiiUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    const auto& cp = m_ControlPoints[i];

    XII_ASSERT_DEBUG(cp.m_Position.x >= m_ControlPoints[i - 1].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");
    XII_ASSERT_DEBUG(m_ControlPoints[i + 1].m_Position.x >= cp.m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    if (cp.m_TangentModeLeft == xiiCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == xiiCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == xiiCurveTangentMode::Auto)
      MakeAutoTangentLeft(i);

    if (cp.m_TangentModeRight == xiiCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == xiiCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == xiiCurveTangentMode::Auto)
      MakeAutoTangentRight(i);
  }

  // first CP
  {
    const xiiUInt32 i  = 0;
    const auto&     cp = m_ControlPoints[i];

    if (cp.m_TangentModeRight == xiiCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == xiiCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == xiiCurveTangentMode::Auto)
      MakeLinearTangentRight(i); // note: first point will always be linear in auto mode
  }

  // last CP
  {
    const xiiUInt32 i  = m_ControlPoints.GetCount() - 1;
    const auto&     cp = m_ControlPoints[i];

    if (cp.m_TangentModeLeft == xiiCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == xiiCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == xiiCurveTangentMode::Auto)
      MakeLinearTangentLeft(i); // note: last point will always be linear in auto mode
  }
}

void xiiCurve1D::MakeFixedLengthTangentLeft(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const double lengthL = (pCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthL >= -0.0000001)
  {
    tCP.m_LeftTangent.SetZero();
  }
  else
  {
    const double tLen = xiiMath::Min((double)tCP.m_LeftTangent.x, -0.001);

    const double fNormL = lengthL / tLen;
    tCP.m_LeftTangent.x = (float)lengthL;
    tCP.m_LeftTangent.y *= (float)fNormL;
  }
}

void xiiCurve1D::MakeFixedLengthTangentRight(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double lengthR = (nCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthR <= 0.0000001)
  {
    tCP.m_RightTangent.SetZero();
  }
  else
  {
    const double tLen = xiiMath::Max((double)tCP.m_RightTangent.x, 0.001);

    const double fNormR  = lengthR / tLen;
    tCP.m_RightTangent.x = (float)lengthR;
    tCP.m_RightTangent.y *= (float)fNormR;
  }
}

void xiiCurve1D::MakeLinearTangentLeft(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const xiiVec2d tangent = (pCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_LeftTangent.Set((float)tangent.x, (float)tangent.y);
}

void xiiCurve1D::MakeLinearTangentRight(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const xiiVec2d tangent = (nCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

void xiiCurve1D::MakeAutoTangentLeft(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const xiiVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const xiiVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const xiiVec2d tangent = xiiMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_LeftTangent.Set(-(float)tangent.x, -(float)tangent.y);
}

void xiiCurve1D::MakeAutoTangentRight(xiiUInt32 uiCpIdx)
{
  auto&       tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0.0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const xiiVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const xiiVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const xiiVec2d tangent = xiiMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

XII_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_Curve1D);
