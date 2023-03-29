#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/PathComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// TODO PathComponent:
// tangent mode for each node (auto, linear, Bezier)
// linked tangents on/off
// editing tangents (in a plane, needs new manipulator)


// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiPathComponentFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiPathComponentFlags::VisualizePath, xiiPathComponentFlags::VisualizeUpDir)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiEventMsgPathChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventMsgPathChanged, 1, xiiRTTIDefaultAllocator<xiiEventMsgPathChanged>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPathComponent::ControlPoint::Serialize(xiiStreamWriter& s) const
{
  s << m_vPosition;
  s << m_vTangentIn;
  s << m_vTangentOut;
  s << m_Roll;

  return XII_SUCCESS;
}

xiiResult xiiPathComponent::ControlPoint::Deserialize(xiiStreamReader& s)
{
  s >> m_vPosition;
  s >> m_vTangentIn;
  s >> m_vTangentOut;
  s >> m_Roll;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPathComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY("Flags", xiiPathComponentFlags, GetPathFlags, SetPathFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiPathComponentFlags::VisualizePath)),
    XII_ACCESSOR_PROPERTY("Closed", GetClosed,SetClosed),
    XII_ACCESSOR_PROPERTY("Detail", GetLinearizationError, SetLinearizationError)->AddAttributes(new xiiDefaultValueAttribute(0.01f), new xiiClampValueAttribute(1.0f, 0.001f)),
    XII_ARRAY_ACCESSOR_PROPERTY("Nodes", Nodes_GetCount, Nodes_GetNode, Nodes_SetNode, Nodes_Insert, Nodes_Remove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiEventMsgPathChanged, OnEventMsgPathChanged),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation/Paths"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiPathComponent::xiiPathComponent() = default;
xiiPathComponent::~xiiPathComponent() = default;

void xiiPathComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_PathFlags;
  s << m_bClosed;
  s << m_fLinearizationError;

  if (m_bControlPointsChanged && !m_bDisableControlPointUpdates)
  {
    xiiDynamicArray<ControlPoint> controlPoints;
    FindControlPoints(controlPoints);
    stream.GetStream().WriteArray(controlPoints).AssertSuccess();
  }
  else
  {
    stream.GetStream().WriteArray(m_ControlPointRepresentation).AssertSuccess();
  }
}

void xiiPathComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();
  s >> m_PathFlags;
  s >> m_bClosed;
  s >> m_fLinearizationError;

  stream.GetStream().ReadArray(m_ControlPointRepresentation).AssertSuccess();

  m_bDisableControlPointUpdates = true;
  m_bControlPointsChanged = false;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::SetClosed(bool bClosed)
{
  if (m_bClosed == bClosed)
    return;

  m_bClosed = bClosed;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::SetPathFlags(xiiBitflags<xiiPathComponentFlags> flags)
{
  if (m_PathFlags == flags)
    return;

  m_PathFlags = flags;

  if (IsActiveAndInitialized())
  {
    if (m_PathFlags.IsNoFlagSet())
      static_cast<xiiPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
    else
      static_cast<xiiPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void xiiPathComponent::Nodes_SetNode(xiiUInt32 i, const xiiString& node)
{
  m_Nodes[i] = node;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::Nodes_Insert(xiiUInt32 uiIndex, const xiiString& node)
{
  m_Nodes.Insert(node, uiIndex);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::Nodes_Remove(xiiUInt32 uiIndex)
{
  m_Nodes.RemoveAtAndCopy(uiIndex);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::FindControlPoints(xiiDynamicArray<ControlPoint>& out_ControlPoints) const
{
  auto& points = out_ControlPoints;

  points.Clear();

  if (m_Nodes.GetCount() <= 1)
    return;

  xiiGameObject* pOwner = const_cast<xiiGameObject*>(GetOwner());
  const xiiTransform invTrans = pOwner->GetGlobalTransform().GetInverse();

  xiiHybridArray<xiiPathNodeTangentMode::StorageType, 64> tangentsIn;
  xiiHybridArray<xiiPathNodeTangentMode::StorageType, 64> tangentsOut;

  for (const xiiString& sNode : m_Nodes)
  {
    const xiiGameObject* pNodeObj = pOwner->FindChildByName(xiiTempHashedString(sNode), false);
    if (pNodeObj == nullptr)
      continue;

    const xiiPathNodeComponent* pNodeComp;
    if (!pNodeObj->TryGetComponentOfBaseType(pNodeComp))
      continue;

    auto& cp = points.ExpandAndGetRef();
    cp.m_vPosition = invTrans * pNodeObj->GetGlobalPosition();
    cp.m_Roll = pNodeComp->GetRoll();

    tangentsOut.PushBack(pNodeComp->GetTangentMode1().GetValue());
    tangentsIn.PushBack(pNodeComp->GetTangentMode2().GetValue());
  }

  const xiiUInt32 uiNumPoints = points.GetCount();
  const xiiUInt32 uiLastIdx = points.GetCount() - 1;

  if (uiNumPoints <= 1)
  {
    points.Clear();
    return;
  }

  xiiUInt32 uiNumTangentsToUpdate = uiNumPoints;
  xiiUInt32 uiPrevIdx = uiLastIdx - 1;
  xiiUInt32 uiCurIdx = uiLastIdx;
  xiiUInt32 uiNextIdx = 0;

  if (!m_bClosed)
  {
    const xiiVec3 vStartTangent = (points[1].m_vPosition - points[0].m_vPosition) * 0.3333333333f;
    const xiiVec3 vEndTangent = (points[uiLastIdx].m_vPosition - points[uiLastIdx - 1].m_vPosition) * 0.3333333333f;

    points[0].m_vTangentIn = vStartTangent;
    points[0].m_vTangentOut = -vStartTangent;

    points[uiLastIdx].m_vTangentIn = vEndTangent;
    points[uiLastIdx].m_vTangentOut = -vEndTangent;

    uiNumTangentsToUpdate = uiNumPoints - 2;
    uiPrevIdx = 0;
    uiCurIdx = 1;
    uiNextIdx = 2;
  }

  for (xiiUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
  {
    auto& tCP = points[uiCurIdx];
    const auto& pCP = points[uiPrevIdx];
    const auto& nCP = points[uiNextIdx];

    const float fLength = xiiMath::Max(0.001f, (nCP.m_vPosition - pCP.m_vPosition).GetLength());
    const float fLerpFactor = xiiMath::Min(1.0f, (tCP.m_vPosition - pCP.m_vPosition).GetLength() / fLength);

    const xiiVec3 dirP = (tCP.m_vPosition - pCP.m_vPosition) * 0.3333333333f;
    const xiiVec3 dirN = (nCP.m_vPosition - tCP.m_vPosition) * 0.3333333333f;

    const xiiVec3 tangent = xiiMath::Lerp(dirP, dirN, fLerpFactor);

    switch (tangentsIn[uiCurIdx])
    {
      case xiiPathNodeTangentMode::Auto:
        tCP.m_vTangentIn = tangent;
        break;
      case xiiPathNodeTangentMode::Linear:
        tCP.m_vTangentIn = dirN;
        break;
    }

    switch (tangentsOut[uiCurIdx])
    {
      case xiiPathNodeTangentMode::Auto:
        tCP.m_vTangentOut = -tangent;
        break;
      case xiiPathNodeTangentMode::Linear:
        tCP.m_vTangentOut = -dirP;
        break;
    }

    uiPrevIdx = uiCurIdx;
    uiCurIdx = uiNextIdx;
    ++uiNextIdx;
  }
}

void xiiPathComponent::EnsureControlPointRepresentationIsUpToDate()
{
  if (!m_bControlPointsChanged || m_bDisableControlPointUpdates)
    return;

  m_ControlPointRepresentation.Clear();

  if (!IsActive())
    return;

  FindControlPoints(m_ControlPointRepresentation);
  m_bControlPointsChanged = false;
}

void xiiPathComponent::EnsureLinearizedRepresentationIsUpToDate()
{
  if (!m_bLinearizedRepresentationChanged)
    return;

  m_LinearizedRepresentation.Clear();

  if (!IsActive())
    return;

  EnsureControlPointRepresentationIsUpToDate();

  CreateLinearizedPathRepresentation(m_ControlPointRepresentation);
  m_bLinearizedRepresentationChanged = false;
}

void xiiPathComponent::OnEventMsgPathChanged(xiiEventMsgPathChanged& msg)
{
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void xiiPathComponent::DrawDebugVisualizations()
{
  if (m_PathFlags.AreNoneSet(xiiPathComponentFlags::VisualizePath | xiiPathComponentFlags::VisualizeUpDir))
    return;

  const bool bVisPath = m_PathFlags.IsSet(xiiPathComponentFlags::VisualizePath);
  const bool bVisUp = m_PathFlags.IsSet(xiiPathComponentFlags::VisualizeUpDir);

  EnsureLinearizedRepresentationIsUpToDate();

  if (m_LinearizedRepresentation.IsEmpty())
    return;

  xiiHybridArray<xiiDebugRenderer::Line, 32> lines;

  xiiUInt32 uiPrev = 0;
  xiiUInt32 uiNext = 1;

  for (; uiNext < m_LinearizedRepresentation.GetCount(); ++uiNext)
  {
    const auto& n0 = m_LinearizedRepresentation[uiPrev];
    const auto& n1 = m_LinearizedRepresentation[uiNext];

    if (bVisPath)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n1.m_vPosition;
      line.m_startColor = xiiColor::DarkRed;
      line.m_endColor = xiiColor::DarkRed;
    }

    if (bVisUp)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n0.m_vPosition + n0.m_vUpDirection * 0.25f;
      line.m_startColor = xiiColor::Black;
      line.m_endColor = xiiColor::LightBlue;
    }

    uiPrev = uiNext;
  }

  xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::White, GetOwner()->GetGlobalTransform());
}

void xiiPathComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    static_cast<xiiPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void xiiPathComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    // assume that if no flag is set, update is already disabled
    static_cast<xiiPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
  }
}

void xiiPathComponent::LinearSampler::SetToStart()
{
  m_fSegmentFraction = 0.0f;
  m_uiSegmentNode = 0;
}

void xiiPathComponent::SetLinearSamplerTo(LinearSampler& sampler, float fDistance) const
{
  if (fDistance < 0.0f && m_LinearizedRepresentation.GetCount() >= 2)
  {
    sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    sampler.m_fSegmentFraction = 1.0f;
  }
  else
  {
    sampler.m_uiSegmentNode = 0;
    sampler.m_fSegmentFraction = 0.0f;
  }

  AdvanceLinearSamplerBy(sampler, fDistance);
}

bool xiiPathComponent::AdvanceLinearSamplerBy(LinearSampler& sampler, float& inout_fAddDistance) const
{
  if (inout_fAddDistance == 0.0f || m_LinearizedRepresentation.IsEmpty())
  {
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (m_LinearizedRepresentation.GetCount() == 1)
  {
    sampler.SetToStart();
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (inout_fAddDistance >= 0)
  {
    for (xiiUInt32 i = sampler.m_uiSegmentNode + 1; i < m_LinearizedRepresentation.GetCount(); ++i)
    {
      const auto& nd0 = m_LinearizedRepresentation[i - 1];
      const auto& nd1 = m_LinearizedRepresentation[i];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = fSegmentLength - fSegmentDistance;

      if (inout_fAddDistance >= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;
        sampler.m_uiSegmentNode = i;
        sampler.m_fSegmentFraction = 0.0f;
      }
      else
      {
        sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }

    sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    sampler.m_fSegmentFraction = 1.0f;
    return false;
  }
  else
  {
    while (true)
    {
      xiiUInt32 ic = sampler.m_uiSegmentNode;
      xiiUInt32 in = xiiMath::Min(sampler.m_uiSegmentNode + 1, m_LinearizedRepresentation.GetCount() - 1);

      const auto& nd0 = m_LinearizedRepresentation[ic];
      const auto& nd1 = m_LinearizedRepresentation[in];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = -fSegmentDistance;

      if (inout_fAddDistance <= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;

        if (sampler.m_uiSegmentNode == 0)
        {
          sampler.m_uiSegmentNode = 0;
          sampler.m_fSegmentFraction = 0.0f;
          return false;
        }

        sampler.m_uiSegmentNode--;
        sampler.m_fSegmentFraction = 1.0f;
      }
      else
      {
        sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }
  }
}

xiiPathComponent::LinearizedElement xiiPathComponent::SampleLinearizedRepresentation(const LinearSampler& sampler) const
{
  if (m_LinearizedRepresentation.IsEmpty())
    return {};

  if (sampler.m_uiSegmentNode + 1 >= m_LinearizedRepresentation.GetCount())
  {
    const xiiUInt32 idx = m_LinearizedRepresentation.GetCount() - 1;

    return m_LinearizedRepresentation[idx];
  }

  const auto& nd0 = m_LinearizedRepresentation[sampler.m_uiSegmentNode];
  const auto& nd1 = m_LinearizedRepresentation[sampler.m_uiSegmentNode + 1];

  LinearizedElement res;
  res.m_vPosition = xiiMath::Lerp(nd0.m_vPosition, nd1.m_vPosition, sampler.m_fSegmentFraction);
  res.m_vUpDirection = xiiMath::Lerp(nd0.m_vUpDirection, nd1.m_vUpDirection, sampler.m_fSegmentFraction);

  return res;
}

void xiiPathComponent::SetLinearizationError(float fError)
{
  if (m_fLinearizationError == fError)
    return;

  m_fLinearizationError = fError;
  m_bLinearizedRepresentationChanged = true;
}

static void ComputeCpDirs(const xiiDynamicArray<xiiPathComponent::ControlPoint>& points, bool bClosed, const xiiCoordinateSystem& cs, xiiDynamicArray<xiiVec3>& inout_cpFwd, xiiDynamicArray<xiiVec3>& inout_cpUp)
{
  const xiiUInt32 uiNumCPs = points.GetCount();
  inout_cpFwd.SetCount(uiNumCPs);
  inout_cpUp.SetCount(uiNumCPs);

  for (xiiUInt32 uiCurPt = 0; uiCurPt < uiNumCPs; ++uiCurPt)
  {
    xiiUInt32 uiPrevPt;
    xiiUInt32 uiNextPt = uiCurPt + 1;

    if (bClosed)
    {
      if (uiCurPt == 0)
        uiPrevPt = uiNumCPs - 1;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt %= uiNumCPs;
    }
    else
    {
      if (uiCurPt == 0)
        uiPrevPt = 0;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt = xiiMath::Min(uiCurPt + 1, uiNumCPs - 1);
    }

    const auto& cpP = points[uiPrevPt];
    const auto& cpC = points[uiCurPt];
    const auto& cpN = points[uiNextPt];

    const xiiVec3 posPrev = xiiMath::EvaluateBezierCurve(0.98f, cpP.m_vPosition, cpP.m_vPosition + cpP.m_vTangentIn, cpC.m_vPosition + cpC.m_vTangentOut, cpC.m_vPosition);
    const xiiVec3 posNext = xiiMath::EvaluateBezierCurve(0.02f, cpC.m_vPosition, cpC.m_vPosition + cpC.m_vTangentIn, cpN.m_vPosition + cpN.m_vTangentOut, cpN.m_vPosition);

    xiiVec3 dirP = (posPrev - cpC.m_vPosition);
    xiiVec3 dirN = (posNext - cpC.m_vPosition);
    dirP.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();
    dirN.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();

    xiiVec3 dirAvg = dirP - dirN;
    dirAvg.NormalizeIfNotZero(cs.m_vForwardDir).IgnoreResult();

    xiiVec3 dirUp = cs.m_vUpDir;
    dirUp.MakeOrthogonalTo(dirAvg);

    dirUp.NormalizeIfNotZero(cs.m_vUpDir).IgnoreResult();

    inout_cpFwd[uiCurPt] = dirAvg;
    inout_cpUp[uiCurPt] = dirUp;
  }
}

static double ComputePathLength(xiiArrayPtr<xiiPathComponent::LinearizedElement> points)
{
  double fLength = 0;
  for (xiiUInt32 i = 1; i < points.GetCount(); ++i)
  {
    fLength += (points[i - 1].m_vPosition - points[i].m_vPosition).GetLength();
  }

  return fLength;
}

static xiiVec3 ComputeTangentAt(float fT, const xiiPathComponent::ControlPoint& cp0, const xiiPathComponent::ControlPoint& cp1)
{
  const xiiVec3 posPrev = xiiMath::EvaluateBezierCurve(xiiMath::Max(0.0f, fT - 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);
  const xiiVec3 posNext = xiiMath::EvaluateBezierCurve(xiiMath::Min(1.0f, fT + 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  return (posNext - posPrev).GetNormalized();
}

static void InsertHalfPoint(xiiDynamicArray<xiiPathComponent::LinearizedElement>& result, xiiDynamicArray<xiiVec3>& tangents, const xiiPathComponent::ControlPoint& cp0, const xiiPathComponent::ControlPoint& cp1, float fLowerT, float fUpperT, const xiiVec3& vLowerPos, const xiiVec3& vUpperPos, float fDistSqr, xiiInt32 iMinSteps, xiiInt32 iMaxSteps)
{
  const float fHalfT = xiiMath::Lerp(fLowerT, fUpperT, 0.5f);

  const xiiVec3 vHalfPos = xiiMath::EvaluateBezierCurve(fHalfT, cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  if (iMinSteps <= 0)
  {
    const xiiVec3 vInterpPos = xiiMath::Lerp(vLowerPos, vUpperPos, 0.5f);

    if ((vHalfPos - vInterpPos).GetLengthSquared() < fDistSqr)
    {
      return;
    }
  }

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(result, tangents, cp0, cp1, fLowerT, fHalfT, vLowerPos, vHalfPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }

  result.ExpandAndGetRef().m_vPosition = vHalfPos;
  tangents.ExpandAndGetRef() = ComputeTangentAt(fHalfT, cp0, cp1);

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(result, tangents, cp0, cp1, fHalfT, fUpperT, vHalfPos, vUpperPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }
}

static void GeneratePathSegment(xiiUInt32 uiCp0, xiiUInt32 uiCp1, xiiArrayPtr<const xiiPathComponent::ControlPoint> points, xiiArrayPtr<xiiVec3> cpUp, xiiArrayPtr<xiiVec3> cpFwd, xiiDynamicArray<xiiPathComponent::LinearizedElement>& result, xiiDynamicArray<xiiVec3>& tangents, float fDistSqr)
{
  tangents.Clear();

  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  xiiInt32 iRollDiv = 0;
  float fToRoll = xiiMath::Abs((cp1.m_Roll - cp0.m_Roll).GetDegree());
  while (fToRoll > 45.0f)
  {
    fToRoll *= 0.5f;
    iRollDiv++;
  }

  result.ExpandAndGetRef().m_vPosition = cp0.m_vPosition;
  tangents.ExpandAndGetRef() = -cpFwd[uiCp0];

  InsertHalfPoint(result, tangents, cp0, cp1, 0.0f, 1.0f, cp0.m_vPosition, cp1.m_vPosition, fDistSqr, xiiMath::Max(1, iRollDiv), 7);

  result.ExpandAndGetRef().m_vPosition = cp1.m_vPosition;
  tangents.ExpandAndGetRef() = -cpFwd[uiCp1];
}

static void ComputeSegmentUpVector(xiiArrayPtr<xiiPathComponent::LinearizedElement> segmentElements, xiiUInt32 uiCp0, xiiUInt32 uiCp1, const xiiArrayPtr<const xiiPathComponent::ControlPoint> points, const xiiArrayPtr<const xiiVec3> cpUp, const xiiArrayPtr<const xiiVec3> tangents, const xiiVec3& vWorldUp)
{
  const double fSegmentLength = ComputePathLength(segmentElements);
  const double fInvSegmentLength = 1.0 / fSegmentLength;

  double fCurDist = 0.0;
  xiiVec3 vPrevPos = segmentElements[0].m_vPosition;

  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  const xiiVec3 cp0up = cpUp[uiCp0];
  const xiiVec3 cp1up = cpUp[uiCp1];

  for (xiiUInt32 t = 0; t < segmentElements.GetCount(); ++t)
  {
    fCurDist += (segmentElements[t].m_vPosition - vPrevPos).GetLength();
    vPrevPos = segmentElements[t].m_vPosition;

    const float fLerpFactor = (float)(fCurDist * fInvSegmentLength);

    const xiiAngle roll = xiiMath::Lerp(cp0.m_Roll, cp1.m_Roll, fLerpFactor);

    xiiQuat qRoll;
    qRoll.SetFromAxisAndAngle(tangents[t], roll);

    xiiVec3 vLocalUp = xiiMath::Lerp(cp0up, cp1up, fLerpFactor);
    vLocalUp.NormalizeIfNotZero(vWorldUp).IgnoreResult();

    segmentElements[t].m_vUpDirection = qRoll * vLocalUp;
  }
}

void xiiPathComponent::CreateLinearizedPathRepresentation(const xiiDynamicArray<ControlPoint>& points)
{
  m_LinearizedRepresentation.Clear();

  const xiiUInt32 uiNumCPs = points.GetCount();

  if (uiNumCPs <= 1)
    return;

  xiiHybridArray<xiiVec3, 64> cpUp;
  xiiHybridArray<xiiVec3, 64> cpFwd;

  xiiCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);

  ComputeCpDirs(points, m_bClosed, cs, cpFwd, cpUp);

  const xiiUInt32 uiNumCPsToUse = m_bClosed ? uiNumCPs + 1 : uiNumCPs;

  xiiHybridArray<xiiVec3, 64> tangents;

  for (xiiUInt32 uiCurPt = 1; uiCurPt < uiNumCPsToUse; ++uiCurPt)
  {
    const xiiUInt32 uiCp0 = uiCurPt - 1;
    const xiiUInt32 uiCp1 = uiCurPt % uiNumCPs;

    const xiiUInt32 uiFirstNewNode = m_LinearizedRepresentation.GetCount();
    GeneratePathSegment(uiCp0, uiCp1, points, cpUp, cpFwd, m_LinearizedRepresentation, tangents, xiiMath::Square(m_fLinearizationError));

    xiiArrayPtr<xiiPathComponent::LinearizedElement> segmentElements = m_LinearizedRepresentation.GetArrayPtr().GetSubArray(uiFirstNewNode);

    ComputeSegmentUpVector(segmentElements, uiCp0, uiCp1, points, cpUp, tangents, cs.m_vUpDir);
  }

  m_fLinearizedLength = (float)ComputePathLength(m_LinearizedRepresentation);
}



//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPathNodeTangentMode, 1)
  XII_ENUM_CONSTANTS(xiiPathNodeTangentMode::Auto, xiiPathNodeTangentMode::Linear)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiPathNodeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Roll", GetRoll, SetRoll),
    XII_ENUM_ACCESSOR_PROPERTY("Tangent1", xiiPathNodeTangentMode, GetTangentMode1, SetTangentMode1),
    XII_ENUM_ACCESSOR_PROPERTY("Tangent2", xiiPathNodeTangentMode, GetTangentMode2, SetTangentMode2),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgTransformChanged, OnMsgTransformChanged),
    XII_MESSAGE_HANDLER(xiiMsgParentChanged, OnMsgParentChanged),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation/Paths"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiPathNodeComponent::xiiPathNodeComponent() = default;
xiiPathNodeComponent::~xiiPathNodeComponent() = default;

void xiiPathNodeComponent::SetRoll(xiiAngle roll)
{
  if (m_Roll != roll)
  {
    m_Roll = roll;
    PathChanged();
  }
}

void xiiPathNodeComponent::SetTangentMode1(xiiEnum<xiiPathNodeTangentMode> mode)
{
  if (m_TangentMode1 != mode)
  {
    m_TangentMode1 = mode;
    PathChanged();
  }
}

void xiiPathNodeComponent::SetTangentMode2(xiiEnum<xiiPathNodeTangentMode> mode)
{
  if (m_TangentMode2 != mode)
  {
    m_TangentMode2 = mode;
    PathChanged();
  }
}

void xiiPathNodeComponent::OnMsgTransformChanged(xiiMsgTransformChanged& msg)
{
  PathChanged();
}

void xiiPathNodeComponent::OnMsgParentChanged(xiiMsgParentChanged& msg)
{
  if (msg.m_Type == xiiMsgParentChanged::Type::ParentUnlinked)
  {
    xiiGameObject* pOldParent = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hParent, pOldParent))
    {
      xiiEventMsgPathChanged msg2;
      pOldParent->SendEventMessage(msg2, this);
    }
  }
  else
  {
    PathChanged();
  }
}

void xiiPathNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();

  PathChanged();
}

void xiiPathNodeComponent::OnDeactivated()
{
  xiiEventMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);

  SUPER::OnDeactivated();
}

void xiiPathNodeComponent::PathChanged()
{
  if (!IsActiveAndInitialized())
    return;

  xiiEventMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);
}

//////////////////////////////////////////////////////////////////////////

xiiPathComponentManager::xiiPathComponentManager(xiiWorld* pWorld)
  : xiiComponentManager(pWorld)
{
}

void xiiPathComponentManager::SetEnableUpdate(xiiPathComponent* pThis, bool bEnable)
{
  if (bEnable)
  {
    if (!m_NeedUpdate.Contains(pThis))
      m_NeedUpdate.PushBack(pThis);
  }
  else
  {
    m_NeedUpdate.RemoveAndSwap(pThis);
  }
}

void xiiPathComponentManager::Initialize()
{
  auto desc = xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&xiiPathComponentManager::Update, this), "xiiPathComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void xiiPathComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (xiiPathComponent* pComponent : m_NeedUpdate)
  {
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->DrawDebugVisualizations();
    }
  }
}
