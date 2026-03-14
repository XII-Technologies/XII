#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/Extractors/VisibleObjectsExtractor.h>

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

        for (xiiBoundingBox& box : boxes)
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

  for (const xiiGameObject* pObject : visibleObjects)
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
