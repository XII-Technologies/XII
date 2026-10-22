/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowRasterWorkPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowRasterWorkPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowRasterWorkPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Cascades", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Residency", m_Residency)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RasterPages", m_Pages)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightId", m_uiLightId)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 0xFFFFFFU)),
    XII_MEMBER_PROPERTY("MaxPages", m_uiMaxPages)->AddAttributes(new xiiDefaultValueAttribute(128U), new xiiClampValueAttribute(0U, 1048576U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowRasterWorkPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiLightId <= 0xFFFFFFU && m_uiMaxPages <= 1048576U ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiVirtualShadowRasterWorkPass::Build(xiiRenderPipelinePassContext& context)
{
  auto light = m_uiLightId, maxPages = m_uiMaxPages;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowRasterLightId", light));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowRasterMaxPages", maxPages));
  xiiUInt64 completed = 0;
  if (light > 0xFFFFFFU || maxPages > 1048576U || xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Shadow raster work requires a current GPU frame, a valid light and a bounded page count.");
  const auto cascades  = context.GetInputData(m_Cascades).Downcast<const xiiShadowCascadeSnapshot>();
  const auto residency = context.GetInputData(m_Residency).Downcast<const xiiVirtualShadowResidencySnapshot>();
  if (!cascades || !residency || !residency->m_Upload || residency->m_Upload->m_uiFrame != context.m_Graph.GetFrameIndex() || residency->m_Upload->m_uiConfigurationRevision != xiiVirtualShadowMapManager::GetConfigurationRevision() || cascades->m_Cascades.IsEmpty() || cascades->m_Cascades.GetCount() > 4)
    return context.ReportFailure("Shadow raster work requires current residency and explicit cascade snapshots.");
  xiiStaticArray<xiiMat4, 4> projections;
  for (const auto& cascade : cascades->m_Cascades)
  {
    if (!cascade.m_mViewProjection.IsValid())
      return context.ReportFailure("Shadow raster work received a nonfinite cascade projection.");
    projections.PushBack(cascade.m_mViewProjection);
  }
  xiiDynamicArray<xiiVirtualShadowRasterPage> pages;
  xiiVirtualShadowMapManager::GatherRasterPages(projections, light, pages, maxPages);
  return context.SetOutput(m_Pages, XII_DEFAULT_NEW(xiiVirtualShadowRasterBatch, context.m_Graph.GetSetupSerial(), residency, std::move(pages)));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowRasterWorkPass);
