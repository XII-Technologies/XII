/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>

struct xiiJoltDebugMode
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Shapes           = XII_BIT(0),
    BodyStates       = XII_BIT(1),
    Bounds           = XII_BIT(2),
    CentersOfMass    = XII_BIT(3),
    Velocities       = XII_BIT(4),
    Constraints      = XII_BIT(5),
    ConstraintLimits = XII_BIT(6),
    Contacts         = XII_BIT(7),
    ContactNormals   = XII_BIT(8),
    Characters       = XII_BIT(9),
    Statistics       = XII_BIT(10),
    ActiveBodiesOnly = XII_BIT(11),
    BodyIdentifiers  = XII_BIT(12),
    SolidShapes      = XII_BIT(13),
    Default          = 0
  };

  struct Bits
  {
    StorageType Shapes : 1;
    StorageType BodyStates : 1;
    StorageType Bounds : 1;
    StorageType CentersOfMass : 1;
    StorageType Velocities : 1;
    StorageType Constraints : 1;
    StorageType ConstraintLimits : 1;
    StorageType Contacts : 1;
    StorageType ContactNormals : 1;
    StorageType Characters : 1;
    StorageType Statistics : 1;
    StorageType ActiveBodiesOnly : 1;
    StorageType BodyIdentifiers : 1;
    StorageType SolidShapes : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiJoltDebugMode);

struct xiiJoltSimulationDiagnostics
{
  xiiTime   m_FixedStep;
  xiiTime   m_LastUpdateDuration; ///< Time in native PhysicsSystem::Update calls in the last simulation update.
  xiiUInt32 m_uiSteps                    = 0;
  xiiUInt32 m_uiBodies                   = 0;
  xiiUInt32 m_uiActiveBodies             = 0;
  xiiUInt32 m_uiSleepingBodies           = 0; ///< Inactive dynamic rigid bodies.
  xiiUInt32 m_uiStaticBodies             = 0;
  xiiUInt32 m_uiDynamicBodies            = 0;
  xiiUInt32 m_uiKinematicBodies          = 0;
  xiiUInt32 m_uiReportedContactManifolds = 0; ///< Live manifolds tracked for gameplay contact reporting.
  xiiUInt32 m_uiConstraints              = 0;
  xiiUInt32 m_uiCharacters               = 0;
  xiiUInt32 m_uiUpdateErrors             = 0; ///< Native update error bits from the last simulation update.
  xiiUInt32 m_uiReservedScratchBytes     = 0; ///< Configured temporary allocator capacity before heap fallback.
};

/// Frame-local world-space primitives. Contains no native pointers or renderer contexts.
struct xiiJoltDebugDrawData
{
  struct Line
  {
    xiiVec3  m_vStart;
    xiiVec3  m_vEnd;
    xiiColor m_Color;
  };

  struct Triangle
  {
    xiiVec3  m_vPositions[3];
    xiiColor m_Color;
  };

  struct Label
  {
    xiiVec3   m_vPosition;
    xiiColor  m_Color;
    xiiString m_sText;
  };

  xiiDynamicArray<Line>     m_Lines;
  xiiDynamicArray<Triangle> m_Triangles;
  xiiDynamicArray<Label>    m_Labels;
  bool                      m_bTruncated = false;
};
