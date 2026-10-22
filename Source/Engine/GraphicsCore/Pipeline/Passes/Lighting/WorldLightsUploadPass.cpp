/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/WorldLightsUploadPass.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldLightsUploadPass,1,xiiRTTIDefaultAllocator<xiiWorldLightsUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Lights",m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IESProfiles",m_Profiles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Counts",m_Counts)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates",m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiWorldLightsUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetWorld() || !context.m_pView->GetExtractedRenderData())
    return context.ReportFailure("World light upload requires an extracted authoritative view.");
  const auto* module = static_cast<const xiiWorld*>(context.m_pView->GetWorld())->GetModule<xiiRenderWorldModule>();
  const auto* lights = module ? module->GetWorldLightData() : nullptr;
  if (!lights)
    return context.ReportFailure("World light extraction has not prepared this frame.");
  const auto uploaded = lights->AddUploadPass(context.m_Graph);
  if (!uploaded.m_Lights.IsValid() || !uploaded.m_Profiles.IsValid() || !uploaded.m_Counts.IsValid())
    return context.ReportFailure("Could not upload the world light snapshot.");
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Lights, uploaded.m_Lights));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Profiles, uploaded.m_Profiles));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Counts, uploaded.m_Counts));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Candidates;
    xiiDynamicArray<xiiUInt32> m_Indices;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    xiiHashSet<xiiUInt32> seen;
    for(const auto* packet:context.m_pView->GetExtractedRenderData()->GetAllRenderData())
    {
      const auto index=lights->GetIndex(packet->m_hOwnerComponent);
      if(index!=xiiInvalidIndex && !seen.Contains(index)) { seen.Insert(index); data.m_Indices.PushBack(index); }
    }
    if(data.m_Indices.IsEmpty()) data.m_Indices.PushBack(xiiInvalidIndex);
    xiiGALBufferCreationDescription description; description.m_uiSize=data.m_Indices.GetCount()*sizeof(xiiUInt32); description.m_uiElementByteStride=sizeof(xiiUInt32);
    description.m_Mode=xiiGALBufferMode::Structured; description.m_Usage=xiiGALResourceUsage::Mutable; description.m_BindFlags=xiiGALBindFlags::ShaderResource;
    data.m_Candidates=builder.WriteBuffer("View Light Candidates",description,xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Candidates), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Indices.GetData()), data.m_Indices.GetCount() * sizeof(xiiUInt32))); });
  return context.SetOutput(m_Candidates, pass.first->m_Candidates);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_WorldLightsUploadPass);
