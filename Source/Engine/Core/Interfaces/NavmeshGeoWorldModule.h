/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/World/WorldModule.h>

struct xiiNavmeshTriangle
{
  xiiVec3                   m_Vertices[3];
  const xiiSurfaceResource* m_pSurface = nullptr;
};

/// A world module that retrieves triangle data that should be used for building navmeshes at runtime.
///
/// If a physics engine is active, it usually automatically provides such a world module to retrieve the triangle data
/// through physics queries.
///
/// In other types of games, a custom world module can be implemented, to generate this data in a different way.
/// If a physics engine is active, but a custom method should be used, you can write a custom world module
/// and then use xiiWorldModuleFactory::RegisterInterfaceImplementation() to specify which module to use.
/// Also see xiiWorldModuleConfig.
class XII_CORE_DLL xiiNavmeshGeoWorldModuleInterface : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNavmeshGeoWorldModuleInterface, xiiWorldModule);

protected:
  xiiNavmeshGeoWorldModuleInterface(xiiWorld* pWorld) :
    xiiWorldModule(pWorld)
  {
  }

public:
  virtual void RetrieveGeometryInArea(xiiUInt32 uiCollisionLayer, const xiiBoundingBox& box, xiiDynamicArray<xiiNavmeshTriangle>& out_triangles) const = 0;
};
