#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Managers/LightProbeGridManager.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiLightProbeGridManager);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightProbeGridManager, 1, xiiRTTIDefaultAllocator<xiiLightProbeGridManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLightProbeGridManager::xiiLightProbeGridManager(xiiWorld* pWorld)
  : xiiWorldModule(pWorld)
{
}

xiiLightProbeGridManager::~xiiLightProbeGridManager() = default;

void xiiLightProbeGridManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiLightProbeGridManager::Update, this);
  desc.m_Phase    = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;
  desc.m_bOnlyUpdateWhenSimulating = false;
  RegisterUpdateFunction(desc);
}

void xiiLightProbeGridManager::Deinitialize()
{
  m_DirtyGrids.Clear();
}

void xiiLightProbeGridManager::InvalidateAll()
{
  m_bBakeOnNextUpdate = true;
  m_DirtyGrids.Clear();
}

void xiiLightProbeGridManager::InvalidateGrid(xiiComponentHandle hComponent)
{
  if (!m_DirtyGrids.Contains(hComponent))
    m_DirtyGrids.PushBack(hComponent);
}

void xiiLightProbeGridManager::Update(const xiiWorldModule::UpdateContext& ctx)
{
  XII_IGNORE_UNUSED(ctx);

  if (m_bBakeOnNextUpdate)
  {
    m_bBakeOnNextUpdate = false;

    // Collect all probe grid components
    xiiWorld* pWorld = GetWorld();
    for (auto it = pWorld->GetComponents<xiiLightProbeGridComponent>(); it.IsValid(); ++it)
    {
      xiiLightProbeGridComponent* pComp = it;
      if (pComp->IsActiveAndInitialized())
        InvalidateGrid(pComp->GetHandle());
    }
  }

  // Process dirty grids — in a full implementation this would schedule GPU bake passes.
  // For now we mark the component's render data invalid so it re-submits next frame.
  xiiWorld* pWorld = GetWorld();
  for (const xiiComponentHandle& hComp : m_DirtyGrids)
  {
    xiiLightProbeGridComponent* pComp = nullptr;
    if (pWorld->TryGetComponent(hComp, pComp) && pComp != nullptr)
      pComp->InvalidateCachedRenderData();
  }

  m_DirtyGrids.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Managers_LightProbeGridManager);
