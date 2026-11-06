/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/AtmosphereLookup.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/AtmospherePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <utility>

xiiAtmosphereLookup::xiiAtmosphereLookup(const xiiAtmosphereConstants& constants, xiiSharedPtr<xiiGALTexture> transmittance, xiiSharedPtr<xiiGALTexture> multiScatter, xiiSharedPtr<xiiGALFence> ready) :
  m_Constants(constants), m_Transmittance(std::move(transmittance)), m_MultiScatter(std::move(multiScatter)), m_Ready(std::move(ready))
{
}

bool xiiAtmosphereLookup::Matches(const xiiAtmosphereSettings& settings) const
{
  if (!xiiGALDevice::HasDefaultDevice() || m_Transmittance->GetDevice() != xiiGALDevice::GetDefaultDevice())
    return false;
  const auto constants = xiiRenderPassUtils::MakeAtmosphereConstants(settings);
  return xiiMemoryUtils::Compare(&m_Constants, &constants, 1) == 0;
}

xiiStatus xiiAtmosphereLookup::Create(const xiiAtmosphereSettings& settings, xiiSharedPtr<const xiiAtmosphereLookup>& out_lookup)
{
  out_lookup.Clear();
  if (!xiiIsValidAtmosphereSettings(settings) || !xiiGALDevice::HasDefaultDevice())
    return xiiStatus("Atmosphere lookup initialization requires valid physical settings and a device.");
  const auto device = xiiGALDevice::GetDefaultDevice();
  auto*      cache  = xiiRenderGraphManager::GetResourceCache();
  const auto frame  = xiiRenderGraphManager::GetActiveFrameIndex();
  if (!cache || !frame)
    return xiiStatus("Atmosphere lookup initialization requires an active shared GPU frame.");
  xiiSharedPtr<xiiGALComputePipelineState> transmittancePipeline, multiScatterPipeline;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(transmittancePipeline, "Shaders/Pipeline/Passes/Atmosphere/AtmosphereTransmittance.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(multiScatterPipeline, "Shaders/Pipeline/Passes/Atmosphere/AtmosphereMultiScatter.xiiShader"));
  xiiSharedPtr<xiiGALSampler> sampler;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(sampler));
  xiiGALTextureCreationDescription image;
  image.m_Type        = xiiGALResourceDimension::Texture2D;
  image.m_Size        = {256, 64};
  image.m_Format      = xiiGALResourceFormat::RGBA32Float;
  image.m_uiMipLevels = 1;
  image.m_BindFlags   = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  auto transmittance  = device->CreateTexture(image);
  image.m_Size        = {32, 32};
  auto multiScatter   = device->CreateTexture(image);
  auto ready          = device->CreateFence({.m_Type = xiiGALFenceType::General});
  if (!transmittance || !multiScatter || !ready)
    return xiiStatus("Could not allocate immutable atmosphere lookup images or readiness fence.");
  xiiSharedPtr<const xiiAtmosphereLookup> lookup = XII_NEW(xiiAlignedAllocatorWrapper::GetAllocator(), xiiAtmosphereLookup, xiiRenderPassUtils::MakeAtmosphereConstants(settings), transmittance, multiScatter, ready);
  xiiRenderGraph                          graph;
  graph.BeginSetup(frame);
  struct Constants
  {
    xiiAtmosphereConstants     m_Snapshot;
    xiiRenderGraphBufferHandle m_Buffer;
  };
  const auto constants = graph.AddPass<Constants>("Upload immutable atmosphere settings", xiiGALCommandQueueFlags::Compute, [&](Constants& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot = lookup->m_Constants;
    data.m_Buffer = xiiRenderPassUtils::Constants(builder, "Atmosphere settings", sizeof(xiiAtmosphereConstants)); }, [](const Constants& data, xiiRenderGraphPassContext& execute) {
    xiiGALMapHelper<xiiAtmosphereConstants> mapped(execute.GetCommandList(), execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mapped = data.m_Snapshot; });
  struct Integrate
  {
    xiiSharedPtr<const xiiAtmosphereLookup>  m_Lookup;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Sampler;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiRenderGraphTextureHandle              m_Input, m_Output;
  };
  const auto integrated = graph.AddPass<Integrate>("Initialize atmosphere transmittance", xiiGALCommandQueueFlags::Compute, [&](Integrate& data, xiiRenderGraphBuilder& builder) {
    data.m_Lookup = lookup;
    data.m_Pipeline = transmittancePipeline;
    data.m_Constants = builder.ReadBuffer(constants.first->m_Buffer, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Output = builder.WriteTexture(builder.ImportTexture("Atmosphere transmittance", transmittance, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess); }, [](const Integrate& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_TransmittanceLUT", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind atmosphere transmittance initialization."); return; }
    commands.DispatchCompute({32, 8, 1}); });
  graph.AddPass<Integrate>("Initialize atmosphere multiple scattering", xiiGALCommandQueueFlags::Compute, [&](Integrate& data, xiiRenderGraphBuilder& builder) {
    data.m_Lookup = lookup;
    data.m_Pipeline = multiScatterPipeline;
    data.m_Sampler = sampler;
    data.m_Constants = builder.ReadBuffer(constants.first->m_Buffer, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Input = builder.ReadTexture(integrated.first->m_Output, xiiGALResourceStateFlags::ShaderResource);
    data.m_Output = builder.WriteTexture(builder.ImportTexture("Atmosphere multiple scattering", multiScatter, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
    builder.ExportTexture(data.m_Input, xiiGALResourceStateFlags::ShaderResource);
    builder.ExportTexture(data.m_Output, xiiGALResourceStateFlags::ShaderResource); }, [](const Integrate& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetSampler("LinearClampSampler", data.m_Sampler.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("g_TransmittanceLUT", execute.GetTexture(data.m_Input)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_MultiScatterLUT", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind atmosphere multiple-scattering initialization."); return; }
    commands.DispatchCompute({4, 4, 1});
    commands.EnqueueSignal(data.m_Lookup->m_Ready.Borrow(), 1); });
  graph.EndSetup();
  xiiRenderGraphBlackboard blackboard;
  xiiStringBuilder         error;
  if (graph.Compile({}, &error).Failed() || graph.Execute(device.Borrow(), nullptr, &blackboard, cache, nullptr, &error).Failed())
    return xiiStatus(error.GetView());
  out_lookup = std::move(lookup);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_AtmosphereLookup);
