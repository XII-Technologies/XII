#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool   cvar_SpatialVisBounds("Spatial.VisBounds", false, xiiCVarFlags::Default, "Enables debug visualization of object bounds");
xiiCVarBool   cvar_SpatialVisLocalBBox("Spatial.VisLocalBBox", false, xiiCVarFlags::Default, "Enables debug visualization of object local bounding box");
xiiCVarBool   cvar_SpatialVisData("Spatial.VisData", false, xiiCVarFlags::Default, "Enables debug visualization of the spatial data structure");
xiiCVarString cvar_SpatialVisDataOnlyCategory("Spatial.VisData.OnlyCategory", "", xiiCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
xiiCVarBool   cvar_SpatialVisDataOnlySelected("Spatial.VisData.OnlySelected", false, xiiCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
xiiCVarString cvar_SpatialVisDataOnlyObject("Spatial.VisData.OnlyObject", "", xiiCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

xiiCVarBool cvar_SpatialExtractionShowStats("Spatial.Extraction.ShowStats", false, xiiCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const xiiView& view)
  {
    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() && !cvar_SpatialVisDataOnlySelected)
    {
      const xiiSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = xiiDynamicCast<const xiiSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        xiiSpatialData::Category filterCategory = xiiSpatialData::FindCategory(cvar_SpatialVisDataOnlyCategory.GetValue());

        xiiHybridArray<xiiBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes, filterCategory);

        for (auto& box : boxes)
        {
          xiiDebugRenderer::DrawLineBox(view.GetHandle(), box, xiiColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const xiiView& view, const xiiGameObject* pObject)
  {
    if (!cvar_SpatialVisBounds && !cvar_SpatialVisLocalBBox && !cvar_SpatialVisData)
      return;

    if (cvar_SpatialVisLocalBBox)
    {
      const xiiBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        xiiDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), xiiColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (cvar_SpatialVisBounds)
    {
      const xiiBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        xiiDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), xiiColor::Lime);
        xiiDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), xiiColor::Magenta);
      }
    }

    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyCategory.GetValue().IsEmpty())
    {
      const xiiSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = xiiDynamicCast<const xiiSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        xiiBoundingBox box;
        if (pSpatialSystemGrid->GetCellBoxForSpatialData(pObject->GetSpatialData(), box).Succeeded())
        {
          xiiDebugRenderer::DrawLineBox(view.GetHandle(), box, xiiColor::Cyan);
        }
      }
    }
  }
#endif
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractor, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red)),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExtractor::xiiExtractor(xiiStringView sName)
{
  m_bActive = true;
  m_sName.Assign(sName);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData   = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

xiiExtractor::~xiiExtractor() = default;

void xiiExtractor::SetName(xiiStringView sName)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

xiiStringView xiiExtractor::GetName() const
{
  return m_sName.GetView();
}

bool xiiExtractor::FilterByViewTags(const xiiView& view, const xiiGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void xiiExtractor::ExtractRenderData(const xiiView& view, const xiiGameObject* pObject, xiiMsgExtractRenderData& msg, xiiExtractedRenderData& extractedRenderData) const
{
  auto AddRenderDataFromMessage = [&](const xiiMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, xiiRenderData::Category(data.m_uiCategory));
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  if (pObject->IsStatic())
  {
    xiiUInt16 uiComponentVersion = pObject->GetComponentVersion();

    auto cachedRenderData = xiiRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (xiiUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      XII_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        XII_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    xiiUInt32 uiCacheIndex = 0;

    auto            components      = pObject->GetComponents();
    const xiiUInt32 uiNumComponents = components.GetCount();
    for (xiiUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const xiiInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != xiiInvalidRenderDataCategory ? msg.m_OverrideCategory : xiiRenderData::Category(cacheEntry.m_uiCategory));

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const xiiComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          xiiHybridArray<xiiInternal::RenderDataCacheEntry, 16> newCacheEntries(xiiFrameAllocator::GetCurrentAllocator());

          for (xiiUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry              = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData      = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory       = msg.m_ExtractedRenderData[uiPartIndex].m_uiCategory;
            newCacheEntry.m_uiComponentIndex = static_cast<xiiUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex      = static_cast<xiiUInt16>(uiPartIndex);
          }

          xiiRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries);
        }

        AddRenderDataFromMessage(msg);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        XII_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<xiiMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        xiiInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData      = nullptr;
        dummyEntry.m_uiCategory       = xiiInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<xiiUInt16>(uiComponentIndex);

        xiiRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, xiiMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
  }
}

void xiiExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
}

void xiiExtractor::PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
}


xiiResult xiiExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return XII_SUCCESS;
}


xiiResult xiiExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisibleObjectsExtractor, 1, xiiRTTIDefaultAllocator<xiiVisibleObjectsExtractor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisibleObjectsExtractor::xiiVisibleObjectsExtractor(xiiStringView sName) :
  xiiExtractor(sName)
{
}

xiiVisibleObjectsExtractor::~xiiVisibleObjectsExtractor() = default;

void xiiVisibleObjectsExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  xiiMsgExtractRenderData msg;
  msg.m_pView = &view;

  XII_LOCK(view.GetWorld()->GetReadMarker());

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  VisualizeSpatialData(view);

  m_uiNumCachedRenderData   = 0;
  m_uiNumUncachedRenderData = 0;
#endif

  for (auto pObject : visibleObjects)
  {
    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if ((cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() || pObject->GetName().FindSubString_NoCase(cvar_SpatialVisDataOnlyObject.GetValue()) != nullptr) && !cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == xiiCameraUsageHint::MainView || view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView);

  if (cvar_SpatialExtractionShowStats && bIsMainView)
  {
    xiiViewHandle hView = view.GetHandle();

    xiiStringBuilder sb;

    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "ExtractionStats", "Extraction Stats:");

    sb.SetFormat("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "ExtractionStats", sb);

    sb.SetFormat("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "ExtractionStats", sb);
  }
#endif
}

xiiResult xiiVisibleObjectsExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiVisibleObjectsExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectedObjectsExtractorBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSelectedObjectsExtractorBase::xiiSelectedObjectsExtractorBase(xiiStringView sName) :
  xiiExtractor(sName), m_OverrideCategory(xiiDefaultRenderDataCategories::Selection)
{
}

xiiSelectedObjectsExtractorBase::~xiiSelectedObjectsExtractorBase() = default;

void xiiSelectedObjectsExtractorBase::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  const xiiDeque<xiiGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  xiiMsgExtractRenderData msg;
  msg.m_pView            = &view;
  msg.m_OverrideCategory = m_OverrideCategory;

  XII_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    const xiiGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if (cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectedObjectsContext, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectedObjectsExtractor, 1, xiiRTTIDefaultAllocator<xiiSelectedObjectsExtractor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SelectionContext", GetSelectionContext, SetSelectionContext),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSelectedObjectsContext::xiiSelectedObjectsContext()  = default;
xiiSelectedObjectsContext::~xiiSelectedObjectsContext() = default;

void xiiSelectedObjectsContext::RemoveDeadObjects(const xiiWorld& world)
{
  for (xiiUInt32 i = 0; i < m_Objects.GetCount();)
  {
    const xiiGameObject* pObj;
    if (world.TryGetObject(m_Objects[i], pObj) == false)
    {
      m_Objects.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }
}

void xiiSelectedObjectsContext::AddObjectAndChildren(const xiiWorld& world, const xiiGameObjectHandle& hObject)
{
  const xiiGameObject* pObj;
  if (world.TryGetObject(hObject, pObj))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
    {
      AddObjectAndChildren(world, it);
    }
  }
}

void xiiSelectedObjectsContext::AddObjectAndChildren(const xiiWorld& world, const xiiGameObject* pObject)
{
  m_Objects.PushBack(pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    AddObjectAndChildren(world, it);
  }
}

xiiSelectedObjectsExtractor::xiiSelectedObjectsExtractor(xiiStringView sName /*= "ExplicitlySelectedObjectsExtractor"*/) :
  xiiSelectedObjectsExtractorBase(sName)
{
}

xiiSelectedObjectsExtractor::~xiiSelectedObjectsExtractor() = default;

const xiiDeque<xiiGameObjectHandle>* xiiSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext)
    return &m_pSelectionContext->m_Objects;

  return nullptr;
}

xiiResult xiiSelectedObjectsExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiSelectedObjectsExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Extractor);
