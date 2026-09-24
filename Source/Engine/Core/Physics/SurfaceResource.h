/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

class xiiWorld;
class xiiUuid;

struct xiiSurfaceResourceEvent
{
  enum class Type
  {
    Created,
    Destroyed
  };

  Type                m_Type;
  xiiSurfaceResource* m_pSurface = nullptr;
};

class XII_CORE_DLL xiiSurfaceResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiSurfaceResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiSurfaceResource, xiiSurfaceResourceDescriptor);

public:
  xiiSurfaceResource();
  ~xiiSurfaceResource();

  const xiiSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  static xiiEvent<const xiiSurfaceResourceEvent&, xiiMutex> s_Events;

  void* m_pPhysicsMaterialPhysX = nullptr;
  void* m_pPhysicsMaterialJolt  = nullptr;

  /// Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(xiiWorld* pWorld, xiiGameObjectHandle hObject, const xiiVec3& vPosition, const xiiVec3& vSurfaceNormal, const xiiVec3& vIncomingDirection, const xiiTempHashedString& sInteraction, const xiiUInt16* pOverrideTeamID, float fImpulseSqr = 0.0f) const;

  bool IsBasedOn(const xiiSurfaceResource* pThisOrBaseSurface) const;

  bool IsBasedOn(const xiiSurfaceResourceHandle hThisOrBaseSurface) const;

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  static const xiiSurfaceInteraction* FindInteraction(const xiiSurfaceResource* pCurSurf, xiiUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue);

  xiiSurfaceResourceDescriptor m_Descriptor;

  struct SurfInt
  {
    xiiUInt64                    m_uiInteractionTypeHash = 0;
    const xiiSurfaceInteraction* m_pInteraction;
  };

  xiiDynamicArray<SurfInt> m_Interactions;
};
