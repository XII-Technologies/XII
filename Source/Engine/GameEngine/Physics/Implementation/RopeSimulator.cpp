#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/RopeSimulator.h>

xiiRopeSimulator::xiiRopeSimulator()  = default;
xiiRopeSimulator::~xiiRopeSimulator() = default;

void xiiRopeSimulator::SimulateRope(const xiiTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr xiiTime  tStep         = xiiTime::Seconds(1.0 / 60.0);
  const xiiSimdFloat tStepSqr      = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());
  const xiiSimdFloat fAllowedError = m_fSegmentLength;

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, fAllowedError);

    m_LeftOverTimeStep -= tStep;
  }
}

void xiiRopeSimulator::SimulateStep(const xiiSimdFloat fDiffSqr, xiiUInt32 uiMaxIterations, xiiSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 2)
    return;

  UpdateNodePositions(fDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (xiiUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const xiiSimdFloat fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

void xiiRopeSimulator::SimulateTillEquilibrium(xiiSimdFloat fAllowedMovement, xiiUInt32 uiMaxIterations)
{
  constexpr xiiTime tStep    = xiiTime::Seconds(1.0 / 60.0);
  xiiSimdFloat      tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  xiiUInt8 uiInEquilibrium = 0;

  while (uiInEquilibrium < 100 && uiMaxIterations > 0)
  {
    --uiMaxIterations;

    SimulateStep(tStepSqr, 32, m_fSegmentLength);
    uiInEquilibrium++;

    if (!HasEquilibrium(fAllowedMovement))
    {
      uiInEquilibrium = 0;
    }
  }
}

bool xiiRopeSimulator::HasEquilibrium(xiiSimdFloat fAllowedMovement) const
{
  const xiiSimdFloat fErrorSqr = fAllowedMovement * fAllowedMovement;

  for (const auto& n : m_Nodes)
  {
    if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared<3>() > fErrorSqr)
    {
      return false;
    }
  }

  return true;
}

float xiiRopeSimulator::GetTotalLength() const
{
  if (m_Nodes.GetCount() <= 1)
    return 0.0f;

  float len = 0;

  xiiSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (xiiUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const xiiSimdVec4f cur = m_Nodes[i].m_vPosition;

    len += (cur - prev).GetLength<3>();

    prev = cur;
  }

  return len;
}

xiiSimdVec4f xiiRopeSimulator::GetPositionAtLength(float fLength) const
{
  if (m_Nodes.IsEmpty())
    return xiiSimdVec4f::ZeroVector();

  xiiSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (xiiUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const xiiSimdVec4f cur = m_Nodes[i].m_vPosition;

    const xiiSimdVec4f dir  = cur - prev;
    const float        dist = dir.GetLength<3>();

    if (fLength <= dist)
    {
      const float interpolate = fLength / dist;
      return prev + dir * interpolate;
    }

    fLength -= dist;
    prev = cur;
  }

  return m_Nodes.PeekBack().m_vPosition;
}

xiiSimdVec4f xiiRopeSimulator::MoveTowards(const xiiSimdVec4f posThis, const xiiSimdVec4f posNext, xiiSimdFloat factor, const xiiSimdVec4f fallbackDir, xiiSimdFloat& inout_fError)
{
  xiiSimdVec4f vDir = (posNext - posThis);
  xiiSimdFloat fLen = vDir.GetLength<3>();

  if (fLen < m_fSegmentLength)
  {
    return xiiSimdVec4f::ZeroVector();
  }

  vDir /= fLen;
  fLen -= m_fSegmentLength;

  const xiiSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

xiiSimdFloat xiiRopeSimulator::EnforceDistanceConstraint()
{
  // this is the "Jakobsen method" to enforce the distance constraints in each rope node
  // just move each node half the error amount towards the left and right neighboring nodes
  // the ends are either not moved at all (when they are 'attached' to something)
  // or they are moved most of the way
  // this is applied iteratively until the overall error is pretty low

  auto& firstNode = m_Nodes[0];
  auto& lastNode  = m_Nodes.PeekBack();

  xiiSimdFloat fError = xiiSimdFloat::Zero();

  if (!m_bFirstNodeIsFixed)
  {
    const xiiSimdVec4f posThis = m_Nodes[0].m_vPosition;
    const xiiSimdVec4f posNext = m_Nodes[1].m_vPosition;

    m_Nodes[0].m_vPosition += MoveTowards(posThis, posNext, 0.75f, xiiSimdVec4f(0, 0, 1), fError);
  }

  for (xiiUInt32 i = 1; i < m_Nodes.GetCount() - 1; ++i)
  {
    const xiiSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const xiiSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;
    const xiiSimdVec4f posNext = m_Nodes[i + 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.5f, xiiSimdVec4f(0, 0, 1), fError);
    m_Nodes[i].m_vPosition += MoveTowards(posThis, posNext, 0.5f, xiiSimdVec4f(0, 0, -1), fError);
  }

  if (!m_bLastNodeIsFixed)
  {
    const xiiUInt32    i       = m_Nodes.GetCount() - 1;
    const xiiSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const xiiSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.75f, xiiSimdVec4f(0, 0, 1), fError);
  }

  return fError;
}

void xiiRopeSimulator::UpdateNodePositions(const xiiSimdFloat tDiffSqr)
{
  const xiiUInt32 uiFirstNode = m_bFirstNodeIsFixed ? 1 : 0;
  const xiiUInt32 uiNumNodes  = m_bLastNodeIsFixed ? m_Nodes.GetCount() - 1 : m_Nodes.GetCount();

  const xiiSimdFloat damping = m_fDampingFactor;

  const xiiSimdVec4f acceleration = xiiSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (xiiUInt32 i = uiFirstNode; i < uiNumNodes; ++i)
  {
    // this (simple) logic is the so called 'Verlet integration' (+ damping)

    auto& n = m_Nodes[i];

    const xiiSimdVec4f previousPos = n.m_vPosition;

    const xiiSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

    // instead of using a single global acceleration, this could also use individual accelerations per node
    // this would be needed to affect the rope more localized
    n.m_vPosition += vel + acceleration;
    n.m_vPreviousPosition = previousPos;
  }

  if (m_bFirstNodeIsFixed)
  {
    m_Nodes[0].m_vPreviousPosition = m_Nodes[0].m_vPosition;
  }
  if (m_bLastNodeIsFixed)
  {
    m_Nodes.PeekBack().m_vPreviousPosition = m_Nodes.PeekBack().m_vPosition;
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_RopeSimulator);
