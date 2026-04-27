#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Managers/ReflectionProbeManager.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiReflectionProbeManager);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeManager, 1, xiiRTTIDefaultAllocator<xiiReflectionProbeManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiReflectionProbeManager::xiiReflectionProbeManager(xiiWorld* pWorld)
  : xiiWorldModule(pWorld)
{
}

xiiReflectionProbeManager::~xiiReflectionProbeManager() = default;

void xiiReflectionProbeManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiReflectionProbeManager::Update, this);
  desc.m_Phase    = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;
  desc.m_bOnlyUpdateWhenSimulating = false;
  RegisterUpdateFunction(desc);
}

void xiiReflectionProbeManager::Deinitialize()
{
  m_SortedProbes.Clear();
}

void xiiReflectionProbeManager::InvalidateAll()
{
  // Force all realtime probes to be re-scored at maximum urgency
  for (auto& key : m_SortedProbes)
    key.m_fScore = xiiMath::MaxValue<float>();
}

void xiiReflectionProbeManager::Update(const xiiWorldModule::UpdateContext& ctx)
{
  XII_IGNORE_UNUSED(ctx);

  m_SortedProbes.Clear();

  xiiWorld* pWorld = GetWorld();

  // Score all realtime probes
  for (auto it = pWorld->GetComponents<xiiReflectionProbeComponent>(); it.IsValid(); ++it)
  {
    const xiiReflectionProbeComponent* pProbe = it;
    if (!pProbe->IsActiveAndInitialized() || !pProbe->GetRealtime())
      continue;

    xiiReflectionProbeSortKey key;
    key.m_hComponent = pProbe->GetHandle();
    // Simple scoring: distance from world origin (in a real system use camera pos)
    key.m_fScore = 1.0f / (pProbe->GetOwner()->GetGlobalPosition().GetLength() + 1.0f);
    m_SortedProbes.PushBack(key);
  }

  // Sort descending by score (highest priority first)
  m_SortedProbes.Sort([](const xiiReflectionProbeSortKey& a, const xiiReflectionProbeSortKey& b) {
    return a.m_fScore > b.m_fScore;
  });

  // Process top-N captures — in a real system this submits a cube-map render pass;
  // here we simply invalidate the component's render data to trigger re-extraction.
  const xiiUInt32 uiCaptures = xiiMath::Min<xiiUInt32>(m_uiMaxCapturesPerFrame, m_SortedProbes.GetCount());
  for (xiiUInt32 i = 0; i < uiCaptures; ++i)
  {
    xiiReflectionProbeComponent* pProbe = nullptr;
    if (pWorld->TryGetComponent(m_SortedProbes[i].m_hComponent, pProbe) && pProbe != nullptr)
      pProbe->InvalidateCachedRenderData();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Managers_ReflectionProbeManager);
