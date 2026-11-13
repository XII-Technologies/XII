/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <MujocoPlugin/System/MuJoCoInspection.h>

struct xiiMuJoCoDebugMode
{
  using StorageType = xiiUInt32;
  enum Enum : StorageType
  {
    CollisionGeometry = XII_BIT(0),
    VisualGeometry    = XII_BIT(1),
    BodyFrames        = XII_BIT(2),
    Joints            = XII_BIT(3),
    JointLimits       = XII_BIT(4),
    CentersOfMass     = XII_BIT(5),
    Inertia           = XII_BIT(6),
    Actuators         = XII_BIT(7),
    Sensors           = XII_BIT(8),
    Contacts          = XII_BIT(9),
    ContactNormals    = XII_BIT(10),
    ContactForces     = XII_BIT(11),
    Statistics        = XII_BIT(12),
    Hierarchy         = XII_BIT(13),
    ContactFrames     = XII_BIT(14),
    Bounds            = XII_BIT(15),
    Overlay           = XII_BIT(16),
    Default           = 0
  };
  struct Bits
  {
    StorageType CollisionGeometry : 1;
    StorageType VisualGeometry : 1;
    StorageType BodyFrames : 1;
    StorageType Joints : 1;
    StorageType JointLimits : 1;
    StorageType CentersOfMass : 1;
    StorageType Inertia : 1;
    StorageType Actuators : 1;
    StorageType Sensors : 1;
    StorageType Contacts : 1;
    StorageType ContactNormals : 1;
    StorageType ContactForces : 1;
    StorageType Statistics : 1;
    StorageType Hierarchy : 1;
    StorageType ContactFrames : 1;
    StorageType Bounds : 1;
    StorageType Overlay : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiMuJoCoDebugMode);

struct xiiMuJoCoDebugSettings
{
  xiiBitflags<xiiMuJoCoDebugMode> m_Modes;
  float                           m_fAxisLength  = 0.25f;
  float                           m_fForceScale  = 0.01f; ///< Meters per Newton, and per Newton-meter for torque arrows.
  float                           m_fPlaneExtent = 10.0f; ///< Inspection half extent for infinite planes.

  bool operator==(const xiiMuJoCoDebugSettings&) const = default;
};

/// CPU-only snapshots owned by the world. Borrow only with world write access, until the next
/// snapshot preparation, model replacement, reset, clear, or destruction.
struct xiiMuJoCoDebugDrawData
{
  enum class GeometryType : xiiUInt8
  {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    Ellipsoid,
    Plane,
    Mesh,
    Bounds
  };
  struct Geometry
  {
    GeometryType       m_Type      = GeometryType::Box;
    xiiTransform       m_Transform = xiiTransform::MakeIdentity();
    xiiVec3            m_vSize     = xiiVec3::MakeZero(); ///< Native half dimensions. Capsules/cylinders use X radius and Y half length.
    xiiColor           m_Color     = xiiColor::White;
    xiiUInt32          m_uiMesh    = xiiInvalidIndex;
    xiiMuJoCoElementId m_GeometryId;
    xiiComponentHandle m_hModelComponent;
  };
  struct Line
  {
    xiiVec3  m_vStart;
    xiiVec3  m_vEnd;
    xiiColor m_Color;
  };
  struct Mesh
  {
    xiiDynamicArray<Line> m_Edges; ///< Immutable local-space edges, built once per mesh representation.
    bool                  m_bTruncated   = false;
    bool                  m_bUnsupported = false;
  };
  struct Label
  {
    xiiVec3   m_vPosition;
    xiiColor  m_Color;
    xiiString m_sText;
  };

  xiiDynamicArray<Geometry> m_Geometry;
  xiiDynamicArray<Line>     m_Lines;
  xiiDynamicArray<Label>    m_Labels;
  xiiDynamicArray<Label>    m_NonSpatialLabels;
  xiiDynamicArray<Mesh>     m_Meshes;
  xiiMuJoCoSimulationInfo   m_Simulation;
  xiiMuJoCoStepDiagnostics  m_Step;
  xiiMuJoCoDebugSettings    m_Settings;
  xiiMuJoCoStateStamp       m_Stamp;
  bool                      m_bTruncated           = false;
  bool                      m_bUnsupportedGeometry = false;
};
