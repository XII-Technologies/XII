#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule)
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

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

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

void xiiRenderWorldModule::Initialize()
{
  // Register ExtractRenderData (concurrent, runs on async worker threads per component manager).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    description.m_Phase                     = xiiWorldUpdatePhase::Async;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  // Destroying views releases all per-view GPU resources (ViewPassResources, profiler, etc.)
  m_Views.Clear();
  m_ViewExtractedData.Clear();
  m_ViewExtractionCaches.Clear();
  m_uiRenderFrameIndex = 0;
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

xiiView* xiiRenderWorldModule::CreateView(xiiStringView sName)
{
  xiiUniquePtr<xiiView>                pView          = XII_DEFAULT_NEW(xiiView);
  xiiUniquePtr<xiiExtractedRenderData> pExtractedData = XII_DEFAULT_NEW(xiiExtractedRenderData);

  pView->SetName(sName);
  pView->SetExtractedRenderData(pExtractedData.Borrow());

  xiiView* pRet = pView.Borrow();

  m_Views.PushBack(std::move(pView));
  m_ViewExtractedData.PushBack(std::move(pExtractedData));
  m_ViewExtractionCaches.ExpandAndGetRef();

  return pRet;
}

void xiiRenderWorldModule::DestroyView(xiiView* pView)
{
  for (xiiUInt32 i = 0; i < m_Views.GetCount(); ++i)
  {
    if (m_Views[i].Borrow() == pView)
    {
      m_Views[i]->SetExtractedRenderData(nullptr);
      m_Views.RemoveAtAndCopy(i);
      m_ViewExtractedData.RemoveAtAndCopy(i);
      m_ViewExtractionCaches.RemoveAtAndCopy(i);
      return;
    }
  }
}

void xiiRenderWorldModule::SubmitRenderData(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching)
{
  if (pContext == nullptr)
    return;

  static_cast<xiiRenderWorldModule*>(pContext)->OnRenderDataSubmitted(msg, pRenderData, category, caching);
}

void xiiRenderWorldModule::OnRenderDataSubmitted(const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching)
{
  if (msg.m_pExtractedRenderData == nullptr || pRenderData == nullptr)
    return;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, caching);

  if (msg.m_uiViewIndex >= m_ViewExtractionCaches.GetCount())
    return;

  if (msg.m_hCurrentObject.IsInvalidated())
    return;

  ViewExtractionCache&      cache     = m_ViewExtractionCaches[msg.m_uiViewIndex];
  ExtractedObjectFrameData& frameData = cache.m_FrameObjectData[msg.m_hCurrentObject];

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

    out_extractedRenderData.AddRenderData(pCachedRenderData.Borrow(), pCachedRenderData->m_Category, xiiRenderData::Caching::IfStatic);
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

    out_extractedRenderData.AddRenderData(pCachedRenderData.Borrow(), pCachedRenderData->m_Category, xiiRenderData::Caching::IfStatic);
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

  for (ViewExtractionCache& cache : m_ViewExtractionCaches)
  {
    if (!hComponent.IsInvalidated() && !cache.m_StaticObjectCache.Contains(hOwnerObject))
    {
      RemoveCachedRenderDataForComponent(cache, hOwnerObject, hComponent);
    }
    else
    {
      RemoveCachedRenderDataForObject(cache, hOwnerObject);
    }
  }
}

void xiiRenderWorldModule::DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pObject)
{
  if (pObject == nullptr)
    return;

  for (ViewExtractionCache& cache : m_ViewExtractionCaches)
  {
    RemoveCachedRenderDataForObjectRecursive(cache, pObject);
  }
}

void xiiRenderWorldModule::DeleteAllCachedRenderData()
{
  for (ViewExtractionCache& cache : m_ViewExtractionCaches)
  {
    cache.m_StaticObjectCache.Clear();
    cache.m_StaticComponentCache.Clear();
    cache.m_ObjectToCachedComponents.Clear();
    cache.m_FrameObjectData.Clear();
  }
}

void xiiRenderWorldModule::ExtractRenderData(const xiiWorldModule::UpdateContext& context)
{
  for (xiiUInt32 i = 0; i < m_Views.GetCount(); ++i)
  {
    xiiView* pView = m_Views[i].Borrow();

    if (!pView->IsValid())
      continue;

    xiiExtractedRenderData* pExtractedData = m_ViewExtractedData[i].Borrow();
    XII_ASSERT_DEV(pExtractedData != nullptr, "xiiRenderWorldModule view cache entry must always have extracted render data.");

    ViewExtractionCache& viewCache = m_ViewExtractionCaches[i];
    viewCache.m_FrameObjectData.Clear();

    pExtractedData->Clear();

    xiiMsgExtractRenderData msg;
    msg.m_pView                    = pView;
    msg.m_pExtractedRenderData     = pExtractedData;
    msg.m_uiViewIndex              = i;
    msg.m_SubmitRenderDataFunction = &xiiRenderWorldModule::SubmitRenderData;
    msg.m_pSubmitRenderDataContext = this;

    // Broadcast to all objects, each object routes to matching component message handlers.
    {
      XII_LOCK(GetWorld()->GetReadMarker());

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        xiiGameObject*            pObject = it;
        const xiiGameObjectHandle hObject = pObject->GetHandle();

        msg.m_hCurrentObject    = hObject;
        msg.m_hCurrentComponent = xiiComponentHandle();

        if (ReuseCachedStaticRenderData(viewCache, hObject, *pExtractedData))
        {
          msg.m_hCurrentObject = xiiGameObjectHandle();
          continue;
        }

        // Dispatch to object-level handlers explicitly; component dispatch is handled below.
        if (const xiiRTTI* pObjectType = pObject->GetDynamicRTTI(); pObjectType != nullptr)
        {
          pObjectType->DispatchMessage(pObject, msg);
        }

        for (xiiComponent* pComponent : pObject->GetComponents())
        {
          if (pComponent == nullptr)
            continue;

          const xiiComponentHandle hComponent = pComponent->GetHandle();

          if (ReuseCachedStaticRenderData(viewCache, hComponent, *pExtractedData))
            continue;

          msg.m_hCurrentComponent = hComponent;
          pComponent->SendMessage(msg);
          msg.m_hCurrentComponent = xiiComponentHandle();
        }

        msg.m_hCurrentObject = xiiGameObjectHandle();
      }
    }

    FinalizeViewExtractionCache(viewCache);

    // Finalize and sort extracted data for this view.
    pExtractedData->SortAndBatches();
  }
}

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt64 uiFrameIndex = m_uiRenderFrameIndex++;

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph        = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard    = pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = pView->GetResourceCache();

    // Clear the per-view blackboard at the start of each frame so passes start clean.
    // History data lives in persistent GPU resources inside ViewPassResources, not here.
    blackboard.Clear();
    blackboard.Set(xiiRGBlackboardKeys::k_FrameIndex, static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    const xiiView::RenderGraphBuilder& graphBuilder = pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      // Application-supplied custom graph (e.g. editor, headless, cinematic passes).
      graphBuilder(*pView, *pGraph, blackboard);
    }
    else
    {
      // Standard full-featured pipeline.
      pView->BuildDefaultRenderGraph(*pGraph, blackboard);
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling  = true;
    compileSettings.m_bEnablePassCulling   = true;
    compileSettings.m_bEnableSplitBarriers = true;
    compileSettings.m_bEnableAsyncQueues   = true;
    compileSettings.m_bEnableCompileCache  = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      // Pass the view's per-view profiler into the executor so every pass gets Duration bracketed.
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache, &pView->GetProfiler());
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());

      // Tick the profiler so it advances its ring and schedules readback on the oldest slot.
      pView->GetProfiler().OnFrameEnd(uiFrameIndex);
    }
  }
}
