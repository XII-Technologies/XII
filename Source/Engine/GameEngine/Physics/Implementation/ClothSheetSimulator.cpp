#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>

void xiiClothSimulator::SimulateCloth(const xiiTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr xiiTime  tStep    = xiiTime::MakeFromSeconds(1.0 / 60.0);
  const xiiSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_vSegmentLength.x);

    m_LeftOverTimeStep -= tStep;
  }
}

void xiiClothSimulator::SimulateStep(const xiiSimdFloat fDiffSqr, xiiUInt32 uiMaxIterations, xiiSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 4)
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

xiiSimdFloat xiiClothSimulator::EnforceDistanceConstraint()
{
  xiiSimdFloat fError = xiiSimdFloat::Zero();

  for (xiiUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (xiiUInt32 x = 0; x < m_uiWidth; ++x)
    {
      const xiiUInt32 idx = (y * m_uiWidth) + x;

      auto& n = m_Nodes[idx];

      if (n.m_bFixed)
        continue;

      const xiiSimdVec4f posThis = n.m_vPosition;

      if (x > 0)
      {
        const xiiSimdVec4f pos = m_Nodes[idx - 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, xiiSimdVec4f(-1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (x + 1 < m_uiWidth)
      {
        const xiiSimdVec4f pos = m_Nodes[idx + 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, xiiSimdVec4f(1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (y > 0)
      {
        const xiiSimdVec4f pos = m_Nodes[idx - m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, xiiSimdVec4f(0, -1, 0), fError, m_vSegmentLength.y);
      }

      if (y + 1 < m_uiHeight)
      {
        const xiiSimdVec4f pos = m_Nodes[idx + m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, xiiSimdVec4f(0, 1, 0), fError, m_vSegmentLength.y);
      }
    }
  }

  return fError;
}

xiiSimdVec4f xiiClothSimulator::MoveTowards(const xiiSimdVec4f posThis, const xiiSimdVec4f posNext, xiiSimdFloat factor, const xiiSimdVec4f fallbackDir, xiiSimdFloat& inout_fError, xiiSimdFloat fSegLen)
{
  xiiSimdVec4f vDir = (posNext - posThis);
  xiiSimdFloat fLen = vDir.GetLength<3>();

  if (fLen.IsEqual(xiiSimdFloat::Zero(), 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= fSegLen;

  const xiiSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

void xiiClothSimulator::UpdateNodePositions(const xiiSimdFloat tDiffSqr)
{
  const xiiSimdFloat damping      = m_fDampingFactor;
  const xiiSimdVec4f acceleration = xiiSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (auto& n : m_Nodes)
  {
    if (n.m_bFixed)
    {
      n.m_vPreviousPosition = n.m_vPosition;
    }
    else
    {
      // this (simple) logic is the so called 'Verlet integration' (+ damping)

      const xiiSimdVec4f previousPos = n.m_vPosition;

      const xiiSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

      // instead of using a single global acceleration, this could also use individual accelerations per node
      // this would be needed to affect the rope more localized
      n.m_vPosition += vel + acceleration;
      n.m_vPreviousPosition = previousPos;
    }
  }
}

bool xiiClothSimulator::HasEquilibrium(xiiSimdFloat fAllowedMovement) const
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


XII_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_ClothSheetSimulator);
