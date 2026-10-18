/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Residency/GeometryUploadPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGeometryUploadPass, 1, xiiRTTIDefaultAllocator<xiiGeometryUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Residency") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiGeometryUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  const auto                 uploaded = xiiGeometryResidencyManager::AddUploadPass(context.m_Graph, context.m_Graph.GetFrameIndex());
  if (!uploaded.m_hGeometryMetadata.IsValid() || !uploaded.m_hMeshletMetadata.IsValid())
    return context.ReportFailure("Could not publish device geometry residency.");
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Geometry, uploaded.m_hGeometryMetadata));
  return context.SetOutput(m_Meshlets, uploaded.m_hMeshletMetadata);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Residency_GeometryUploadPass);
