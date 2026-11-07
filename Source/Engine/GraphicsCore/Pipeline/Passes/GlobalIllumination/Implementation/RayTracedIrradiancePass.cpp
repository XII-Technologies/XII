/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/RayTracedIrradiancePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/RayTracedIrradianceConstants.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>

xiiRayTracedIrradiancePass::xiiRayTracedIrradiancePass() { m_Irradiance.SetResolution({xiiRenderPipelineTextureResolutionMode::Input}); }
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayTracedIrradiancePass, 1, xiiRTTIDefaultAllocator<xiiRayTracedIrradiancePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Geometry", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance", m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(8U), new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("MaximumHits", m_uiMaximumHits)->AddAttributes(new xiiDefaultValueAttribute(2U), new xiiClampValueAttribute(1U, 4U)),
    XII_MEMBER_PROPERTY("SampleOffset", m_uiSampleOffset),
    XII_MEMBER_PROPERTY("RayLength", m_fRayLength)->AddAttributes(new xiiDefaultValueAttribute(20.0f)),
    XII_MEMBER_PROPERTY("SurfaceBias", m_fSurfaceBias)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
    XII_MEMBER_PROPERTY("AllowUnsupportedHardware", m_bAllowUnsupportedHardware)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiRayTracedIrradiancePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiSampleCount > 0 && m_uiSampleCount <= 32 && m_uiMaximumHits > 0 && m_uiMaximumHits <= 4 && xiiMath::IsFinite(m_fRayLength) && m_fRayLength > 0.001f && m_fRayLength <= 100000 && xiiMath::IsFinite(m_fSurfaceBias) && m_fSurfaceBias >= 0 && m_fSurfaceBias <= 10 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiRayTracedIrradiancePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto depth = context.GetInputTexture(m_Depth);
  if (!depth.IsValid())
    return XII_SUCCESS;
  const auto  device = xiiGALDevice::GetDefaultDevice();
  const auto& limits = device->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  if (device->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled || !limits.m_CapabilityFlags.IsSet(xiiGALRayTracingCapabilityFlags::StandaloneShaders) || !xiiGALBindlessResourceTable::IsInitialized())
    return m_bAllowUnsupportedHardware ? XII_SUCCESS : context.ReportFailure("Ray irradiance requires hardware ray pipelines and bindless material resources.");
  const auto* input  = xiiScreenSpacePassUtils::Image(context, depth, 1);
  const auto  normal = context.GetInputTexture(m_Normal);
  const auto  camera = context.GetInputBuffer(m_Camera);
  if (!input || !xiiScreenSpacePassUtils::Image(context, normal, 2, &input->m_Size) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Ray irradiance requires matching depth/normal images and exact camera constants.");
  const auto size = input->m_Size;
  if (xiiUInt64(size.width) * size.height > limits.m_uiMaxRayGenThreads)
    return context.ReportFailure("Ray irradiance exceeds ray generation dispatch limits.");
  xiiSizeU32 outputSize;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Irradiance, depth, outputSize));
  if (outputSize != size)
    return context.ReportFailure("Ray irradiance requires input-sized output.");
  const auto                 scene = context.GetInputAccelerationStructure(m_Scene);
  xiiRenderPassResourceScope scope(context);
  if (scene.IsEmpty())
  {
    const auto setup = [&](auto& image, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription desc;
      desc.m_Type      = xiiGALResourceDimension::Texture2D;
      desc.m_Size      = size;
      desc.m_Format    = xiiGALResourceFormat::RGBA32Float;
      desc.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      image            = builder.WriteTexture("Empty scene irradiance", desc, xiiGALResourceStateFlags::RenderTarget);
    };
    const auto record = [](const auto& image, xiiRenderGraphPassContext& execute) {
      xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(image)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    };
    const auto cleared = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, setup, record);
    return context.SetOutput(m_Irradiance, *cleared.first);
  }
  const auto  materials = context.GetInputBuffer(m_Materials), geometry = context.GetInputBuffer(m_Geometry);
  const auto* m = context.m_Graph.GetBufferDescription(materials);
  const auto* g = context.m_Graph.GetBufferDescription(geometry);
  if (!scene.IsValid() || !context.m_Graph.GetBufferDescription(scene.m_Dependency) || !m || !g || m->m_Mode != xiiGALBufferMode::Structured || g->m_Mode != xiiGALBufferMode::Structured || m->m_uiElementByteStride != sizeof(xiiRayTracingMaterialData) || g->m_uiElementByteStride != sizeof(xiiRayTracingGeometryData) || m->m_uiSize / sizeof(xiiRayTracingMaterialData) != g->m_uiSize / sizeof(xiiRayTracingGeometryData) || m->m_uiSize == 0 || m->m_uiSize % sizeof(xiiRayTracingMaterialData) != 0 || g->m_uiSize % sizeof(xiiRayTracingGeometryData) != 0 || !m->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !g->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Ray irradiance requires a current TLAS dependency and matching canonical hit records.");
  if (!limits.m_uiShaderGroupHandleSize || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupBaseAlignment) || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupHandleAlignment) || limits.m_uiMaxRecursionDepth < m_uiMaximumHits)
    return context.ReportFailure("Ray irradiance exceeds shader record or recursion capabilities.");
  const auto stride = xiiMemoryUtils::AlignSize(xiiUInt64(limits.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(limits.m_uiShaderGroupBaseAlignment, limits.m_uiShaderGroupHandleAlignment)));
  if (stride > limits.m_uiMaxShaderRecordStride || stride > xiiMath::MaxValue<xiiUInt32>() / 3U)
    return context.ReportFailure("Ray irradiance shader records exceed device limits.");
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RayTracedIrradiance.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Ray irradiance shader is unavailable.");
  xiiGALRayTracingPipelineStateCreationDescription description;
  description.m_pPipelineResourceSignature               = shader->GetPipelineResourceSignature();
  description.m_RayTracingPipeline.m_uiMaxRecursionDepth = m_uiMaximumHits;
  description.m_uiMaximumPayloadSize                     = 32;
  description.m_uiMaximumAttributeSize                   = 8;
  auto& generation                                       = description.m_GeneralShaders.ExpandAndGetRef();
  generation.m_sName.Assign("IrradianceGeneration");
  generation.m_pShader = shader->GetGALShader(xiiGALShaderType::RayGeneration);
  auto& miss           = description.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("IrradianceMiss");
  miss.m_pShader = shader->GetGALShader(xiiGALShaderType::RayMiss);
  auto& hit      = description.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("IrradianceHit");
  hit.m_pClosestHitShader = shader->GetGALShader(xiiGALShaderType::RayClosestHit);
  hit.m_pAnyHitShader     = shader->GetGALShader(xiiGALShaderType::RayAnyHit);
  const auto pipeline     = xiiGALPipelineCache::GetPipeline(description);
  if (!pipeline)
    return context.ReportFailure("Could not create ray irradiance pipeline.");
  if (!m_Sampler)
  {
    xiiGALSamplerCreationDescription sampler;
    sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MipFilter       = xiiGALFilterType::Linear;
    sampler.m_AddressU        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressV        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressW        = xiiGALTextureAddressMode::Wrap;
    sampler.m_uiMaxAnisotropy = 8;
    m_Sampler                 = device->CreateSampler(sampler);
    if (!m_Sampler)
      return context.ReportFailure("Could not allocate ray material fallback sampler.");
  }
  struct Data
  {
    xiiRenderGraphTextureHandle                 m_Depth, m_Normal, m_Output;
    xiiRenderGraphBufferHandle                  m_Dependency, m_Camera, m_Constants, m_Materials, m_Geometry, m_Records;
    xiiRayTracedIrradianceConstants             m_Settings;
    xiiSharedPtr<xiiGALTopLevelAS>              m_Scene;
    xiiSharedPtr<xiiGALRayTracingPipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>                 m_Sampler;
    xiiSizeU32                                  m_Size;
    xiiUInt64                                   m_Stride;
  };
  const auto setup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth                 = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal                = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera                = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Dependency            = builder.ReadBuffer(scene.m_Dependency, xiiGALResourceStateFlags::BuildASRead);
    data.m_Materials             = builder.ReadBuffer(materials, xiiGALResourceStateFlags::ShaderResource);
    data.m_Geometry              = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
    data.m_Scene                 = scene.m_pStructure;
    data.m_Output                = xiiCompositionPassUtils::Output(builder, size, xiiGALResourceFormat::RGBA32Float);
    data.m_Constants             = xiiRenderPassUtils::Constants(builder, "Ray irradiance settings", sizeof(data.m_Settings));
    data.m_Settings.RayDistances = xiiVec4(m_fRayLength, m_fSurfaceBias, 0, 0);
    data.m_Settings.PathSamples  = xiiVec4U32(m_uiSampleCount, m_uiMaximumHits, m_uiSampleOffset, 0);
    data.m_Size                  = size;
    data.m_Stride                = stride;
    data.m_Pipeline              = pipeline;
    data.m_Sampler               = m_Sampler;
    xiiGALBufferCreationDescription records;
    records.m_uiSize    = 3 * stride;
    records.m_Mode      = xiiGALBufferMode::Raw;
    records.m_Usage     = xiiGALResourceUsage::Mutable;
    records.m_BindFlags = xiiGALBindFlags::RayTracing;
    data.m_Records      = builder.WriteBuffer("Irradiance shader records", records, xiiGALResourceStateFlags::RayTracing);
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiRayTracedIrradianceConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Settings;
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetAccelerationStructure("g_Scene", data.m_Scene.Borrow(), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetConstantBuffer("xiiRayTracedIrradianceConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetShaderResourceTextureView("g_Normal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Irradiance", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
    const auto stages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stages);
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stages);
    commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
    xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_RayTracingBuffers", stages);
    xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_RayTracingTextures", stages);
    xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", stages);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind ray irradiance resources.");
      return;
    }
    xiiGALUpdateSBTDescription records;
    records.m_pPipelineState      = data.m_Pipeline.Borrow();
    records.m_pShaderBindingTable = execute.GetBuffer(data.m_Records);
    records.m_RayGenerationTable  = {0, data.m_Stride, data.m_Stride};
    records.m_MissTable           = {data.m_Stride, data.m_Stride, data.m_Stride};
    records.m_HitTable            = {2 * data.m_Stride, data.m_Stride, data.m_Stride};
    commands.UpdateSBT(records);
    xiiGALTraceRaysDescription trace(records.m_pShaderBindingTable, data.m_Size.width, data.m_Size.height);
    trace.m_RayGenerationTable = records.m_RayGenerationTable;
    trace.m_MissTable          = records.m_MissTable;
    trace.m_HitTable           = records.m_HitTable;
    commands.TraceRays(trace);
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, setup, record);
  return context.SetOutput(m_Irradiance, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_Implementation_RayTracedIrradiancePass);
