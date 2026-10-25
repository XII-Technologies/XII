/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionConstants.h>

static_assert(sizeof(xiiAmbientOcclusionConstants) == 32);
struct xiiAmbientOcclusionPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState>    m_ScreenPipeline;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_RayPipeline;
  xiiSharedPtr<xiiGALSampler>                 m_Point, m_Anisotropic;
  xiiUInt64                                   m_uiRecordStride = 0;
  xiiResult                                   PrepareRays()
  {
    if (m_RayPipeline)
      return XII_SUCCESS;
    const auto device = xiiGALDevice::GetDefaultDevice();
    if (!device || device->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled || !xiiGALBindlessResourceTable::IsInitialized())
      return XII_FAILURE;
    const auto shader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RayTracedAmbientOcclusion.xiiShader");
    const auto handle = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
    if (!handle.IsValid())
      return XII_FAILURE;
    xiiResourceLock<xiiShaderPermutationResource> permutation(handle, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid() || !permutation->IsShaderValid())
      return XII_FAILURE;
    xiiGALRayTracingPipelineStateCreationDescription description;
    description.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
    description.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1;
    description.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
    description.m_uiMaximumAttributeSize                   = 2 * sizeof(float);
    auto& generation                                       = description.m_GeneralShaders.ExpandAndGetRef();
    generation.m_sName.Assign("RTAORayGeneration");
    generation.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
    auto& miss           = description.m_GeneralShaders.ExpandAndGetRef();
    miss.m_sName.Assign("RTAOMiss");
    miss.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayMiss);
    auto& hit      = description.m_TriangleHitShaders.ExpandAndGetRef();
    hit.m_sName.Assign("RTAOTriangleHit");
    hit.m_pClosestHitShader = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
    hit.m_pAnyHitShader     = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
    if (!description.m_GeneralShaders[0].m_pShader || !miss.m_pShader || !hit.m_pClosestHitShader || !hit.m_pAnyHitShader)
      return XII_FAILURE;
    const auto& limits = device->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
    m_uiRecordStride   = xiiMemoryUtils::AlignSize(xiiUInt64(limits.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(1U, limits.m_uiShaderGroupBaseAlignment)));
    if (!m_uiRecordStride || (limits.m_uiMaxShaderRecordStride && m_uiRecordStride > limits.m_uiMaxShaderRecordStride))
      return XII_FAILURE;
    m_RayPipeline = xiiGALPipelineCache::GetPipeline(description);
    if (!m_RayPipeline)
      return XII_FAILURE;
    return XII_SUCCESS;
  }
};
xiiAmbientOcclusionPass::xiiAmbientOcclusionPass()
{
  m_Output.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiAmbientOcclusionPass::~xiiAmbientOcclusionPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAmbientOcclusionPass, 1, xiiRTTIDefaultAllocator<xiiAmbientOcclusionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("NormalRoughness", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameGlobal", m_Global)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RayTracingScene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinEnabledAttribute("UseRayTracing"), new xiiRenderPipelinePinOptionalAttribute("AllowScreenSpace")),
    XII_MEMBER_PROPERTY("RayTracingMaterialData", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinEnabledAttribute("UseRayTracing"), new xiiRenderPipelinePinOptionalAttribute("AllowScreenSpace")),
    XII_MEMBER_PROPERTY("RayTracingGeometryData", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinEnabledAttribute("UseRayTracing"), new xiiRenderPipelinePinOptionalAttribute("AllowScreenSpace")),
    XII_MEMBER_PROPERTY("RawAO", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Constants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("UseRayTracing", m_bUseRayTracing)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("AllowScreenSpace", m_bAllowScreenSpace)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.01f, 10000.0f)),
    XII_MEMBER_PROPERTY("RayOriginBias", m_fRayOriginBias)->AddAttributes(new xiiDefaultValueAttribute(0.005f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("RayTMin", m_fRayTMin)->AddAttributes(new xiiDefaultValueAttribute(0.001f), new xiiClampValueAttribute(0.00001f, 100.0f)),
    XII_MEMBER_PROPERTY("NormalBias", m_fNormalBias)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("PixelStride", m_fPixelStride)->AddAttributes(new xiiDefaultValueAttribute(6.0f), new xiiClampValueAttribute(0.1f, 128.0f)),
    XII_MEMBER_PROPERTY("MinSampleDistance", m_fMinSampleDistance)->AddAttributes(new xiiDefaultValueAttribute(0.005f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("DirectionCount", m_uiDirectionCount)->AddAttributes(new xiiDefaultValueAttribute(8U), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("StepCount", m_uiStepCount)->AddAttributes(new xiiDefaultValueAttribute(4U), new xiiClampValueAttribute(1U, 32U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/AmbientOcclusion") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiAmbientOcclusionPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if ((!m_bUseRayTracing && !m_bAllowScreenSpace) || !xiiMath::IsFinite(m_fRadius) || m_fRadius < 0.01f || m_fRadius > 10000 || !xiiMath::IsFinite(m_fRayOriginBias) || m_fRayOriginBias < 0 || m_fRayOriginBias > 10 || !xiiMath::IsFinite(m_fRayTMin) || m_fRayTMin < 0.00001f || m_fRayTMin > 100 || m_fRayTMin >= m_fRadius || !xiiMath::IsFinite(m_fNormalBias) || m_fNormalBias < 0 || m_fNormalBias > 1 || !xiiMath::IsFinite(m_fPixelStride) || m_fPixelStride < 0.1f || m_fPixelStride > 128 || !xiiMath::IsFinite(m_fMinSampleDistance) || m_fMinSampleDistance < 0 || m_fMinSampleDistance > 10 || m_fMinSampleDistance >= m_fRadius || m_uiDirectionCount == 0 || m_uiDirectionCount > 64 || m_uiStepCount == 0 || m_uiStepCount > 32)
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiBitflags<xiiRenderPipelineDeviceFeature> xiiAmbientOcclusionPass::GetRequiredDeviceFeatures() const
{
  if (!m_bEnabled)
    return {};
  return m_bAllowScreenSpace ? xiiRenderPipelineDeviceFeature::Compute : xiiRenderPipelineDeviceFeature::RayTracing | xiiRenderPipelineDeviceFeature::BindlessResources;
}
xiiResult xiiAmbientOcclusionPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
    return XII_SUCCESS;
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (!device || device->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Ambient occlusion requires R8 storage-image support (TextureUAVExtendedFormats). Disable AO on unsupported devices.");
  const auto depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal);
  const auto camera = context.GetInputBuffer(m_Camera), global = context.GetInputBuffer(m_Global);
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Output, depth, size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(depth);
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  const auto* normalDescription = context.m_Graph.GetTextureDescription(normal);
  const auto* cameraDescription = context.m_Graph.GetBufferDescription(camera);
  const auto* globalDescription = context.m_Graph.GetBufferDescription(global);
  if (!xiiScreenSpacePassUtils::Image(context, depth, 1, &size) || !normalDescription || !xiiScreenSpacePassUtils::Image(context, normal, 2, &size) || !cameraDescription || cameraDescription->m_uiSize != sizeof(xiiFrameCameraConstants) || !globalDescription || globalDescription->m_uiSize != sizeof(xiiFrameGlobalConstants))
    return context.ReportFailure("AO inputs have inconsistent extents or frame constant layouts.");
  const auto scene = m_bUseRayTracing ? context.GetInputAccelerationStructure(m_Scene) : xiiRenderPipelineAccelerationStructureBinding();
  const bool trace = scene.IsValid();
  if (!trace && !m_bAllowScreenSpace)
    return context.ReportFailure("AO requires an explicitly connected ray scene.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  if (trace)
  {
    if (!context.GetInputBuffer(m_Materials).IsValid() || !context.GetInputBuffer(m_Geometry).IsValid())
      return context.ReportFailure("Ray-traced AO requires material and geometry records.");
    XII_SUCCEED_OR_RETURN(m_pState->PrepareRays());
    if (!m_pState->m_Anisotropic)
    {
      xiiGALSamplerCreationDescription description;
      description.m_MinFilter       = xiiGALFilterType::Anisotropic;
      description.m_MagFilter       = xiiGALFilterType::Anisotropic;
      description.m_MipFilter       = xiiGALFilterType::Linear;
      description.m_AddressU        = xiiGALTextureAddressMode::Wrap;
      description.m_AddressV        = xiiGALTextureAddressMode::Wrap;
      description.m_AddressW        = xiiGALTextureAddressMode::Wrap;
      description.m_uiMaxAnisotropy = 8;
      m_pState->m_Anisotropic       = xiiGALDevice::GetDefaultDevice()->CreateSampler(description);
      if (!m_pState->m_Anisotropic)
        return XII_FAILURE;
    }
  }
  else
  {
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_ScreenPipeline, "Shaders/Pipeline/ScreenSpaceAmbientOcclusion.xiiShader"));
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(m_pState->m_Point));
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle                   m_Depth, m_Normal, m_Output;
    xiiRenderGraphBufferHandle                    m_Camera, m_Global, m_Constants, m_Materials, m_Geometry, m_Sbt;
    xiiRenderPipelineAccelerationStructureBinding m_Scene;
    xiiAmbientOcclusionConstants                  m_Parameters;
    xiiSizeU32                                    m_Size;
    xiiSharedPtr<xiiGALComputePipelineState>      m_ScreenPipeline;
    xiiSharedPtr<xiiGALRayTracingPipelineState>   m_RayPipeline;
    xiiSharedPtr<xiiGALSampler>                   m_Sampler;
    xiiUInt64                                     m_uiRecordStride = 0;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, trace ? xiiGALCommandQueueFlags::Graphics : xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_ScreenPipeline = m_pState->m_ScreenPipeline;
    data.m_RayPipeline = m_pState->m_RayPipeline;
    data.m_Sampler = trace ? m_pState->m_Anisotropic : m_pState->m_Point;
    data.m_uiRecordStride = m_pState->m_uiRecordStride;
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Global = builder.ReadBuffer(global, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "AmbientOcclusionConstants", sizeof(data.m_Parameters));
    data.m_Parameters.Radius = m_fRadius;
    data.m_Parameters.RayOriginBias = m_fRayOriginBias;
    data.m_Parameters.RayTMin = m_fRayTMin;
    data.m_Parameters.NormalBias = m_fNormalBias;
    data.m_Parameters.PixelStride = m_fPixelStride;
    data.m_Parameters.MinSampleDistance = m_fMinSampleDistance;
    data.m_Parameters.DirectionCount = m_uiDirectionCount;
    data.m_Parameters.StepCount = m_uiStepCount;
    if (trace)
    {
      data.m_Scene = scene;
      data.m_Scene.m_Dependency = builder.ReadBuffer(scene.m_Dependency, xiiGALResourceStateFlags::BuildASRead);
      data.m_Materials = builder.ReadBuffer(context.GetInputBuffer(m_Materials), xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(context.GetInputBuffer(m_Geometry), xiiGALResourceStateFlags::ShaderResource);
      xiiGALBufferCreationDescription sbt;
      sbt.m_uiSize = 3 * m_pState->m_uiRecordStride;
      sbt.m_Mode = xiiGALBufferMode::Raw;
      sbt.m_BindFlags = xiiGALBindFlags::RayTracing;
      sbt.m_Usage = xiiGALResourceUsage::Mutable;
      data.m_Sbt = builder.WriteBuffer("AO Shader Binding Table", sbt, xiiGALResourceStateFlags::RayTracing);
    }
    xiiGALTextureCreationDescription output;
    output.m_Type = xiiGALResourceDimension::Texture2D;
    output.m_Size = size;
    output.m_Format = xiiGALResourceFormat::R8UNormalized;
    output.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    data.m_Output = builder.WriteTexture("AO", output, xiiGALResourceStateFlags::UnorderedAccess); }, [trace](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiAmbientOcclusionConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Parameters;
    }
    // These uniforms are used by ray generation; requiring every ray stage
    // would exclude the reflected bindings optimized out of hit/miss shaders.
    const xiiBitflags<xiiGALShaderType> stages = trace ? xiiGALShaderType::RayGeneration : xiiGALShaderType::Compute;
    if (trace) commands.SetPipelineState(data.m_RayPipeline.Borrow()); else commands.SetPipelineState(data.m_ScreenPipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), stages);
    commands.ResolveAndSetConstantBuffer("PerFrameGlobal", execute.GetBuffer(data.m_Global), stages);
    commands.ResolveAndSetConstantBuffer("xiiAmbientOcclusionConstants", execute.GetBuffer(data.m_Constants), stages);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), trace ? xiiGALShaderType::RayGeneration : xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), trace ? xiiGALShaderType::RayGeneration : xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_AOOut", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), trace ? xiiGALShaderType::RayGeneration : xiiGALShaderType::Compute);
    if (trace)
    {
      commands.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_Scene.m_pStructure.Borrow(), xiiGALShaderType::RayGeneration);
      commands.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      commands.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
      xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_RayTracingBuffers", xiiGALShaderType::RayAnyHit);
      xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
      xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", xiiGALShaderType::RayAnyHit);
    }
    else commands.ResolveAndSetSampler("g_sPointClamp", data.m_Sampler.Borrow());
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    if (trace)
    {
      const auto stride = data.m_uiRecordStride;
      xiiGALUpdateSBTDescription update;
      update.m_pPipelineState = data.m_RayPipeline.Borrow();
      update.m_pShaderBindingTable = execute.GetBuffer(data.m_Sbt);
      update.m_RayGenerationTable = {0, stride, stride};
      update.m_MissTable = {stride, stride, stride};
      update.m_HitTable = {2 * stride, stride, stride};
      commands.UpdateSBT(update);
      xiiGALTraceRaysDescription rays(update.m_pShaderBindingTable, data.m_Size.width, data.m_Size.height);
      rays.m_RayGenerationTable = update.m_RayGenerationTable;
      rays.m_MissTable = update.m_MissTable;
      rays.m_HitTable = update.m_HitTable;
      commands.TraceRays(rays);
    }
    else commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Constants, emitted.first->m_Constants));
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_AmbientOcclusion_AmbientOcclusionPass);
