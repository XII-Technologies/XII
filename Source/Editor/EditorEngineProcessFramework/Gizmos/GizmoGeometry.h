/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Math/BoundingBox.h>

/// Shapes consumed by the Editor's manipulators and component visualizers.
enum class xiiEngineGizmoHandleType : xiiUInt8
{
  Arrow,
  Box,
  Sphere,
  CylinderZ,
  HalfSphereZ,
  BoxCorners,
  BoxEdges,
  BoxFaces,
  LineBox,
  LineRect,
  Cone,
  Frustum,
  FromFile,
  TranslationAxis,
  TranslationPlane,
  RotationRing,
  ScaleAxis,
  ScaleUniform,
};

/// Local picking geometry and one-time GPU template source for Editor handles.
/// Procedural ring strokes use the same radius and subdivisions in the shader.
/// This object owns no world objects or GPU resources; custom meshes use the Utilities OBJ loader.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoGeometry
{
public:
  /// Replaces the geometry. Failure leaves it empty with invalid bounds.
  xiiResult Build(xiiEngineGizmoHandleType type, xiiStringView sMeshFile = {});

  const xiiGeometry&    GetGeometry() const { return m_Geometry; }
  const xiiBoundingBox& GetBounds() const { return m_Bounds; }

  /// Finds the closest forward hit using the supplied local-to-world transform without allocating.
  /// Non-normalized ray directions are accepted; out_fRayTime is the ray parameter, not necessarily distance.
  /// Lines use world-space fLineRadius and return the time of closest approach. Triangles are double-sided.
  /// Invalid inputs, singular transforms, and misses return failure without changing out_fRayTime.
  xiiResult PickRay(const xiiMat4& transform, const xiiVec3& vRayStart, const xiiVec3& vRayDirection, float fLineRadius, float& out_fRayTime) const;

private:
  xiiGeometry    m_Geometry;
  xiiBoundingBox m_Bounds = xiiBoundingBox::MakeInvalid();
};
