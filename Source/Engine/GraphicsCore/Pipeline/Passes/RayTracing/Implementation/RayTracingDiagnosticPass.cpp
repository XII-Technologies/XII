/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingDiagnosticPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/ViewRenderModeConstants.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayTracingDiagnosticPass, 1, xiiRTTIDefaultAllocator<xiiRayTracingDiagnosticPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Geometry", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DiagnosticColor", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiRayTracingDiagnosticPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !xiiViewRenderMode::IsRayTracingVisualizationMode(context.m_pView->GetViewRenderMode()))
    return XII_SUCCESS;
  if (!xiiGALDevice::HasDefaultDevice() || xiiGALDevice::GetDefaultDevice()->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("This visualization requires hardware ray tracing.");
  const auto* viewCamera = context.m_pView->GetCamera();
  if (!viewCamera || viewCamera->IsStereoscopic() || !xiiMath::IsFinite(viewCamera->GetNearPlane()) || !xiiMath::IsFinite(viewCamera->GetFarPlane()) || viewCamera->GetFarPlane() <= viewCamera->GetNearPlane())
    return context.ReportFailure("Ray visualization requires a finite mono perspective or orthographic camera range.");
  const auto mode = context.m_pView->GetViewRenderMode().GetValue();
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Output, {}, size));
  xiiGALTextureCreationDescription output;
  output.m_Type                    = xiiGALResourceDimension::Texture2D;
  output.m_Size                    = size;
  output.m_Format                  = xiiGALResourceFormat::RGBA16Float;
  output.m_BindFlags               = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::RenderTarget;
  const auto                 scene = context.GetInputAccelerationStructure(m_Scene);
  xiiRenderPassResourceScope scope(context);
  if (scene.IsEmpty())
  {
    // No TraceRay or miss-shader invocation occurs without a TLAS.
    const auto cleared = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& texture, xiiRenderGraphBuilder& builder) { texture = builder.WriteTexture("Empty ray workload", output, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& texture, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); });
    return context.SetOutput(m_Output, *cleared.first);
  }
  if (!scene.IsValid() || !context.m_Graph.GetBufferDescription(scene.m_Dependency))
    return context.ReportFailure("Ray visualization requires a current TLAS build dependency.");
  const auto materials = context.GetInputBuffer(m_Materials), geometry = context.GetInputBuffer(m_Geometry), camera = context.GetInputBuffer(m_Camera);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0;
  };
  if (!valid(materials, sizeof(xiiRayTracingMaterialData)) || !valid(geometry, sizeof(xiiRayTracingGeometryData)) ||
      context.m_Graph.GetBufferDescription(materials)->m_uiSize / sizeof(xiiRayTracingMaterialData) != context.m_Graph.GetBufferDescription(geometry)->m_uiSize / sizeof(xiiRayTracingGeometryData) ||
      !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Ray visualization requires matching canonical hit records and exact camera constants.");
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  if (!size.HasNonZeroArea() || xiiUInt64(size.width) * size.height > limits.m_uiMaxRayGenThreads)
    return context.ReportFailure("Ray visualization exceeds ray-generation dispatch limits.");
  if (!limits.m_uiShaderGroupHandleSize || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupBaseAlignment) || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupHandleAlignment) || !limits.m_uiMaxRecursionDepth)
    return context.ReportFailure("Invalid ray-tracing shader-record capabilities.");
  const xiiUInt64 stride = xiiMemoryUtils::AlignSize(xiiUInt64(limits.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(limits.m_uiShaderGroupBaseAlignment, limits.m_uiShaderGroupHandleAlignment)));
  if (!stride || stride > limits.m_uiMaxShaderRecordStride || stride > xiiMath::MaxValue<xiiUInt32>() / 3)
    return context.ReportFailure("Ray visualization shader records exceed device limits.");
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/RayTracing/ViewDiagnostics.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Ray visualization shader is unavailable.");
  if (!m_Sampler)
  {
    xiiGALSamplerCreationDescription sampler;
    sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MipFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_AddressU        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressV        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressW        = xiiGALTextureAddressMode::Wrap;
    sampler.m_uiMaxAnisotropy = 8;
    m_Sampler                 = xiiGALDevice::GetDefaultDevice()->CreateSampler(sampler);
  }
  if (!m_Sampler)
    return context.ReportFailure("Could not create the ray material fallback sampler.");
  xiiGALRayTracingPipelineStateCreationDescription description;
  description.m_pPipelineResourceSignature               = shader->GetPipelineResourceSignature();
  description.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1;
  description.m_uiMaximumPayloadSize                     = 48;
  description.m_uiMaximumAttributeSize                   = 2 * sizeof(float);
  auto& generation                                       = description.m_GeneralShaders.ExpandAndGetRef();
  generation.m_sName.Assign("ViewDiagnosticGeneration");
  generation.m_pShader = shader->GetGALShader(xiiGALShaderType::RayGeneration);
  auto& miss           = description.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("ViewDiagnosticMiss");
  miss.m_pShader = shader->GetGALShader(xiiGALShaderType::RayMiss);
  auto& hit      = description.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("ViewDiagnosticTriangle");
  hit.m_pClosestHitShader = shader->GetGALShader(xiiGALShaderType::RayClosestHit);
  hit.m_pAnyHitShader     = shader->GetGALShader(xiiGALShaderType::RayAnyHit);
  const auto pipeline     = xiiGALPipelineCache::GetPipeline(description);
  if (!pipeline)
    return context.ReportFailure("Could not create the ray visualization pipeline.");
  struct Data
  {
    xiiRenderGraphBufferHandle                  m_Dependency, m_Materials, m_Geometry, m_Camera, m_Constants, m_Records;
    xiiRenderGraphTextureHandle                 m_Output;
    xiiSharedPtr<xiiGALTopLevelAS>              m_Scene;
    xiiSharedPtr<xiiGALRayTracingPipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>                 m_Sampler;
    xiiViewRenderModeConstants                  m_Snapshot;
    xiiUInt64                                   m_uiStride;
    xiiSizeU32                                  m_Size;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Dependency = builder.ReadBuffer(scene.m_Dependency, xiiGALResourceStateFlags::BuildASRead); data.m_Scene = scene.m_pStructure;
    data.m_Materials = builder.ReadBuffer(materials, xiiGALResourceStateFlags::ShaderResource);
    data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Ray visualization mode", sizeof(data.m_Snapshot));
    data.m_Snapshot = {}; data.m_Snapshot.RenderMode = mode;
    data.m_Output = builder.WriteTexture("Ray diagnostics", output, xiiGALResourceStateFlags::UnorderedAccess);
    xiiGALBufferCreationDescription records; records.m_uiSize = 3 * stride; records.m_Mode = xiiGALBufferMode::Raw; records.m_Usage = xiiGALResourceUsage::Mutable; records.m_BindFlags = xiiGALBindFlags::RayTracing;
    data.m_Records = builder.WriteBuffer("Ray diagnostic shader records", records, xiiGALResourceStateFlags::RayTracing);
    data.m_Pipeline = pipeline; data.m_uiStride = stride; data.m_Size = size; data.m_Sampler = m_Sampler; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiViewRenderModeConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Snapshot; }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetConstantBuffer("xiiViewRenderModeConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetAccelerationStructure("g_Scene", data.m_Scene.Borrow(), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Output", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit);
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit);
    xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_RayTracingBuffers", xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit);
    xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
    xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", xiiGALShaderType::RayAnyHit);
    commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind ray visualization resources."); return; }
    xiiGALUpdateSBTDescription records; records.m_pPipelineState = data.m_Pipeline.Borrow(); records.m_pShaderBindingTable = execute.GetBuffer(data.m_Records);
    records.m_RayGenerationTable = {0, data.m_uiStride, data.m_uiStride}; records.m_MissTable = {data.m_uiStride, data.m_uiStride, data.m_uiStride}; records.m_HitTable = {2 * data.m_uiStride, data.m_uiStride, data.m_uiStride};
    commands.UpdateSBT(records);
    xiiGALTraceRaysDescription trace(records.m_pShaderBindingTable, data.m_Size.width, data.m_Size.height, 1); trace.m_RayGenerationTable = records.m_RayGenerationTable; trace.m_MissTable = records.m_MissTable; trace.m_HitTable = records.m_HitTable;
    commands.TraceRays(trace); });
  return context.SetOutput(m_Output, emitted.first->m_Output);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_RayTracing_Implementation_RayTracingDiagnosticPass);
