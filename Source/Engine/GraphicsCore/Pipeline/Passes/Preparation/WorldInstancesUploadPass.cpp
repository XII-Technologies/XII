/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/WorldInstancesUploadPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldInstancesUploadPass, 1, xiiRTTIDefaultAllocator<xiiWorldInstancesUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Instances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SkinningMatrices", m_Skinning)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Preparation") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiWorldInstancesUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetGpuInstanceData())
    return context.ReportFailure("World-derived GPU instances are unavailable.");
  // AddUploadPass deduplicates by world/frame/graph identity. Keep its canonical names
  // outside the occurrence scope so two instances do not duplicate world uploads.
  const auto uploaded = context.m_pView->GetGpuInstanceData()->AddUploadPass(context.m_Graph, context.m_Graph.GetFrameIndex());
  if (!uploaded.m_Instances.IsValid() || !uploaded.m_Skinning.IsValid())
    return context.ReportFailure("Could not upload the world GPU instance snapshot.");
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Instances, uploaded.m_Instances));
  return context.SetOutput(m_Skinning, uploaded.m_Skinning);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Preparation_WorldInstancesUploadPass);
