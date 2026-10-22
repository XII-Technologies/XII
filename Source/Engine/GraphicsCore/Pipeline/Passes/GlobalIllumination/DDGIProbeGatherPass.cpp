/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/DDGI/DDGIConstants.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/GatherConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>

struct xiiDDGIProbeGatherPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point;
};
xiiDDGIProbeGatherPass::xiiDDGIProbeGatherPass()  = default;
xiiDDGIProbeGatherPass::~xiiDDGIProbeGatherPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIProbeGatherPass, 1, xiiRTTIDefaultAllocator<xiiDDGIProbeGatherPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DDGIProbeIrradianceAtlas", m_IrradianceAtlas)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DDGIProbeDistanceAtlas", m_DistanceAtlas)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DDGIProbeStates", m_ProbeStates)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DDGIProbeConstants", m_ProbeConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("EnvironmentLight", m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DDGIIrradiance", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1000.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDDGIProbeGatherPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fIntensity) && m_fIntensity >= 0 && m_fIntensity <= 1000 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiDDGIProbeGatherPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal);
  const auto camera = context.GetInputBuffer(m_Camera);
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, depth, 0, 0, size));
  const auto* normalDescription = context.m_Graph.GetTextureDescription(normal);
  const auto* cameraDescription = context.m_Graph.GetBufferDescription(camera);
  if (!normalDescription || normalDescription->m_Size != size || !cameraDescription || cameraDescription->m_uiSize < sizeof(xiiFrameCameraConstants))
    return context.ReportFailure("GI gather requires matching depth/normal extents and valid camera constants.");
  const auto* states     = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_ProbeStates));
  const auto* constants  = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_ProbeConstants));
  const auto* light      = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_Environment));
  const auto* irradiance = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_IrradianceAtlas));
  const auto* distance   = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_DistanceAtlas));
  if (!states || states->m_uiElementByteStride != sizeof(xiiGpuDDGIProbeState) || states->m_uiSize < sizeof(xiiGpuDDGIProbeState) || !constants || constants->m_uiSize < sizeof(xiiDDGIProbeConstants) || !light || light->m_uiSize != sizeof(xiiEnvironmentLightConstants) || !irradiance || !distance || irradiance->m_Size != distance->m_Size)
    return context.ReportFailure("DDGI gather requires compatible probe records, constants and equal atlas extents.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/DDGIProbeGather.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(m_pState->m_Point));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Normal, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Constants;
    xiiRenderGraphTextureHandle              m_IrradianceAtlas;
    xiiRenderGraphTextureHandle              m_DistanceAtlas;
    xiiRenderGraphBufferHandle               m_ProbeStates;
    xiiRenderGraphBufferHandle               m_ProbeConstants;
    xiiRenderGraphBufferHandle               m_Environment;
    xiiSizeU32                               m_Size;
    float                                    m_fIntensity;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Point;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_fIntensity = m_fIntensity; data.m_Pipeline = m_pState->m_Pipeline; data.m_Point = m_pState->m_Point;
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "GI Gather Parameters", sizeof(xiiGIGatherConstants));
    data.m_IrradianceAtlas = builder.ReadTexture(context.GetInputTexture(m_IrradianceAtlas), xiiGALResourceStateFlags::ShaderResource);
    data.m_DistanceAtlas = builder.ReadTexture(context.GetInputTexture(m_DistanceAtlas), xiiGALResourceStateFlags::ShaderResource);
    data.m_ProbeStates = builder.ReadBuffer(context.GetInputBuffer(m_ProbeStates), xiiGALResourceStateFlags::ShaderResource);
    data.m_ProbeConstants = builder.ReadBuffer(context.GetInputBuffer(m_ProbeConstants), xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Environment = builder.ReadBuffer(context.GetInputBuffer(m_Environment), xiiGALResourceStateFlags::ConstantBuffer);
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
    commands.ResolveAndSetShaderResourceTextureView("g_DDGIIrradianceAtlas", execute.GetTexture(data.m_IrradianceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_DDGIDistanceAtlas", execute.GetTexture(data.m_DistanceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_DDGIProbeStates", execute.GetBuffer(data.m_ProbeStates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants", execute.GetBuffer(data.m_ProbeConstants), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants", execute.GetBuffer(data.m_Environment), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_DDGIOut", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U}); });
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_DDGIProbeGatherPass);
