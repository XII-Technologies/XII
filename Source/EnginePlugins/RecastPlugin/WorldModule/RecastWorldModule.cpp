#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiRecastWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecastWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRecastWorldModule::xiiRecastWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRecastWorldModule::~xiiRecastWorldModule() = default;

void xiiRecastWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRecastWorldModule::UpdateNavMesh, this);
    updateDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = false;
    updateDesc.m_fPriority                 = 0.0f;

    RegisterUpdateFunction(updateDesc);
  }

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiRecastWorldModule::ResourceEventHandler, this));
}

void xiiRecastWorldModule::Deinitialize()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiRecastWorldModule::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void xiiRecastWorldModule::SetNavMeshResource(const xiiRecastNavMeshResourceHandle& hNavMesh)
{
  m_hNavMesh       = hNavMesh;
  m_pDetourNavMesh = nullptr;
  m_pNavMeshPointsOfInterest.Clear();
}

void xiiRecastWorldModule::UpdateNavMesh(const UpdateContext& ctxt)
{
  if (m_pDetourNavMesh == nullptr && m_hNavMesh.IsValid())
  {
    xiiResourceLock<xiiRecastNavMeshResource> pNavMesh(m_hNavMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pNavMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    m_pDetourNavMesh = pNavMesh->GetNavMesh();

    if (m_pDetourNavMesh)
    {
      m_pNavMeshPointsOfInterest = XII_DEFAULT_NEW(xiiNavMeshPointOfInterestGraph);
      m_pNavMeshPointsOfInterest->ExtractInterestPointsFromMesh(*pNavMesh->GetNavMeshPolygons());
    }
  }

  if (m_pNavMeshPointsOfInterest)
  {
    m_pNavMeshPointsOfInterest->IncreaseCheckVisibiblityTimeStamp(GetWorld()->GetClock().GetAccumulatedTime());
  }
}

void xiiRecastWorldModule::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiRecastNavMeshResource>())
  {
    // triggers a recreation in the next update
    m_pDetourNavMesh = nullptr;
  }
}
