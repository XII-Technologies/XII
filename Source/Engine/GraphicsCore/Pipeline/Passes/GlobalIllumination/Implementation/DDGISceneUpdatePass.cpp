/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGISceneUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGISceneUpdatePass, 1, xiiRTTIDefaultAllocator<xiiDDGISceneUpdatePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AllowAbsentSchedule", m_bAllowAbsentSchedule)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Schedule", m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeConstants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeUpdates", m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialProbeStates", m_StatesInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SourceRadiance", m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialIrradiance", m_IrradianceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialDistance", m_DistanceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeStates", m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance", m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Distance", m_Distance)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiDDGISceneUpdatePass::Build(xiiRenderPipelinePassContext& context)
{
  if (m_bAllowAbsentSchedule && !context.GetInputData(m_Schedule))
    return XII_SUCCESS;
  const auto snapshot = xiiDDGIProbePassUtils::Snapshot(context, m_Schedule);
  const auto device   = xiiGALDevice::GetDefaultDevice();
  if (!snapshot || !snapshot->m_bDirectional || !device)
    return context.ReportFailure("DDGI scene updates require a directional probe schedule and a device.");
  const auto  depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), radiance = context.GetInputTexture(m_Radiance);
  const auto* scene  = xiiScreenSpacePassUtils::Image(context, depth, 1);
  const auto  camera = context.GetInputBuffer(m_Camera), constants = context.GetInputBuffer(m_Constants);
  if (!scene || !xiiScreenSpacePassUtils::Image(context, normal, 2, &scene->m_Size) || !xiiScreenSpacePassUtils::Image(context, radiance, 3, &scene->m_Size) ||
      !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)) || !xiiScreenSpacePassUtils::Uniform(context, constants, sizeof(xiiDDGIProbeConstants)))
    return context.ReportFailure("DDGI scene updates require matching depth, normal, radiance, and camera resources.");
  const auto  updates = context.GetInputBuffer(m_Updates), states = context.GetInputBuffer(m_StatesInput);
  const auto  irradiance = context.GetInputTexture(m_IrradianceInput), distance = context.GetInputTexture(m_DistanceInput);
  const auto* irradianceDescription = context.m_Graph.GetTextureDescription(irradiance);
  const auto* distanceDescription   = context.m_Graph.GetTextureDescription(distance);
  if (!xiiDDGIProbePassUtils::Buffer(context, updates, sizeof(xiiGpuDDGIProbeUpdate), snapshot->m_Updates.GetCount(), xiiGALBindFlags::ShaderResource) ||
      !xiiDDGIProbePassUtils::Buffer(context, states, sizeof(xiiGpuDDGIProbeState), snapshot->m_Probes.GetCount(), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess) ||
      !irradianceDescription || !distanceDescription || irradianceDescription->m_Size != snapshot->m_AtlasSize || distanceDescription->m_Size != snapshot->m_AtlasSize ||
      irradiance.GetResourceId() == distance.GetResourceId())
    return context.ReportFailure("DDGI scene update resources must match the directional probe schedule.");
  const auto& limits = device->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  groups = snapshot->m_Updates.GetCount();
  if (groups > limits.m_uiMaxThreadGroupCountX || limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64)
    return context.ReportFailure("DDGI scene update budget exceeds compute dispatch limits.");
  xiiHashSet<xiiUInt32> seen;
  for (const auto& update : snapshot->m_Updates)
  {
    if (update.m_vMetadata.x >= snapshot->m_Probes.GetCount() || seen.Contains(update.m_vMetadata.x))
      return context.ReportFailure("DDGI scene update slots must be unique and within the probe domain.");
    seen.Insert(update.m_vMetadata.x);
  }
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/DDGISceneUpdate.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle                m_Camera, m_Constants, m_Updates, m_States;
    xiiRenderGraphTextureHandle               m_Depth, m_Normal, m_Radiance, m_Irradiance, m_Distance;
    xiiSharedPtr<xiiGALComputePipelineState>  m_Pipeline;
    xiiSharedPtr<const xiiDDGIProbeFrameData> m_Snapshot;
    xiiUInt32                                 m_uiGroups;
  };
  const auto setup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Camera     = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants  = builder.ReadBuffer(constants, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Updates    = builder.ReadBuffer(updates, xiiGALResourceStateFlags::ShaderResource);
    data.m_States     = builder.ReadWriteBuffer(states, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Depth      = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal     = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Radiance   = builder.ReadTexture(radiance, xiiGALResourceStateFlags::ShaderResource);
    data.m_Irradiance = builder.ReadWriteTexture(irradiance, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Distance   = builder.ReadWriteTexture(distance, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Snapshot   = snapshot;
    data.m_Pipeline   = m_Pipeline;
    data.m_uiGroups   = groups;
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_ProbeUpdates", execute.GetBuffer(data.m_Updates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_ProbeStates", execute.GetBuffer(data.m_States)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Normal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Radiance", execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Irradiance", execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Distance", execute.GetTexture(data.m_Distance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind directional probe scene update resources.");
      return;
    }
    commands.DispatchCompute({data.m_uiGroups, 1, 1});
    data.m_Snapshot->CommitUpdates();
  };
  const auto emitted = context.m_Graph.AddPass<Data>("Update Directional Scene Probes", xiiGALCommandQueueFlags::Compute, setup, record);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_States, emitted.first->m_States));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Irradiance, emitted.first->m_Irradiance));
  return context.SetOutput(m_Distance, emitted.first->m_Distance);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_Implementation_DDGISceneUpdatePass);
