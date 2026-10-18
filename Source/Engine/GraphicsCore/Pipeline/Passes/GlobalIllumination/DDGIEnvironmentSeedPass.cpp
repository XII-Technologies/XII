/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIEnvironmentSeedPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIEnvironmentSeedPass,1,xiiRTTIDefaultAllocator<xiiDDGIEnvironmentSeedPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeConstants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight",m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeUpdates",m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialProbeStates",m_StatesInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialIrradiance",m_IrradianceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialDistance",m_DistanceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeStates",m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance",m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Distance",m_Distance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDDGIEnvironmentSeedPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiDDGIProbePassUtils::Snapshot(context, m_Schedule);
  if (!snapshot || !xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("DDGI seeding requires a valid scheduling snapshot and device.");
  const auto  constants = context.GetInputBuffer(m_Constants), environment = context.GetInputBuffer(m_Environment), updates = context.GetInputBuffer(m_Updates), states = context.GetInputBuffer(m_StatesInput);
  const auto* c          = context.m_Graph.GetBufferDescription(constants);
  const auto* e          = context.m_Graph.GetBufferDescription(environment);
  const auto  irradiance = context.GetInputTexture(m_IrradianceInput), distance = context.GetInputTexture(m_DistanceInput);
  const auto* a = context.m_Graph.GetTextureDescription(irradiance);
  const auto* b = context.m_Graph.GetTextureDescription(distance);
  if (!c || c->m_uiSize != sizeof(xiiDDGIProbeConstants) || !e || e->m_uiSize != sizeof(xiiEnvironmentLightConstants) || !xiiDDGIProbePassUtils::Buffer(context, updates, sizeof(xiiGpuDDGIProbeUpdate), snapshot->m_Updates.GetCount(), xiiGALBindFlags::ShaderResource) || !xiiDDGIProbePassUtils::Buffer(context, states, sizeof(xiiGpuDDGIProbeState), snapshot->m_Probes.GetCount(), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess) || !a || !b || a->m_Size != snapshot->m_AtlasSize || b->m_Size != snapshot->m_AtlasSize || irradiance.m_Id == distance.m_Id)
    return context.ReportFailure("DDGI seed resources must exactly match the scheduling snapshot and have distinct atlas allocations.");
  xiiHashSet<xiiUInt32> seen;
  for (const auto& update : snapshot->m_Updates)
  {
    if (update.m_vMetadata.x >= snapshot->m_Probes.GetCount() || seen.Contains(update.m_vMetadata.x))
      return context.ReportFailure("DDGI update slots must be unique and within the probe domain.");
    seen.Insert(update.m_vMetadata.x);
  }
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  groups = (snapshot->m_Updates.GetCount() + 63U) / 64U;
  if (groups > limits.m_uiMaxThreadGroupCountX || limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64)
    return context.ReportFailure("DDGI seed budget exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/DDGIEnvironmentSeed.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle                m_Constants, m_Environment, m_Updates, m_States;
    xiiRenderGraphTextureHandle               m_Irradiance, m_Distance;
    xiiSharedPtr<xiiGALComputePipelineState>  m_Pipeline;
    xiiSharedPtr<const xiiDDGIProbeFrameData> m_Snapshot;
    xiiUInt32                                 m_Groups;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Pipeline=m_Pipeline; data.m_Snapshot=snapshot; data.m_Groups=groups;
    data.m_Constants=builder.ReadBuffer(constants,xiiGALResourceStateFlags::ConstantBuffer); data.m_Environment=builder.ReadBuffer(environment,xiiGALResourceStateFlags::ConstantBuffer); data.m_Updates=builder.ReadBuffer(updates,xiiGALResourceStateFlags::ShaderResource);
    data.m_States=builder.ReadWriteBuffer(states,xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Irradiance=builder.ReadWriteTexture(irradiance,xiiGALResourceStateFlags::UnorderedAccess); data.m_Distance=builder.ReadWriteTexture(distance,xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Environment),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_ProbeUpdates",execute.GetBuffer(data.m_Updates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_ProbeStates",execute.GetBuffer(data.m_States)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Irradiance",execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Distance",execute.GetTexture(data.m_Distance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute({data.m_Groups,1,1});
    data.m_Snapshot->CommitUpdates(); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_States, pass.first->m_States));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Irradiance, pass.first->m_Irradiance));
  return context.SetOutput(m_Distance, pass.first->m_Distance);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_DDGIEnvironmentSeedPass);
