#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

class xiiJoltMaterial : public JPH::PhysicsMaterial
{
public:
  xiiJoltMaterial();
  ~xiiJoltMaterial();

  xiiSurfaceResource* m_pSurface = nullptr;

  float m_fRestitution = 0.0f;
  float m_fFriction    = 0.2f;
};
