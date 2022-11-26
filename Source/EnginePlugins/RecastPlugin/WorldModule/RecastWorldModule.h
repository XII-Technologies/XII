#pragma once

#include <RecastPlugin/RecastPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>

class dtCrowd;
class dtNavMesh;
struct xiiResourceEvent;

using xiiRecastNavMeshResourceHandle = xiiTypedResourceHandle<class xiiRecastNavMeshResource>;

class XII_RECASTPLUGIN_DLL xiiRecastWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiRecastWorldModule, xiiWorldModule);

public:
  xiiRecastWorldModule(xiiWorld* pWorld);
  ~xiiRecastWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void                                  SetNavMeshResource(const xiiRecastNavMeshResourceHandle& hNavMesh);
  const xiiRecastNavMeshResourceHandle& GetNavMeshResource() { return m_hNavMesh; }

  const dtNavMesh*                      GetDetourNavMesh() const { return m_pDetourNavMesh; }
  const xiiNavMeshPointOfInterestGraph* GetNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }
  xiiNavMeshPointOfInterestGraph*       AccessNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }

private:
  void UpdateNavMesh(const UpdateContext& ctxt);
  void ResourceEventHandler(const xiiResourceEvent& e);

  const dtNavMesh*                             m_pDetourNavMesh = nullptr;
  xiiRecastNavMeshResourceHandle               m_hNavMesh;
  xiiUniquePtr<xiiNavMeshPointOfInterestGraph> m_pNavMeshPointsOfInterest;
};
