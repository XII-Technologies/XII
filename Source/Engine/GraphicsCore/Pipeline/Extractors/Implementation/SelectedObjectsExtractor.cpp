#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/Extractors/SelectedObjectsExtractor.h>

namespace
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSelectedObject(const xiiView& view, const xiiGameObject* pObject, bool bVisualizeBounds, bool bVisualizeLocalBox, bool bVisualizeData, bool bVisualizeDataOnlyCategory)
  {
    if (!bVisualizeBounds && !bVisualizeLocalBox && !bVisualizeData)
      return;

    if (bVisualizeLocalBox)
    {
      const xiiBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        xiiDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), xiiColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (bVisualizeBounds)
    {
      const xiiBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        xiiDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), xiiColor::Lime);
        xiiDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), xiiColor::Magenta);
      }
    }

    if (bVisualizeData && bVisualizeDataOnlyCategory)
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


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectedObjectsExtractorBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSelectedObjectsExtractorBase::xiiSelectedObjectsExtractorBase(xiiStringView sName) :
  xiiExtractor(sName), m_OverrideCategory(xiiDefaultRenderDataCategories::Selection)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_pSpatialVisBoundsCVar           = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("Spatial.VisBounds"));
  m_pSpatialVisLocalBBoxCVar        = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("Spatial.VisLocalBBox"));
  m_pSpatialVisDataCVar             = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("Spatial.VisData"));
  m_pSpatialVisDataOnlySelectedCVar = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("Spatial.VisData.OnlySelected"));
  m_pSpatialVisDataOnlyCategoryCVar = static_cast<xiiCVarString*>(xiiCVar::FindCVarByName("Spatial.VisData.OnlyCategory"));
#endif
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

  for (const xiiGameObjectHandle& hObject : *pSelection)
  {
    const xiiGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObject, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (m_pSpatialVisBoundsCVar && m_pSpatialVisLocalBBoxCVar && m_pSpatialVisDataCVar && m_pSpatialVisDataOnlySelectedCVar && m_pSpatialVisDataOnlyCategoryCVar)
    {
      if (m_pSpatialVisDataOnlySelectedCVar->GetValue())
      {
        VisualizeSelectedObject(view, pObject, m_pSpatialVisBoundsCVar->GetValue(), m_pSpatialVisLocalBBoxCVar->GetValue(), m_pSpatialVisDataCVar->GetValue(), m_pSpatialVisDataOnlyCategoryCVar->GetValue().IsEmpty());
      }
    }
  }
#endif
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
    const xiiGameObject* pObject;
    if (world.TryGetObject(m_Objects[i], pObject) == false)
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
  const xiiGameObject* pObject;
  if (world.TryGetObject(hObject, pObject))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
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

//////////////////////////////////////////////////////////////////////////

xiiSelectedObjectsExtractor::xiiSelectedObjectsExtractor(xiiStringView sName /*= "ExplicitlySelectedObjectsExtractor"*/) :
  xiiSelectedObjectsExtractorBase(sName)
{
}

xiiSelectedObjectsExtractor::~xiiSelectedObjectsExtractor() = default;

const xiiDeque<xiiGameObjectHandle>* xiiSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext)
  {
    return &m_pSelectionContext->m_Objects;
  }
  return nullptr;
}
