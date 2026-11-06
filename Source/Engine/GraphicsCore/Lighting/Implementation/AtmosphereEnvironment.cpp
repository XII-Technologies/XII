/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/AtmosphereEnvironment.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/AtmosphereEnvironmentConstants.h>
#include <utility>

xiiAtmosphereEnvironment::xiiAtmosphereEnvironment(xiiSharedPtr<const xiiAtmosphereLookup> lookup, xiiSharedPtr<const xiiEnvironmentMap> environment, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters) :
  m_Lookup(std::move(lookup)), m_Environment(std::move(environment)), m_DirectionAndIlluminance(directionAndIlluminance), m_Color(color), m_fAltitudeMeters(altitudeMeters)
{
}

bool xiiAtmosphereEnvironment::Matches(const xiiSharedPtr<const xiiAtmosphereLookup>& lookup, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters) const
{
  return m_Lookup == lookup && m_DirectionAndIlluminance == directionAndIlluminance && m_Color == color && m_fAltitudeMeters == altitudeMeters;
}

xiiStatus xiiAtmosphereEnvironment::Create(const xiiSharedPtr<const xiiAtmosphereLookup>& lookup, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters, xiiSharedPtr<const xiiAtmosphereEnvironment>& out_environment)
{
  out_environment.Clear();
  if (!lookup || !directionAndIlluminance.IsValid() || directionAndIlluminance.GetAsVec3().GetLengthSquared() < 1e-12f || directionAndIlluminance.w < 0 || !color.IsValid() || color.x < 0 || color.y < 0 || color.z < 0 || !xiiMath::IsFinite(altitudeMeters) || altitudeMeters < 0 || !xiiGALDevice::HasDefaultDevice())
    return xiiStatus("Atmosphere environment initialization requires valid lookup, solar and altitude settings.");
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (lookup->m_Transmittance->GetDevice() != device)
    return xiiStatus("Physical sky lookup images belong to another graphics device.");
  auto*      cache = xiiRenderGraphManager::GetResourceCache();
  const auto frame = xiiRenderGraphManager::GetActiveFrameIndex();
  if (!cache || !frame)
    return xiiStatus("Atmosphere environment initialization requires an active shared GPU frame.");
  xiiSharedPtr<xiiGALComputePipelineState> pipeline;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, "Shaders/Pipeline/AtmosphereEnvironment.xiiShader"));
  xiiSharedPtr<xiiGALSampler> sampler;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(sampler));
  xiiGALTextureCreationDescription image;
  image.m_Type               = xiiGALResourceDimension::TextureCube;
  image.m_Size               = {64, 64};
  image.m_uiArraySizeOrDepth = 6;
  image.m_uiMipLevels        = 1;
  image.m_Format             = xiiGALResourceFormat::RGBA32Float;
  image.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  auto source                = device->CreateTexture(image);
  auto ready                 = device->CreateFence({.m_Type = xiiGALFenceType::General});
  if (!source || !ready)
    return xiiStatus("Could not allocate immutable physical sky capture or readiness fence.");
  xiiRenderGraph graph;
  graph.BeginSetup(frame);
  struct Data
  {
    xiiSharedPtr<const xiiAtmosphereLookup>  m_Lookup;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Sampler;
    xiiSharedPtr<xiiGALFence>                m_Ready;
    xiiAtmosphereEnvironmentConstants        m_Settings;
    xiiRenderGraphBufferHandle               m_Atmosphere, m_Constants;
    xiiRenderGraphTextureHandle              m_Transmittance, m_MultiScatter, m_Source;
  };
  graph.AddPass<Data>("Capture immutable physical sky", xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Lookup = lookup; data.m_Pipeline = pipeline; data.m_Sampler = sampler; data.m_Ready = ready;
    data.m_Settings.CaptureDirectionAndIlluminance = directionAndIlluminance;
    data.m_Settings.CaptureColorAndAltitudeMeters = xiiVec4(color, altitudeMeters);
    data.m_Settings.CaptureExtent = xiiVec4U32(64, 64, 0, 0);
    data.m_Atmosphere = xiiRenderPassUtils::Constants(builder, "Sky atmosphere settings", sizeof(xiiAtmosphereConstants));
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Sky capture settings", sizeof(xiiAtmosphereEnvironmentConstants));
    builder.WaitForFence(lookup->m_Ready, 1).AssertSuccess();
    data.m_Transmittance = builder.ReadTexture(builder.ImportTexture("Sky transmittance", lookup->m_Transmittance, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    data.m_MultiScatter = builder.ReadTexture(builder.ImportTexture("Sky multiple scattering", lookup->m_MultiScatter, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    data.m_Source = builder.WriteTexture(builder.ImportTexture("Captured sky", source, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
    builder.ExportTexture(data.m_Source, xiiGALResourceStateFlags::ShaderResource); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiAtmosphereConstants> mapped(commands, execute.GetBuffer(data.m_Atmosphere), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Lookup->m_Constants; }
    { xiiGALMapHelper<xiiAtmosphereEnvironmentConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Settings; }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", execute.GetBuffer(data.m_Atmosphere), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiAtmosphereEnvironmentConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetSampler("LinearClampSampler", data.m_Sampler.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("g_Transmittance", execute.GetTexture(data.m_Transmittance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_MultiScatter", execute.GetTexture(data.m_MultiScatter)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    xiiGALTextureViewCreationDescription view;
    view.m_ViewType = xiiGALTextureViewType::UnorderedAccess;
    view.m_ResourceDimension = xiiGALResourceDimension::Texture2DArray;
    auto target = execute.GetTexture(data.m_Source)->CreateView(view);
    if (!target) { execute.ReportFailure("Could not create physical sky capture view."); return; }
    commands.ResolveAndSetUnorderedAccessTextureView("g_CapturedSky", target.Borrow(), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind physical sky capture resources."); return; }
    commands.DispatchCompute({8, 8, 6});
    commands.EnqueueSignal(data.m_Ready.Borrow(), 1); });
  graph.EndSetup();
  xiiRenderGraphBlackboard blackboard;
  xiiStringBuilder         error;
  if (graph.Compile({}, &error).Failed() || graph.Execute(device.Borrow(), nullptr, &blackboard, cache, nullptr, &error).Failed())
    return xiiStatus(error.GetView());
  xiiSharedPtr<const xiiEnvironmentMap> environment;
  XII_SUCCEED_OR_RETURN(xiiEnvironmentMap::Create(source, environment, ready));
  out_environment = XII_DEFAULT_NEW(xiiAtmosphereEnvironment, lookup, std::move(environment), directionAndIlluminance, color, altitudeMeters);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_AtmosphereEnvironment);
