/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiViewEventType, 1)
  XII_ENUM_CONSTANT(xiiViewEventType::Created),
  XII_ENUM_CONSTANT(xiiViewEventType::Deleted),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);

namespace
{
  static xiiUniquePtr<xiiRenderData> CloneRenderDataForCache(const xiiRenderData* pRenderData)
  {
    if (pRenderData == nullptr)
      return nullptr;

    const xiiRTTI* pType = pRenderData->GetDynamicRTTI();
    if (pType == nullptr)
      return nullptr;

    xiiRTTIAllocator* pAllocator = pType->GetAllocator();
    if (pAllocator == nullptr)
      return nullptr;

    xiiInternal::NewInstance<xiiRenderData> pClone = pAllocator->Clone<xiiRenderData>(pRenderData);
    if (pClone == nullptr)
      return nullptr;

    return xiiUniquePtr<xiiRenderData>(pClone);
  }
} // namespace

xiiUniquePtr<xiiRenderWorldModule::ExtractionEvent> xiiRenderWorldModule::s_pRenderEvent;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderWorldEvents)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiRenderWorldModule::StartupRenderEvents();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRenderWorldModule::ShutdownRenderEvents();
  }


XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiRenderWorldModule::StartupRenderEvents()
{
  XII_ASSERT_DEV(s_pRenderEvent == nullptr, "Render world events were started twice.");
  s_pRenderEvent = XII_DEFAULT_NEW(ExtractionEvent);
}

void xiiRenderWorldModule::ShutdownRenderEvents()
{
  s_pRenderEvent.Clear();
}

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) : xiiWorldModule(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiRenderWorldModule::OnResourceEvent, this));
}

xiiRenderWorldModule::~xiiRenderWorldModule()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiRenderWorldModule::OnResourceEvent, this));
}

void xiiRenderWorldModule::OnResourceEvent(const xiiResourceEvent& event)
{
  if (event.m_Type != xiiResourceEvent::Type::ResourceContentUpdated && event.m_Type != xiiResourceEvent::Type::ResourceContentUnloading)
    return;
  const auto* type = event.m_pResource->GetDynamicRTTI();
  if (type->IsDerivedFrom<xiiMeshResource>() || type->IsDerivedFrom<xiiMeshBufferResource>())
    m_bGeometryCacheDirty = true;
}

void xiiRenderWorldModule::ReleaseGpuResources()
{
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
    it.Value().m_pExtractedData->Clear();
  DeleteAllCachedRenderData();
  m_RetiredPackets.Clear();
  m_pRayTracingResources.Clear();
  m_pGpuInstances.Clear();
  m_pWorldLights.Clear();
}

// Reuse the existing world registry for device shutdown; do not maintain another world/scene registry.
XII_ON_GLOBAL_EVENT(xiiStartup_ShutdownHighLevelSystems_Begin)
{
  for (xiiUInt32 i = 0; i < xiiWorld::GetWorldCount(); ++i)
    if (auto* world = xiiWorld::GetWorld(i))
    {
      XII_LOCK(world->GetWriteMarker());
      if (auto* module = world->GetModule<xiiRenderWorldModule>())
        module->ReleaseGpuResources();
    }
}

void xiiRenderWorldModule::Initialize()
{
  // Compile CPU-only view demand before scene updates. Extract only after authoritative world
  // transforms are final, then record/execute after extraction in the same PostTransform phase.
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::PrepareExtraction, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PreAsync;
    description.m_fPriority                 = -1000.0f; // After ordinary view creation/configuration updates.
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostTransform;
    description.m_fPriority                 = -1000.0f; // Consume normal modules' current derived scene data.
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }

  // Execute after extraction and authoritative transform updates, on the world update thread.
  {
    auto description        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    description.m_Phase     = xiiWorldUpdatePhase::PostTransform;
    description.m_fPriority = -2000.0f;
    description.m_DependsOn.PushBack(xiiMakeHashedString("xiiRenderWorldModule::ExtractRenderData"));
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  // Removing entries invalidates ID-table iterators. Snapshot the handles before
  // destruction, which also lets deletion listeners observe already-invalid handles.
  xiiDynamicArray<xiiViewHandle> views;
  views.Reserve(m_ViewIdTable.GetCount());
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    views.PushBack(it.Value().m_pView->GetHandle());
  }
  for (const auto& view : views)
    DestroyView(view);

  m_pRayTracingResources.Clear();
  m_pGpuInstances.Clear();
  m_pWorldLights.Clear();

  xiiDebugRenderer::ClearContext(GetWorld());
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

xiiResult xiiRenderWorldModule::UpdateWorldLights(xiiArrayPtr<xiiRenderData* const> packets)
{
  if (!m_pWorldLights)
    m_pWorldLights = XII_DEFAULT_NEW(xiiWorldLightData, *GetWorld());
  return m_pWorldLights->UpdateFromExtraction(packets);
}

xiiResult xiiRenderWorldModule::UpdateGpuInstances(const xiiExtractedRenderData& packets, xiiUInt64 frame)
{
  return UpdateGpuInstances(packets.GetAllRenderData(), frame);
}

xiiRayTracingResources::BuildHandles xiiRenderWorldModule::AddRayTracingBuildPass(xiiRenderGraph& graph, xiiUInt64 frame, xiiRayTracingFrameResources* viewResources, const xiiArrayPtr<const xiiUInt32>* selection) const
{
  if (!m_pGpuInstances || !xiiRayTracingResources::IsHardwareRayTracingSupported())
    return {};
  if (!m_pRayTracingResources)
    m_pRayTracingResources = XII_DEFAULT_NEW(xiiRayTracingFrameResources);
  return xiiRayTracingResources::AddBuildPass(graph, frame, viewResources ? *viewResources : *m_pRayTracingResources, *m_pGpuInstances, selection, m_pRayTracingResources.Borrow());
}

xiiResult xiiRenderWorldModule::UpdateGpuInstances(xiiArrayPtr<xiiRenderData* const> packets, xiiUInt64 frame)
{
  if (!m_pGpuInstances)
    m_pGpuInstances = XII_DEFAULT_NEW(xiiGpuInstanceData, *GetWorld());
  return m_pGpuInstances->UpdateFromExtraction(packets, frame);
}

xiiViewHandle xiiRenderWorldModule::CreateView(xiiStringView sName, xiiView*& out_pView)
{
  ViewDetail viewDetail;
  viewDetail.m_pView = XII_DEFAULT_NEW(xiiView, m_pWorld);

  viewDetail.m_pView->SetName(sName);

  ViewDetail*     pViewDetail;
  const xiiViewId viewId = m_ViewIdTable.Insert(std::move(viewDetail));
  XII_VERIFY(m_ViewIdTable.TryGetValue(viewId, pViewDetail), "Failed to retrieve view detail after valid insert.");

  pViewDetail->m_pView->m_InternalId = viewId;
  pViewDetail->m_pExtractedData      = XII_DEFAULT_NEW(xiiExtractedRenderData, *pViewDetail->m_pView);
  pViewDetail->m_pView->SetExtractedRenderData(pViewDetail->m_pExtractedData.Borrow());

  xiiViewEvent viewEvent;
  viewEvent.m_Type  = xiiViewEventType::Created;
  viewEvent.m_pView = pViewDetail->m_pView.Borrow();
  m_ViewEvents.Broadcast(viewEvent);

  out_pView = pViewDetail->m_pView.Borrow();

  return pViewDetail->m_pView->GetHandle();
}

void xiiRenderWorldModule::DestroyView(const xiiViewHandle& hView)
{
  if (hView.IsInvalidated())
    return;

  ViewDetail viewDetail;
  if (!m_ViewIdTable.Remove(hView, &viewDetail))
    return;

  xiiViewEvent viewEvent;
  viewEvent.m_Type  = xiiViewEventType::Deleted;
  viewEvent.m_pView = viewDetail.m_pView.Borrow();
  m_ViewEvents.Broadcast(viewEvent);
  xiiDebugRenderer::ClearContext(*viewDetail.m_pView);
}

bool xiiRenderWorldModule::TryGetView(const xiiViewHandle& hView, xiiView*& out_pView) const
{
  ViewDetail* pViewDetail;
  if (!m_ViewIdTable.TryGetValue(hView, pViewDetail))
    return false;

  out_pView = pViewDetail->m_pView.Borrow();
  return true;
}

xiiView* xiiRenderWorldModule::GetViewByUsageHint(xiiEnum<xiiCameraUsageHint> usageHint, xiiEnum<xiiCameraUsageHint> alternativeUsageHint) const
{
  xiiView* pAlternativeView = nullptr;

  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& value = it.Value();

    if (value.m_pView->GetCameraUsageHint() == usageHint)
    {
      return value.m_pView.Borrow();
    }
    else if (alternativeUsageHint != xiiCameraUsageHint::None && value.m_pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = value.m_pView.Borrow();
    }
  }
  return pAlternativeView;
}

void xiiRenderWorldModule::SubmitRenderData(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  if (pContext == nullptr)
    return;

  static_cast<xiiRenderWorldModule*>(pContext)->OnRenderDataSubmitted(msg, pRenderData, caching);
}

void xiiRenderWorldModule::OnRenderDataSubmitted(const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching, bool collectShared)
{
  if (msg.m_pView == nullptr)
    return;

  if (msg.m_pExtractedRenderData == nullptr || pRenderData == nullptr)
    return;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, caching);

  if (msg.m_hCurrentObject.IsInvalidated())
    return;

  xiiDynamicArray<SharedPacket>* shared = nullptr;
  if (m_SharedFramePackets.TryGetValue(msg.m_hCurrentComponent, shared))
  {
    if (collectShared)
      shared->PushBack({pRenderData, caching});
    return;
  }
  ViewDetail&               viewDetail = m_ViewIdTable[msg.m_pView->GetHandle()];
  ExtractedObjectFrameData& frameData  = viewDetail.m_ExtractionCache.m_FrameObjectData[msg.m_hCurrentObject];

  if (caching == xiiRenderData::Caching::IfStatic)
  {
    if (!msg.m_hCurrentComponent.IsInvalidated())
    {
      frameData.m_ComponentFrameData[msg.m_hCurrentComponent].m_StaticRenderData.PushBack(pRenderData);
    }
    else
    {
      frameData.m_ObjectLevelStaticRenderData.PushBack(pRenderData);
    }
  }
  else
  {
    frameData.m_bHasDynamicRenderData = true;

    if (!msg.m_hCurrentComponent.IsInvalidated())
    {
      frameData.m_ComponentFrameData[msg.m_hCurrentComponent].m_bHasDynamicRenderData = true;
    }
  }
}

bool xiiRenderWorldModule::ReuseCachedStaticRenderData(const ViewExtractionCache& cache, xiiGameObjectHandle hObject, xiiExtractedRenderData& out_extractedRenderData) const
{
  const CachedStaticObjectData* pCachedData = nullptr;
  if (!cache.m_StaticObjectCache.TryGetValue(hObject, pCachedData) || pCachedData == nullptr)
    return false;

  for (const auto& pCachedRenderData : pCachedData->m_StaticRenderData)
  {
    if (pCachedRenderData == nullptr)
      continue;

    out_extractedRenderData.AddRenderData(pCachedRenderData.Borrow(), xiiRenderData::Caching::IfStatic);
  }

  return true;
}

bool xiiRenderWorldModule::ReuseCachedStaticRenderData(const ViewExtractionCache& cache, xiiComponentHandle hComponent, xiiExtractedRenderData& out_extractedRenderData) const
{
  const CachedStaticComponentData* pCachedData = nullptr;
  if (!cache.m_StaticComponentCache.TryGetValue(hComponent, pCachedData) || pCachedData == nullptr)
    return false;

  for (const auto& pCachedRenderData : pCachedData->m_StaticRenderData)
  {
    if (pCachedRenderData == nullptr)
      continue;

    out_extractedRenderData.AddRenderData(pCachedRenderData.Borrow(), xiiRenderData::Caching::IfStatic);
  }

  return true;
}

void xiiRenderWorldModule::FinalizeViewExtractionCache(ViewExtractionCache& cache)
{
  auto AppendClones = [](const xiiDynamicArray<xiiRenderData*>& sourceData, xiiDynamicArray<xiiUniquePtr<xiiRenderData>>& out_clonedData) {
    for (const xiiRenderData* pRenderData : sourceData)
    {
      xiiUniquePtr<xiiRenderData> pClone = CloneRenderDataForCache(pRenderData);
      if (pClone == nullptr)
        return false;

      out_clonedData.PushBack(std::move(pClone));
    }

    return true;
  };

  for (auto it = cache.m_FrameObjectData.GetIterator(); it.IsValid(); ++it)
  {
    const xiiGameObjectHandle       hObject   = it.Key();
    const ExtractedObjectFrameData& frameData = it.Value();

    bool bHasStaticRenderData = !frameData.m_ObjectLevelStaticRenderData.IsEmpty();
    if (!bHasStaticRenderData)
    {
      for (auto componentIt = frameData.m_ComponentFrameData.GetIterator(); componentIt.IsValid(); ++componentIt)
      {
        if (!componentIt.Value().m_StaticRenderData.IsEmpty())
        {
          bHasStaticRenderData = true;
          break;
        }
      }
    }

    if (!bHasStaticRenderData)
    {
      RemoveCachedRenderDataForObject(cache, hObject);
      continue;
    }

    if (!frameData.m_bHasDynamicRenderData)
    {
      CachedStaticObjectData newCachedData;

      bool bCloningSucceeded = AppendClones(frameData.m_ObjectLevelStaticRenderData, newCachedData.m_StaticRenderData);
      if (bCloningSucceeded)
      {
        for (auto componentIt = frameData.m_ComponentFrameData.GetIterator(); componentIt.IsValid(); ++componentIt)
        {
          bCloningSucceeded = AppendClones(componentIt.Value().m_StaticRenderData, newCachedData.m_StaticRenderData);
          if (!bCloningSucceeded)
            break;
        }
      }

      if (!bCloningSucceeded || newCachedData.m_StaticRenderData.IsEmpty())
      {
        RemoveCachedRenderDataForObject(cache, hObject);
        continue;
      }

      RemoveCachedRenderDataForObject(cache, hObject);
      cache.m_StaticObjectCache.Insert(hObject, std::move(newCachedData));
      continue;
    }

    // Mixed object: keep per-component static cache and skip object-wide cache.
    RemoveCachedRenderDataForObject(cache, hObject);

    xiiDynamicArray<xiiComponentHandle> cachedComponents;

    for (auto componentIt = frameData.m_ComponentFrameData.GetIterator(); componentIt.IsValid(); ++componentIt)
    {
      const xiiComponentHandle           hComponent         = componentIt.Key();
      const ExtractedComponentFrameData& componentFrameData = componentIt.Value();

      if (componentFrameData.m_bHasDynamicRenderData || componentFrameData.m_StaticRenderData.IsEmpty())
        continue;

      CachedStaticComponentData newCachedData;
      const bool                bCloningSucceeded = AppendClones(componentFrameData.m_StaticRenderData, newCachedData.m_StaticRenderData);

      if (!bCloningSucceeded || newCachedData.m_StaticRenderData.IsEmpty())
        continue;

      cache.m_StaticComponentCache.Insert(hComponent, std::move(newCachedData));
      cachedComponents.PushBack(hComponent);
    }

    if (!cachedComponents.IsEmpty())
    {
      cache.m_ObjectToCachedComponents.Insert(hObject, std::move(cachedComponents));
    }
  }

  cache.m_FrameObjectData.Clear();
}

void xiiRenderWorldModule::RemoveCachedRenderDataForObject(ViewExtractionCache& cache, xiiGameObjectHandle hObject)
{
  CachedStaticObjectData* objectData = nullptr;
  if (cache.m_StaticObjectCache.TryGetValue(hObject, objectData))
    for (auto& data : objectData->m_StaticRenderData)
      m_RetiredPackets.PushBack(std::move(data));
  cache.m_StaticObjectCache.Remove(hObject);

  xiiDynamicArray<xiiComponentHandle>* pCachedComponents = nullptr;
  if (cache.m_ObjectToCachedComponents.TryGetValue(hObject, pCachedComponents) && pCachedComponents != nullptr)
  {
    for (xiiComponentHandle hCachedComponent : *pCachedComponents)
    {
      CachedStaticComponentData* componentData = nullptr;
      if (cache.m_StaticComponentCache.TryGetValue(hCachedComponent, componentData))
        for (auto& data : componentData->m_StaticRenderData)
          m_RetiredPackets.PushBack(std::move(data));
      cache.m_StaticComponentCache.Remove(hCachedComponent);
    }
  }

  cache.m_ObjectToCachedComponents.Remove(hObject);
}

void xiiRenderWorldModule::RemoveCachedRenderDataForComponent(ViewExtractionCache& cache, xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent)
{
  CachedStaticComponentData* componentData = nullptr;
  if (cache.m_StaticComponentCache.TryGetValue(hComponent, componentData))
    for (auto& data : componentData->m_StaticRenderData)
      m_RetiredPackets.PushBack(std::move(data));
  cache.m_StaticComponentCache.Remove(hComponent);

  xiiDynamicArray<xiiComponentHandle>* pCachedComponents = nullptr;
  if (!cache.m_ObjectToCachedComponents.TryGetValue(hOwnerObject, pCachedComponents) || pCachedComponents == nullptr)
    return;

  const xiiUInt32 uiComponentIndex = pCachedComponents->IndexOf(hComponent);
  if (uiComponentIndex != xiiInvalidIndex)
  {
    pCachedComponents->RemoveAtAndCopy(uiComponentIndex);
  }

  if (pCachedComponents->IsEmpty())
  {
    cache.m_ObjectToCachedComponents.Remove(hOwnerObject);
  }
}

void xiiRenderWorldModule::DeleteCachedRenderData(xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent)
{
  if (hOwnerObject.IsInvalidated())
    return;

  if (!hComponent.IsInvalidated())
  {
    CachedStaticComponentData* cached = nullptr;
    if (m_SharedStaticPackets.TryGetValue(hComponent, cached))
    {
      for (auto& data : cached->m_StaticRenderData)
        m_RetiredPackets.PushBack(std::move(data));
      m_SharedStaticPackets.Remove(hComponent);
    }
  }
  else
    for (auto it = m_SharedStaticPackets.GetIterator(); it.IsValid();)
    {
      const bool matches = !it.Value().m_StaticRenderData.IsEmpty() && it.Value().m_StaticRenderData[0]->m_hOwnerObject == hOwnerObject;
      if (!matches)
      {
        ++it;
        continue;
      }
      for (auto& data : it.Value().m_StaticRenderData)
        m_RetiredPackets.PushBack(std::move(data));
      m_SharedFramePackets.Remove(it.Key());
      it = m_SharedStaticPackets.Remove(it);
    }
  m_SharedFramePackets.Remove(hComponent);
  xiiBitflags<xiiRenderExtractionCategory> categories = xiiRenderExtractionCategory::All;
  const xiiComponent*                      component  = nullptr;
  if (!hComponent.IsInvalidated() && GetWorld()->TryGetComponent(hComponent, component))
    if (const auto* renderComponent = xiiDynamicCast<const xiiRenderComponent*>(component))
      categories = renderComponent->GetRenderExtractionCategories();
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& value = it.Value();
    if (!value.m_pView->GetExtractionRequirements().IsAnySet(categories))
      continue;

    if (!hComponent.IsInvalidated() && !value.m_ExtractionCache.m_StaticObjectCache.Contains(hOwnerObject))
    {
      RemoveCachedRenderDataForComponent(value.m_ExtractionCache, hOwnerObject, hComponent);
    }
    else
    {
      RemoveCachedRenderDataForObject(value.m_ExtractionCache, hOwnerObject);
    }
  }
}

void xiiRenderWorldModule::DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* object)
{
  if (!object)
    return;
  xiiHybridArray<const xiiGameObject*, 64> pending;
  pending.PushBack(object);
  while (!pending.IsEmpty())
  {
    const auto* current = pending.PeekBack();
    pending.PopBack();
    DeleteCachedRenderData(current->GetHandle(), {});
    for (auto child = current->GetChildren(); child.IsValid(); ++child)
      pending.PushBack(child);
  }
}

void xiiRenderWorldModule::DeleteAllCachedRenderData()
{
  for (auto& entry : m_SharedStaticPackets)
    for (auto& packet : entry.Value().m_StaticRenderData)
      m_RetiredPackets.PushBack(std::move(packet));
  m_SharedStaticPackets.Clear();
  m_SharedFramePackets.Clear();
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& value = it.Value();

    for (auto& entry : value.m_ExtractionCache.m_StaticObjectCache)
      for (auto& packet : entry.Value().m_StaticRenderData)
        m_RetiredPackets.PushBack(std::move(packet));
    for (auto& entry : value.m_ExtractionCache.m_StaticComponentCache)
      for (auto& packet : entry.Value().m_StaticRenderData)
        m_RetiredPackets.PushBack(std::move(packet));
    value.m_ExtractionCache.m_StaticObjectCache.Clear();
    value.m_ExtractionCache.m_StaticComponentCache.Clear();
    value.m_ExtractionCache.m_ObjectToCachedComponents.Clear();
    value.m_ExtractionCache.m_FrameObjectData.Clear();
  }
}

void xiiRenderWorldModule::PrepareExtraction(const xiiWorldModule::UpdateContext&)
{
  // Drop all borrowed pointers before releasing retired/static snapshot ownership.
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
    it.Value().m_pExtractedData->Clear();
  m_SharedFramePackets.Clear();
  if (m_bGeometryCacheDirty.Set(false))
    DeleteAllCachedRenderData();
  m_RetiredPackets.Clear();
  m_ExtractionRequirements = {};
  m_RuntimeFeatures        = {};
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& detail       = it.Value();
    auto& view         = *detail.m_pView;
    detail.m_bPrepared = false;
    // Drop packet pointers before a demand change can destroy their owning static cache.
    detail.m_pExtractedData->Clear();
    view.m_ExtractionRequirements = {};
    view.m_RuntimeFeatures        = {};
    view.m_ExtractionStatistics   = {};
    view.m_LastRenderResult       = XII_FAILURE;
    if (view.IsRenderEnabled() && view.IsValid() && view.SynchronizeRenderPipeline().Succeeded())
    {
      detail.m_bPrepared = true;
      m_ExtractionRequirements |= view.GetExtractionRequirements();
      m_RuntimeFeatures |= view.GetRuntimeFeatures();
    }
    if (detail.m_ExtractionCache.m_Requirements != view.GetExtractionRequirements())
    {
      ViewExtractionCache empty;
      detail.m_ExtractionCache                = std::move(empty);
      detail.m_ExtractionCache.m_Requirements = view.GetExtractionRequirements();
    }
  }
}

void xiiRenderWorldModule::ExtractRenderData(const xiiWorldModule::UpdateContext& context)
{
  m_SharedFramePackets.Clear();
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& viewDetail = it.Value();

    if (!viewDetail.m_bPrepared || !viewDetail.m_pView->IsRenderEnabled() || !viewDetail.m_pView->IsValid())
      continue;

    XII_ASSERT_DEV(viewDetail.m_pExtractedData != nullptr, "xiiRenderWorldModule view cache entry must always have extracted render data.");

    viewDetail.m_ExtractionCache.m_FrameObjectData.Clear();

    viewDetail.m_pExtractedData->Clear();
    const auto requirements = viewDetail.m_pView->GetExtractionRequirements();
    if (requirements.IsNoFlagSet())
      continue;

    xiiRenderWorldModuleExtractionEvent extractionEvent;
    extractionEvent.m_Type  = xiiRenderWorldModuleExtractionEvent::Type::BeforeViewExtraction;
    extractionEvent.m_pView = viewDetail.m_pView.Borrow();
    s_pRenderEvent->Broadcast(extractionEvent);

    xiiMsgExtractRenderData msg;
    msg.m_Requirements             = requirements;
    msg.m_pView                    = viewDetail.m_pView.Borrow();
    msg.m_pExtractedRenderData     = viewDetail.m_pExtractedData.Borrow();
    msg.m_SubmitRenderDataFunction = &xiiRenderWorldModule::SubmitRenderData;
    msg.m_pSubmitRenderDataContext = this;

    // Broadcast to all objects, each object routes to matching component message handlers.
    {
      XII_LOCK(GetWorld()->GetReadMarker());

      const xiiWorld* world = GetWorld();
      for (auto it = world->GetObjects(); it.IsValid(); ++it)
      {
        const xiiGameObject* pObject = it;
        // Filter before cache reuse as tags and activation can change without invalidating static render data.
        const xiiTagSet& tags = pObject->GetTags();
        const xiiView&   view = *viewDetail.m_pView;
        if (!pObject->IsActive() || tags.IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !tags.IsAnySet(view.m_IncludeTags)))
          continue;

        const xiiGameObjectHandle hObject = pObject->GetHandle();

        msg.m_hCurrentObject    = hObject;
        msg.m_hCurrentComponent = xiiComponentHandle();

        if (ReuseCachedStaticRenderData(viewDetail.m_ExtractionCache, hObject, *viewDetail.m_pExtractedData))
        {
          msg.m_hCurrentObject = xiiGameObjectHandle();
          continue;
        }

        // Dispatch to object-level handlers explicitly, component dispatch is handled below.
        xiiGetStaticRTTI<xiiGameObject>()->DispatchMessage(pObject, msg);

        for (const xiiComponent* pComponent : pObject->GetComponents())
        {
          if (pComponent == nullptr || !pComponent->IsActiveAndInitialized())
            continue;
          const auto* renderComponent = xiiDynamicCast<const xiiRenderComponent*>(pComponent);
          const auto  categories      = renderComponent ? renderComponent->GetRenderExtractionCategories() : xiiBitflags<xiiRenderExtractionCategory>(xiiRenderExtractionCategory::Custom);
          if (!requirements.IsAnySet(categories))
          {
            ++viewDetail.m_pView->m_ExtractionStatistics.m_uiSkippedComponents;
            continue;
          }

          const xiiComponentHandle hComponent = pComponent->GetHandle();

          const bool independent = renderComponent && renderComponent->IsRenderDataViewIndependent();
          if (independent)
          {
            xiiDynamicArray<SharedPacket>* shared = nullptr;
            if (!m_SharedFramePackets.TryGetValue(hComponent, shared) && pObject->IsStatic())
            {
              const CachedStaticComponentData* persistent = nullptr;
              if (m_SharedStaticPackets.TryGetValue(hComponent, persistent))
              {
                auto& packets = m_SharedFramePackets[hComponent];
                for (const auto& data : persistent->m_StaticRenderData)
                  packets.PushBack({data.Borrow(), xiiRenderData::Caching::IfStatic});
                shared = &packets;
              }
            }
            if (shared)
            {
              msg.m_hCurrentComponent = hComponent;
              for (const auto& packet : *shared)
                OnRenderDataSubmitted(msg, packet.m_Data, packet.m_Caching, false);
              msg.m_hCurrentComponent = {};
              ++viewDetail.m_pView->m_ExtractionStatistics.m_uiReusedComponents;
              continue;
            }
            m_SharedFramePackets[hComponent]; // Also caches an empty extraction for this frame.
          }

          if (ReuseCachedStaticRenderData(viewDetail.m_ExtractionCache, hComponent, *viewDetail.m_pExtractedData))
          {
            ++viewDetail.m_pView->m_ExtractionStatistics.m_uiReusedComponents;
            continue;
          }

          msg.m_hCurrentComponent = hComponent;

          pComponent->SendMessage(msg);
          ++viewDetail.m_pView->m_ExtractionStatistics.m_uiDispatchedComponents;
          if (independent && pObject->IsStatic())
          {
            const xiiDynamicArray<SharedPacket>* shared = nullptr;
            if (m_SharedFramePackets.TryGetValue(hComponent, shared) && !shared->IsEmpty())
            {
              CachedStaticComponentData cached;
              bool                      complete = true;
              for (const auto& packet : *shared)
              {
                if (packet.m_Caching != xiiRenderData::Caching::IfStatic)
                {
                  complete = false;
                  break;
                }
                auto clone = CloneRenderDataForCache(packet.m_Data);
                if (!clone)
                {
                  complete = false;
                  break;
                }
                cached.m_StaticRenderData.PushBack(std::move(clone));
              }
              if (complete)
                m_SharedStaticPackets.Insert(hComponent, std::move(cached));
            }
          }


          msg.m_hCurrentComponent = xiiComponentHandle();
        }

        msg.m_hCurrentObject = xiiGameObjectHandle();
      }
    }

    FinalizeViewExtractionCache(viewDetail.m_ExtractionCache);

    // Finalize and sort extracted data for this view.
    viewDetail.m_pExtractedData->SortAndBatches();

    extractionEvent.m_Type = xiiRenderWorldModuleExtractionEvent::Type::AfterViewExtraction;
    s_pRenderEvent->Broadcast(extractionEvent);
  }
}

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  if (!xiiGALDevice::HasDefaultDevice())
    return;
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  bool active = false;
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
    active |= it.Value().m_bPrepared && it.Value().m_pView->IsRenderEnabled();
  if (!active)
    return;

  // Honor compiled demand before choosing the frame's retirement/ring limits.
  if (m_RuntimeFeatures.IsAnySet(xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::RayTracing))
    if (xiiGeometryResidencyManager::EnsureInitialized().Failed() || xiiMaterialManager::EnsureInitialized().Failed())
    {
      xiiLog::Error("Could not initialize the GPU residency requested by this world's pipelines.");
      return;
    }

  // Device-global frame slices and retirement share one clock with registered graphs.
  // The frame scope serializes recording and captures all queues after world extraction.
  xiiRenderGraphFrame frame;
  if (frame.Begin().Failed())
  {
    xiiLog::Error("Could not begin the world rendering frame.");
    return;
  }
  const xiiUInt64 uiFrameIndex     = frame.GetFrameIndex();
  const xiiUInt64 uiCompletedFrame = frame.GetCompletedFrame();

  // Convert extracted engine meshes into persistent, subsystem-owned GPU scene records before
  // residency streaming runs. Stable render IDs retain previous transforms across frames.
  if (m_RuntimeFeatures.IsAnySet(xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::RayTracing))
  {
    xiiDynamicArray<xiiRenderData*> packets;
    for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
      if (it.Value().m_bPrepared && it.Value().m_pView->IsRenderEnabled())
        packets.PushBackRange(it.Value().m_pExtractedData->GetAllRenderData());
    if (UpdateGpuInstances(packets, uiFrameIndex).Failed())
    {
      xiiLog::Error("Failed to extract world GPU instances.");
      return;
    }
    for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
      m_pGpuInstances->SelectInstances(it.Value().m_pExtractedData->GetAllRenderData(), it.Value().m_pView->m_GpuInstanceSelection);
  }
  else
    m_pGpuInstances.Clear();

  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::Lighting))
  {
    xiiDynamicArray<xiiRenderData*> packets;
    for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
      if (it.Value().m_bPrepared && it.Value().m_pView->IsRenderEnabled() && it.Value().m_pView->GetRuntimeFeatures().IsSet(xiiRenderPipelineFeature::Lighting))
        packets.PushBackRange(it.Value().m_pExtractedData->GetAllRenderData());
    if (UpdateWorldLights(packets).Failed())
    {
      xiiLog::Error("Could not update world light records or their IES resources.");
      return;
    }
  }
  else
    m_pWorldLights.Clear();

  // Scene synchronization can acquire resource-backed materials that did not exist at the start
  // of the frame. Resolve their bindless textures before any view captures its material upload.
  if (m_RuntimeFeatures.IsAnySet(xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::RayTracing) && xiiMaterialManager::IsInitialized())
    xiiMaterialManager::PrepareGpuResources();

  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::GpuScene) && xiiGeometryResidencyManager::IsInitialized())
  {
    const xiiGeometryResidencyDescription& geometryDescription = xiiGeometryResidencyManager::GetConfiguration();
    xiiGeometryResidencyManager::ProcessStreaming(uiFrameIndex, uiCompletedFrame, geometryDescription.m_uiUploadBudgetPerFrameBytes);
  }

  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& viewDetail = it.Value();

    if (!viewDetail.m_bPrepared || !viewDetail.m_pView->IsRenderEnabled() || !viewDetail.m_pView->IsValid())
      continue;

    xiiRenderGraphCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;
    compileSettings.m_bEnablePassCulling  = true;
    compileSettings.m_bEnableAsyncQueues  = true;
    compileSettings.m_bEnableCompileCache = true;

    xiiStringBuilder graphError;
    viewDetail.m_pView->m_LastRenderResult = xiiRenderGraphManager::ExecuteGraph(frame, viewDetail.m_pView->m_hRenderGraph, viewDetail.m_pView.Borrow(), compileSettings, &graphError);
    if (viewDetail.m_pView->m_LastRenderResult.Failed())
    {
      viewDetail.m_pView->InvalidateTemporalHistory();
      xiiLog::Error("Failed to execute render graph for view '{}': {}", viewDetail.m_pView->GetName(), graphError);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderWorldModule);
