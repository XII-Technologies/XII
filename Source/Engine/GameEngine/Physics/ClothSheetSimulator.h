#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging cloth.
///
/// Uses Verlet Integration to update the cloth positions from velocities, and the "Jakobsen method" to enforce distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class XII_GAMEENGINE_DLL xiiClothSimulator
{
public:
  struct Node
  {
    /// Whether this node can swing freely or will remain fixed in place.
    bool         m_bFixed            = false;
    xiiSimdVec4f m_vPosition         = xiiSimdVec4f::ZeroVector();
    xiiSimdVec4f m_vPreviousPosition = xiiSimdVec4f::ZeroVector();
  };

  /// Resolution of the cloth along X
  xiiUInt8 m_uiWidth = 32;

  /// Resolution of the cloth along Y
  xiiUInt8 m_uiHeight = 32;

  /// Overall force acting equally upon all cloth nodes.
  xiiVec3 m_vAcceleration;

  /// Factor with which all node velocities are damped to reduce swinging.
  float m_fDampingFactor = 0.995f;

  /// The distance along x and y between each neighboring node.
  xiiVec2 m_vSegmentLength = xiiVec2(0.1f);

  /// All cloth nodes.
  xiiDynamicArray<Node, xiiAlignedAllocatorWrapper> m_Nodes;

  void SimulateCloth(const xiiTime& diff);
  void SimulateStep(const xiiSimdFloat fDiffSqr, xiiUInt32 uiMaxIterations, xiiSimdFloat fAllowedError);
  bool HasEquilibrium(xiiSimdFloat fAllowedMovement) const;

private:
  xiiSimdFloat EnforceDistanceConstraint();
  void         UpdateNodePositions(const xiiSimdFloat tDiffSqr);
  xiiSimdVec4f MoveTowards(const xiiSimdVec4f posThis, const xiiSimdVec4f posNext, xiiSimdFloat factor, const xiiSimdVec4f fallbackDir, xiiSimdFloat& inout_fError, xiiSimdFloat fSegLen);

  xiiTime m_LeftOverTimeStep;
};
