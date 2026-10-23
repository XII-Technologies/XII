/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/GatherConstants.h>
#include <Shaders/Pipeline/Passes/SparseVoxelRadiance/SparseVoxelRadianceConstants.h>

struct xiiSparseVoxelGatherPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point;
};
xiiSparseVoxelGatherPass::xiiSparseVoxelGatherPass()
{
  m_Output.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiSparseVoxelGatherPass::~xiiSparseVoxelGatherPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelGatherPass, 1, xiiRTTIDefaultAllocator<xiiSparseVoxelGatherPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SparseVoxelRadiancePool", m_RadiancePool)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SparseVoxelPageTable", m_PageTable)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SparseVoxelLevelData", m_LevelData)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SparseVoxelConstants", m_VoxelConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SparseVoxelIrradiance", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1000.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSparseVoxelGatherPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fIntensity) && m_fIntensity >= 0 && m_fIntensity <= 1000 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiSparseVoxelGatherPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal);
  const auto camera = context.GetInputBuffer(m_Camera);
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Output, depth, size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(depth);
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  const auto* normalDescription = context.m_Graph.GetTextureDescription(normal);
  const auto* cameraDescription = context.m_Graph.GetBufferDescription(camera);
  if (!normalDescription || normalDescription->m_Size != size || !cameraDescription || cameraDescription->m_uiSize < sizeof(xiiFrameCameraConstants))
    return context.ReportFailure("GI gather requires matching depth/normal extents and valid camera constants.");
  const auto* pages     = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_PageTable));
  const auto* levels    = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_LevelData));
  const auto* pool      = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_RadiancePool));
  const auto* constants = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_VoxelConstants));
  if (!pages || pages->m_uiElementByteStride != sizeof(xiiUInt32) || pages->m_uiSize < sizeof(xiiUInt32) || !levels || levels->m_uiElementByteStride != sizeof(xiiGpuSparseVoxelLevel) || levels->m_uiSize < sizeof(xiiGpuSparseVoxelLevel) || !pool || pool->m_uiElementByteStride != sizeof(xiiVec4) || pool->m_uiSize < sizeof(xiiVec4) || !constants || constants->m_uiSize < sizeof(xiiSparseVoxelRadianceConstants))
    return context.ReportFailure("Sparse gather requires compatible page, level, radiance and constant buffer layouts.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SparseVoxelGather.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(m_pState->m_Point));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Normal, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Constants;
    xiiRenderGraphBufferHandle               m_RadiancePool;
    xiiRenderGraphBufferHandle               m_PageTable;
    xiiRenderGraphBufferHandle               m_LevelData;
    xiiRenderGraphBufferHandle               m_VoxelConstants;
    xiiSizeU32                               m_Size;
    float                                    m_fIntensity;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Point;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_fIntensity = m_fIntensity; data.m_Pipeline=m_pState->m_Pipeline; data.m_Point=m_pState->m_Point;
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "GI Gather Parameters", sizeof(xiiGIGatherConstants));
    data.m_RadiancePool = builder.ReadBuffer(context.GetInputBuffer(m_RadiancePool), xiiGALResourceStateFlags::ShaderResource);
    data.m_PageTable = builder.ReadBuffer(context.GetInputBuffer(m_PageTable), xiiGALResourceStateFlags::ShaderResource);
    data.m_LevelData = builder.ReadBuffer(context.GetInputBuffer(m_LevelData), xiiGALResourceStateFlags::ShaderResource);
    data.m_VoxelConstants = builder.ReadBuffer(context.GetInputBuffer(m_VoxelConstants), xiiGALResourceStateFlags::ConstantBuffer);
    xiiGALTextureCreationDescription description;
    description.m_Type = xiiGALResourceDimension::Texture2D;
    description.m_Size = size;
    description.m_Format = xiiGALResourceFormat::RGBA16Float;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    data.m_Output = builder.WriteTexture("Irradiance", description, xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiGIGatherConstants> constants(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->GatherIntensityAndPadding = xiiVec4(data.m_fIntensity, 0, 0, 0);
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetSampler("PointClampSampler", data.m_Point.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiGIGatherConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_GBufNormal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SparseVoxelRadiancePool", execute.GetBuffer(data.m_RadiancePool)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SparseVoxelPageTable", execute.GetBuffer(data.m_PageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SparseVoxelLevels", execute.GetBuffer(data.m_LevelData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiSparseVoxelRadianceConstants", execute.GetBuffer(data.m_VoxelConstants), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_SparseVoxelIrradianceOut", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U}); });
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_SparseVoxelGatherPass);
