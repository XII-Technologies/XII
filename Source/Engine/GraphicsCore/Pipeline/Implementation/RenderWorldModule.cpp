/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
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

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

void xiiRenderWorldModule::Initialize()
{
  // Compile CPU-only view demand before scene updates. Extract only after authoritative world
  // transforms are final, then record/execute after extraction in the same PostTransform phase.
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::PrepareExtraction, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PreAsync;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostTransform;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete).
  {
    auto description    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    description.m_Phase = xiiWorldUpdatePhase::PostTransform;
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

  xiiDebugRenderer::ClearContext(GetWorld());
}

void xiiRenderWorldModule::OnSimulationStarted()
{
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

void xiiRenderWorldModule::OnRenderDataSubmitted(const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  if (msg.m_pView == nullptr)
    return;

  if (msg.m_pExtractedRenderData == nullptr || pRenderData == nullptr)
    return;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, caching);

  if (msg.m_hCurrentObject.IsInvalidated())
    return;

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
  cache.m_StaticObjectCache.Remove(hObject);

  xiiDynamicArray<xiiComponentHandle>* pCachedComponents = nullptr;
  if (cache.m_ObjectToCachedComponents.TryGetValue(hObject, pCachedComponents) && pCachedComponents != nullptr)
  {
    for (xiiComponentHandle hCachedComponent : *pCachedComponents)
    {
      cache.m_StaticComponentCache.Remove(hCachedComponent);
    }
  }

  cache.m_ObjectToCachedComponents.Remove(hObject);
}

void xiiRenderWorldModule::RemoveCachedRenderDataForComponent(ViewExtractionCache& cache, xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent)
{
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

void xiiRenderWorldModule::RemoveCachedRenderDataForObjectRecursive(ViewExtractionCache& cache, const xiiGameObject* pObject)
{
  if (pObject == nullptr)
    return;

  RemoveCachedRenderDataForObject(cache, pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    RemoveCachedRenderDataForObjectRecursive(cache, it);
  }
}

void xiiRenderWorldModule::DeleteCachedRenderData(xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent)
{
  if (hOwnerObject.IsInvalidated())
    return;

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

void xiiRenderWorldModule::DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    RemoveCachedRenderDataForObjectRecursive(it.Value().m_ExtractionCache, pObject);
  }
}

void xiiRenderWorldModule::DeleteAllCachedRenderData()
{
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& value = it.Value();

    value.m_ExtractionCache.m_StaticObjectCache.Clear();
    value.m_ExtractionCache.m_StaticComponentCache.Clear();
    value.m_ExtractionCache.m_ObjectToCachedComponents.Clear();
    value.m_ExtractionCache.m_FrameObjectData.Clear();
  }
}

void xiiRenderWorldModule::PrepareExtraction(const xiiWorldModule::UpdateContext&)
{
  m_ExtractionRequirements = {};
  m_RuntimeFeatures        = {};
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& detail                  = it.Value();
    auto& view                    = *detail.m_pView;
    view.m_ExtractionRequirements = {};
    view.m_RuntimeFeatures        = {};
    view.m_ExtractionStatistics   = {};
    if (view.IsRenderEnabled() && view.IsValid() && view.SynchronizeRenderPipeline().Succeeded())
    {
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
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& viewDetail = it.Value();

    if (!viewDetail.m_pView->IsRenderEnabled() || !viewDetail.m_pView->IsValid())
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

          if (ReuseCachedStaticRenderData(viewDetail.m_ExtractionCache, hComponent, *viewDetail.m_pExtractedData))
          {
            ++viewDetail.m_pView->m_ExtractionStatistics.m_uiReusedComponents;
            continue;
          }

          msg.m_hCurrentComponent = hComponent;

          pComponent->SendMessage(msg);
          ++viewDetail.m_pView->m_ExtractionStatistics.m_uiDispatchedComponents;

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

  if (m_ViewIdTable.IsEmpty())
    return;

  // Device-global frame slices and retirement share one clock with registered graphs.
  // Extraction remains parallel; the frame scope serializes recording and captures all queues.
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
  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    ViewDetail& viewDetail = it.Value();
    if (viewDetail.m_pView->IsRenderEnabled() && viewDetail.m_pView->IsValid())
    {
      if (viewDetail.m_pView->PrepareRenderResources().Failed())
        continue;
      XII_IGNORE_UNUSED(viewDetail.m_pView->SynchronizeGpuScene(uiFrameIndex));
    }
  }

  // Scene synchronization can acquire resource-backed materials that did not exist at the start
  // of the frame. Resolve their bindless textures before any view captures its material upload.
  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::GpuScene) && xiiMaterialManager::IsInitialized())
    xiiMaterialManager::PrepareGpuResources();

  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::GpuScene) && xiiGeometryResidencyManager::IsInitialized())
  {
    const xiiGeometryResidencyDescription& geometryDescription = xiiGeometryResidencyManager::GetConfiguration();
    xiiGeometryResidencyManager::ProcessStreaming(uiFrameIndex, uiCompletedFrame, geometryDescription.m_uiUploadBudgetPerFrameBytes);
  }

  for (auto it = m_ViewIdTable.GetIterator(); it.IsValid(); ++it)
  {
    auto& viewDetail = it.Value();

    if (!viewDetail.m_pView->IsRenderEnabled() || !viewDetail.m_pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph        = viewDetail.m_pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard    = viewDetail.m_pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = viewDetail.m_pView->GetResourceCache();
    resourceCache.BeginFrame(uiFrameIndex, uiCompletedFrame);

    // Clear the per-view blackboard at the start of each frame so passes start clean.
    // History data lives in persistent GPU resources inside the pipeline runtime, not here.
    blackboard.Clear();
    blackboard.Set(xiiRGBlackboardKeys::k_FrameIndex, static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    // Publish virtual-shadow residency before the view builds its passes. Consumers
    // can therefore declare proper render-graph reads and inherit the transfer
    // dependency without reaching into the manager during execution.
    const xiiVirtualShadowMapManager::UploadHandles virtualShadowHandles = viewDetail.m_pView->GetRuntimeFeatures().IsSet(xiiRenderPipelineFeature::VirtualShadows) ? xiiVirtualShadowMapManager::AddUploadPass(*pGraph, uiFrameIndex) : xiiVirtualShadowMapManager::UploadHandles{};
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowPhysicalBaseIndex, virtualShadowHandles.m_uiFrameBaseIndex);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowPhysicalPageCount, virtualShadowHandles.m_uiPhysicalPageCount);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowTableBaseIndex, virtualShadowHandles.m_uiVirtualTableBaseIndex);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowTableCapacity, virtualShadowHandles.m_uiVirtualTableCapacity);

    const xiiView::RenderGraphBuilder& graphBuilder = viewDetail.m_pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      // Application-supplied custom graph (e.g. editor, headless, cinematic passes).
      graphBuilder(*viewDetail.m_pView, *pGraph, blackboard);
    }
    else
    {
      // Standard full-featured pipeline.
      if (viewDetail.m_pView->BuildRenderPipeline(*pGraph, blackboard, virtualShadowHandles).Failed())
      {
        pGraph->EndSetup();
        resourceCache.EndFrame();
        xiiLog::Error("Failed to build render pipeline for view '{}'.", viewDetail.m_pView->GetName());
        continue;
      }
    }

    pGraph->EndSetup();

    xiiRenderGraphCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;
    compileSettings.m_bEnablePassCulling  = true;
    compileSettings.m_bEnableAsyncQueues  = true;
    compileSettings.m_bEnableCompileCache = true;

    xiiStringBuilder graphError;
    if (pGraph->Compile(compileSettings, &graphError).Succeeded())
    {
      // Pass the view's per-view profiler into the executor so every pass gets Duration bracketed.
      if (pGraph->Execute(pDevice, viewDetail.m_pView.Borrow(), &blackboard, &resourceCache, &viewDetail.m_pView->GetProfiler(), &graphError).Failed())
      {
        // Earlier callbacks may have recorded/promoted history before a later submission fails.
        // That state cannot be used as completed temporal input on the next view update.
        viewDetail.m_pView->InvalidateTemporalHistory();
        xiiLog::Error("Failed to execute render graph for view '{}': {}", viewDetail.m_pView->GetName(), graphError);
      }
    }
    else
    {
      xiiLog::Error("Failed to compile render graph for view '{}': {}", viewDetail.m_pView->GetName(), graphError);
    }
    resourceCache.EndFrame();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderWorldModule);
