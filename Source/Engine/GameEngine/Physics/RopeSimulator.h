/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging ropes.
///
/// Can be used both for interactive rope simulation, as well as to just pre-compute the shape of hanging wires, cables, etc.
/// Uses Verlet Integration to update the rope positions from velocities, and the "Jakobsen method" to enforce
/// rope distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class XII_GAMEENGINE_DLL xiiRopeSimulator
{
public:
  struct Node
  {
    xiiSimdVec4f m_vPosition         = xiiSimdVec4f::MakeZero();
    xiiSimdVec4f m_vPreviousPosition = xiiSimdVec4f::MakeZero();

    // could add per node acceleration
    // could add per node mass
  };

public:
  xiiRopeSimulator();
  ~xiiRopeSimulator();

  /// \brief External acceleration, typically gravity or a combination of gravity and wind.
  /// Applied to all rope nodes equally.
  xiiVec3 m_vAcceleration = xiiVec3(0, 0, -10);

  /// \brief All the nodes in the rope
  xiiDynamicArray<Node, xiiAlignedAllocatorWrapper> m_Nodes;

  /// \brief A factor to dampen velocities to make the rope stop swinging.
  /// Should be between 0.97 (strong damping) and 1.0 (no damping).
  float m_fDampingFactor = 0.995f;

  /// \brief How long each rope segment (between two nodes) should be.
  float m_fSegmentLength = 0.1f;

  bool m_bFirstNodeIsFixed = true;
  bool m_bLastNodeIsFixed  = true;

  void         SimulateRope(const xiiTime& diff);
  void         SimulateStep(const xiiSimdFloat fDiffSqr, xiiUInt32 uiMaxIterations, xiiSimdFloat fAllowedError);
  void         SimulateTillEquilibrium(xiiSimdFloat fAllowedMovement = 0.005f, xiiUInt32 uiMaxIterations = 1000);
  bool         HasEquilibrium(xiiSimdFloat fAllowedMovement) const;
  float        GetTotalLength() const;
  xiiSimdVec4f GetPositionAtLength(float fLength) const;

private:
  xiiSimdFloat EnforceDistanceConstraint();
  void         UpdateNodePositions(const xiiSimdFloat tDiffSqr);
  xiiSimdVec4f MoveTowards(const xiiSimdVec4f posThis, const xiiSimdVec4f posNext, xiiSimdFloat factor, const xiiSimdVec4f fallbackDir, xiiSimdFloat& inout_fError);

  xiiTime m_LeftOverTimeStep;
};
