/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeUploadPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIProbeUploadPass,1,xiiRTTIDefaultAllocator<xiiDDGIProbeUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeStates",m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeUpdates",m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeConstants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDDGIProbeUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiDDGIProbePassUtils::Snapshot(context, m_Schedule);
  if (!snapshot)
    return context.ReportFailure("DDGI upload requires a valid current scheduling snapshot.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSharedPtr<const xiiDDGIProbeFrameData> m_Snapshot;
    xiiRenderGraphBufferHandle                m_States, m_Updates, m_Constants;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot;
    xiiGALBufferCreationDescription d; d.m_Mode=xiiGALBufferMode::Structured; d.m_Usage=xiiGALResourceUsage::Mutable;
    d.m_uiElementByteStride=sizeof(xiiGpuDDGIProbeState); d.m_uiSize=snapshot->m_Probes.GetCount()*sizeof(xiiGpuDDGIProbeState); d.m_BindFlags=xiiGALBindFlags::ShaderResource|xiiGALBindFlags::UnorderedAccess;
    data.m_States=builder.WriteBuffer("Probe States",d,xiiGALResourceStateFlags::CopyDestination);
    d.m_uiElementByteStride=sizeof(xiiGpuDDGIProbeUpdate); d.m_uiSize=snapshot->m_Updates.GetCount()*sizeof(xiiGpuDDGIProbeUpdate); d.m_BindFlags=xiiGALBindFlags::ShaderResource;
    data.m_Updates=builder.WriteBuffer("Probe Updates",d,xiiGALResourceStateFlags::CopyDestination);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Probe Constants",sizeof(xiiDDGIProbeConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    commands.UpdateBuffer(execute.GetBuffer(data.m_States),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Probes.GetData()),data.m_Snapshot->m_Probes.GetCount()*sizeof(xiiGpuDDGIProbeState)));
    commands.UpdateBuffer(execute.GetBuffer(data.m_Updates),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Updates.GetData()),data.m_Snapshot->m_Updates.GetCount()*sizeof(xiiGpuDDGIProbeUpdate)));
    { xiiGALMapHelper<xiiDDGIProbeConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot->m_Constants; }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Constants); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_States, pass.first->m_States));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Updates, pass.first->m_Updates));
  return context.SetOutput(m_Constants, pass.first->m_Constants);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_DDGIProbeUploadPass);
