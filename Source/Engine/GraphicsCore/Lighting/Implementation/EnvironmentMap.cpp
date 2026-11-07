/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/EnvironmentMap.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <utility>

xiiEnvironmentMap::xiiEnvironmentMap(xiiSharedPtr<xiiGALTexture> source, xiiSharedPtr<xiiGALTexture> specular, xiiSharedPtr<xiiGALTexture> irradiance, xiiSharedPtr<xiiGALFence> ready, xiiUInt64 memory) :
  m_Source(std::move(source)), m_Specular(std::move(specular)), m_Irradiance(std::move(irradiance)), m_Ready(std::move(ready)), m_uiMemoryGPU(memory)
{
}

xiiStatus xiiEnvironmentMap::Create(const xiiSharedPtr<xiiGALTexture>& source, xiiSharedPtr<const xiiEnvironmentMap>& out_map, xiiSharedPtr<xiiGALFence> sourceReady)
{
  out_map.Clear();
  if (!source || !xiiGALDevice::HasDefaultDevice())
    return xiiStatus("Environment convolution requires a resident cubemap and a device.");
  auto       device     = xiiGALDevice::GetDefaultDevice();
  if (source->GetDevice() != device || (sourceReady && (sourceReady->GetDevice() != device || sourceReady->GetDescription().m_Type != xiiGALFenceType::General)))
    return xiiStatus("Environment convolution requires a source and optional general readiness fence from the current device.");
  auto*      cache      = xiiRenderGraphManager::GetResourceCache();
  const auto frameIndex = xiiRenderGraphManager::GetActiveFrameIndex();
  if (!cache || !frameIndex)
    return xiiStatus("Environment initialization requires an active shared GPU frame.");
  const auto& input  = source->GetDescription();
  const auto& format = xiiGALTextureUtilities::GetResourceFormatProperties(input.m_Format);
  if (input.m_Type != xiiGALResourceDimension::TextureCube || input.m_uiArraySizeOrDepth != 6 || !input.m_Size.HasNonZeroArea() || input.m_Size.width != input.m_Size.height || input.m_uiSampleCount != 1 || !input.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALTextureUtilities::GetComponentCount(input.m_Format) < 3 || format.IsTypeless() || (format.m_ComponentType != xiiGALResourceFormatComponentType::Float && !format.IsNormalized() && format.m_ComponentType != xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB))
    return xiiStatus("Environment convolution requires a readable RGB single cubemap.");
  xiiSharedPtr<xiiGALComputePipelineState> specularPipeline, diffusePipeline;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(specularPipeline, "Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(diffusePipeline, "Shaders/Pipeline/ReflectionIrradiance.xiiShader"));
  xiiSharedPtr<xiiGALSampler> sampler;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(sampler));
  xiiGALTextureCreationDescription description;
  description.m_Type = xiiGALResourceDimension::TextureCube;
  // Keep separate mirror and rough endpoints even for a one-texel source face.
  description.m_Size               = {xiiMath::Max(input.m_Size.width, 2U), xiiMath::Max(input.m_Size.height, 2U)};
  description.m_uiArraySizeOrDepth = 6;
  description.m_uiMipLevels        = xiiMath::Log2i(description.m_Size.width) + 1;
  description.m_Format             = xiiGALResourceFormat::RGBA32Float;
  description.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  auto specular                    = device->CreateTexture(description);
  description.m_Size               = {16, 16};
  description.m_uiMipLevels        = 1;
  auto irradiance                  = device->CreateTexture(description);
  auto ready                       = device->CreateFence({.m_Type = xiiGALFenceType::General});
  if (!specular || !irradiance || !ready)
    return xiiStatus("Could not allocate immutable environment lighting images or readiness fence.");
  const auto                            memory = specular->GetMemoryConsumption() + irradiance->GetMemoryConsumption();
  xiiSharedPtr<const xiiEnvironmentMap> map    = XII_DEFAULT_NEW(xiiEnvironmentMap, source, specular, irradiance, ready, memory);
  xiiRenderGraph                        graph;
  graph.BeginSetup(frameIndex);
  struct Data
  {
    xiiSharedPtr<const xiiEnvironmentMap>    m_Map;
    xiiSharedPtr<xiiGALComputePipelineState> m_SpecularPipeline, m_DiffusePipeline;
    xiiSharedPtr<xiiGALSampler>              m_Sampler;
    xiiRenderGraphTextureHandle              m_Source, m_Specular, m_Irradiance;
    xiiRenderGraphBufferHandle               m_Constants;
  };
  graph.AddPass<Data>("Initialize environment lighting", xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    if (sourceReady) builder.WaitForFence(sourceReady, 1).AssertSuccess();
    data.m_Map = map; data.m_SpecularPipeline = specularPipeline; data.m_DiffusePipeline = diffusePipeline; data.m_Sampler = sampler;
    data.m_Source = builder.ReadTexture(builder.ImportTexture("Environment source", source, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    data.m_Specular = builder.WriteTexture(builder.ImportTexture("GGX environment", specular, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Irradiance = builder.WriteTexture(builder.ImportTexture("Lambert environment", irradiance, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Environment convolution constants", sizeof(xiiReflectionFilteredSpecularConstants));
    builder.ExportTexture(data.m_Specular, xiiGALResourceStateFlags::ShaderResource);
    builder.ExportTexture(data.m_Irradiance, xiiGALResourceStateFlags::ShaderResource); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    const auto sourceView = execute.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    for (xiiUInt32 mode = 0; mode < 2; ++mode)
    {
      auto* output = execute.GetTexture(mode ? data.m_Irradiance : data.m_Specular);
      const auto& image = output->GetDescription();
      commands.SetPipelineState(mode ? data.m_DiffusePipeline.Borrow() : data.m_SpecularPipeline.Borrow());
      commands.ResolveAndSetShaderResourceTextureView("InputCubemap", sourceView.Borrow(), xiiGALShaderType::Compute);
      commands.ResolveAndSetSampler("LinearClampSampler", data.m_Sampler.Borrow());
      commands.ResolveAndSetConstantBuffer("xiiReflectionFilteredSpecularConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
      for (xiiUInt32 mip = 0; mip < image.m_uiMipLevels; ++mip)
      {
        const auto width = xiiMath::Max(image.m_Size.width >> mip, 1U);
        {
          xiiGALMapHelper<xiiReflectionFilteredSpecularConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
          mapped->MipLevel = mip; mapped->MipCount = image.m_uiMipLevels; mapped->SampleCount = 1024; mapped->FaceExtent = width;
        }
        xiiGALStateTransitionDescription transition;
        transition.m_pResource = execute.GetBuffer(data.m_Constants); transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
        xiiGALTextureViewCreationDescription view;
        view.m_ViewType = xiiGALTextureViewType::UnorderedAccess; view.m_ResourceDimension = xiiGALResourceDimension::Texture2DArray;
        view.m_uiMostDetailedMip = mip; view.m_uiMipLevelCount = 1;
        auto target = output->CreateView(view);
        if (!target) { execute.ReportFailure("Could not create environment convolution mip view."); return; }
        commands.ResolveAndSetUnorderedAccessTextureView("ReflectionOutput", target.Borrow(), xiiGALShaderType::Compute);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind environment convolution resources."); return; }
        commands.DispatchCompute({(width + 7U) / 8U, (width + 7U) / 8U, 6});
      }
    }
    commands.EnqueueSignal(data.m_Map->m_Ready.Borrow(), 1); });
  graph.EndSetup();
  xiiStringBuilder         error;
  xiiRenderGraphBlackboard blackboard;
  if (graph.Compile({}, &error).Failed() || graph.Execute(device.Borrow(), nullptr, &blackboard, cache, nullptr, &error).Failed())
    return xiiStatus(error.GetView());
  out_map = std::move(map);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_EnvironmentMap);
