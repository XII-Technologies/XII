/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <JoltPlugin/Resources/JoltCollisionMeshResource.h>

/// Immutable snapshot retained independently by each native body across resource reload/unload.
class xiiJoltCollisionMeshData final : public xiiRefCounted
{
public:
  JPH::RefConst<JPH::Shape>                 m_pShape;
  xiiJoltCollisionGeometryType::Enum        m_Type = xiiJoltCollisionGeometryType::ConvexHull;
  xiiDynamicArray<xiiSurfaceResourceHandle> m_Surfaces;
};
