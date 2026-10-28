/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/ViewRenderModeConstants.h>

using namespace xiiRenderPassUtils;
struct xiiViewRenderModePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point;
  xiiSharedPtr<xiiGALComputePipelineState> m_pExposurePipeline;
};
xiiViewRenderModePass::xiiViewRenderModePass()
{
  m_OutputDisplayLinearColor.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiViewRenderModePass::~xiiViewRenderModePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewRenderModePass, 1, xiiRTTIDefaultAllocator<xiiViewRenderModePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DisplayLinearColorInput", m_InputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("DisplayLinearColorOutput", m_OutputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferAlbedo", m_InputGBufferAlbedo)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_InputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferMaterial", m_InputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferEmissive", m_InputGBufferEmissive)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferObjectId", m_InputGBufferObjectId)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("WireframeSurface", m_InputWireframeSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Overdraw", m_InputOverdraw)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_InputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("StableAO", m_InputStableAO)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("IndirectLighting", m_InputIndirectLighting)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("EnvironmentSpecular", m_InputEnvironmentSpecular)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BloomTexture", m_InputBloomTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_InputPerFrameCamera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("CurrentExposure", m_InputCurrentExposure)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewMode", m_bUseViewMode)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiViewRenderMode, m_Mode)->AddAttributes(new xiiDefaultValueAttribute(xiiViewRenderMode::Default)),
    XII_MEMBER_PROPERTY("MotionVectorScale", m_fMotionVectorScale)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiViewRenderModePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiViewRenderMode::IsSupportedVisualizationMode(m_Mode) && xiiMath::IsFinite(m_fMotionVectorScale) && m_fMotionVectorScale > 0.0f ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiViewRenderModePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto mode  = static_cast<xiiViewRenderMode::Enum>(m_bUseViewMode && context.m_pView ? context.m_pView->GetViewRenderMode().GetValue() : m_Mode.GetValue());
  const auto input = context.GetInputTexture(m_InputDisplayLinearColor);
  if (mode == xiiViewRenderMode::Default || mode == xiiViewRenderMode::ToneMappingCurve || mode == xiiViewRenderMode::ColorGradingLUT)
    return context.SetOutput(m_OutputDisplayLinearColor, input);
  if (!xiiViewRenderMode::IsSupportedVisualizationMode(mode))
    return XII_FAILURE;
  xiiRenderPassResourceScope scope(context);
  xiiSizeU32                 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, m_OutputDisplayLinearColor, input, size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(input);
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  // Every diagnostic writes all pixels. Its output does not depend on normal
  // grading/tone mapping, so the graph may cull those unused producers.
  const auto outputDescription = *sizingInput;
  if (mode == xiiViewRenderMode::Exposure)
  {
    const auto  exposure    = context.GetInputBuffer(m_InputCurrentExposure);
    const auto* description = context.m_Graph.GetBufferDescription(exposure);
    if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiSize != 2 * sizeof(float) || description->m_uiElementByteStride != sizeof(float))
      return context.ReportFailure("Exposure visualization requires the producer's structured exposure/luminance pair.");
    if (!m_State)
      m_State = XII_DEFAULT_NEW(State);
    XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_State->m_pExposurePipeline, "Shaders/Pipeline/ExposureVisualization.xiiShader"));
    struct Data
    {
      xiiRenderGraphBufferHandle               m_hExposure;
      xiiRenderGraphTextureHandle              m_hOutput;
      xiiSizeU32                               m_Size;
      xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
        data.m_hExposure = builder.ReadBuffer(exposure, xiiGALResourceStateFlags::ShaderResource);
        data.m_hOutput = builder.WriteTexture("Exposure visualization", outputDescription, xiiGALResourceStateFlags::UnorderedAccess);
        data.m_Size = size;
        data.m_pPipeline = m_State->m_pExposurePipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
        auto& commands = execute.GetCommandList();
        commands.SetPipelineState(data.m_pPipeline.Borrow());
        commands.ResolveAndSetShaderResourceBufferView("g_Exposure", execute.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        commands.ResolveAndSetUnorderedAccessTextureView("g_Output", execute.GetTexture(data.m_hOutput)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        {
          execute.ReportFailure("Could not bind exposure visualization resources.");
          return;
        }
        commands.DispatchCompute({(data.m_Size.width + 7) / 8, (data.m_Size.height + 7) / 8, 1}); });
    return context.SetOutput(m_OutputDisplayLinearColor, emitted.first->m_hOutput);
  }
  const xiiRenderPipelineTextureInputPin* source   = &m_InputSceneDepth;
  xiiUInt32                               channels = 1;
  switch (mode)
  {
    case xiiViewRenderMode::Overdraw:
      source = &m_InputOverdraw;
      break;
    case xiiViewRenderMode::BaseColor:
      source   = &m_InputGBufferAlbedo;
      channels = 3;
      break;
    case xiiViewRenderMode::NormalMap:
      source   = &m_InputGBufferNormal;
      channels = 2;
      break;
    case xiiViewRenderMode::Metallic:
    case xiiViewRenderMode::Roughness:
    case xiiViewRenderMode::Specular:
      source   = &m_InputGBufferMaterial;
      channels = 3;
      break;
    case xiiViewRenderMode::Emissive:
      source   = &m_InputGBufferEmissive;
      channels = 3;
      break;
    case xiiViewRenderMode::AmbientOcclusion:
      source = &m_InputStableAO;
      break;
    case xiiViewRenderMode::MotionVectors:
      source   = &m_InputDilatedVelocityBuffer;
      channels = 2;
      break;
    case xiiViewRenderMode::IndirectLighting:
      source   = &m_InputIndirectLighting;
      channels = 3;
      break;
    case xiiViewRenderMode::ReflectionContribution:
      source   = &m_InputEnvironmentSpecular;
      channels = 3;
      break;
    case xiiViewRenderMode::Bloom:
      source   = &m_InputBloomTexture;
      channels = 3;
      break;
    default:
      break;
  }
  const bool surface  = xiiViewRenderMode::IsSurfaceVisualizationMode(mode);
  auto       selected = context.GetInputTexture(*source);
  auto       depth    = context.GetInputTexture(m_InputSceneDepth);
  auto       objects  = context.GetInputTexture(mode == xiiViewRenderMode::Wireframe || surface ? m_InputWireframeSurface : m_InputGBufferObjectId);
  const auto camera   = context.GetInputBuffer(m_InputPerFrameCamera);
  if (!selected.IsValid() || !depth.IsValid() || !camera.IsValid() || ((mode == xiiViewRenderMode::Wireframe || mode == xiiViewRenderMode::InstanceID || surface) && !objects.IsValid()))
  {
    xiiLog::Error("Visualization '{}' mode {} requires its selected channel ({}), depth ({}), camera ({}), and identity/surface ({}) inputs.", context.m_sPassName, mode, selected.IsValid(), depth.IsValid(), camera.IsValid(), objects.IsValid());
    return XII_FAILURE;
  }
  if (!xiiScreenSpacePassUtils::Image(context, selected, channels) || !xiiScreenSpacePassUtils::Image(context, depth, 1) ||
      !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Visualization requires float-readable channels, depth and exact camera constants.");
  if (objects.IsValid())
  {
    const auto* description = context.m_Graph.GetTextureDescription(objects);
    const auto  expected    = mode == xiiViewRenderMode::Wireframe || surface ? xiiGALResourceFormat::RGBA32UInt : xiiGALResourceFormat::R32UInt;
    if (!description || description->m_Format != expected)
      return context.ReportFailure("Visualization identities must match the selected R32UInt identity or RGBA32UInt surface-query contract.");
  }
  if (!m_State)
    m_State = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_State->m_Pipeline, "Shaders/Pipeline/ViewRenderMode.xiiShader"));
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_State->m_Point));
  if (!objects.IsValid())
  {
    struct Empty
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    objects = context.m_Graph.AddPass<Empty>("Empty visualization identities", xiiGALCommandQueueFlags::Graphics, [](Empty& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {1, 1};
      description.m_Format = xiiGALResourceFormat::RGBA32UInt;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      data.m_Texture = builder.WriteTexture("Empty identities", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Empty& data, xiiRenderGraphPassContext& execute) { ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero()); }).first->m_Texture;
  }
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Source, m_Depth, m_Objects, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Constants;
    xiiViewRenderMode::Enum                  m_Mode;
    xiiSizeU32                               m_Size;
    float                                    m_fMotionVectorScale;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiSharedPtr<xiiGALSampler>              m_pSampler;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Source = builder.ReadTexture(selected, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Objects = builder.ReadTexture(objects, xiiGALResourceStateFlags::ShaderResource);
    data.m_Output = builder.WriteTexture("View visualization", outputDescription, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = Constants(builder, "ViewRenderModeConstants", sizeof(xiiViewRenderModeConstants));
    data.m_Mode = mode;
    data.m_Size = size;
    data.m_fMotionVectorScale = m_fMotionVectorScale;
    data.m_pPipeline = m_State->m_Pipeline;
    data.m_pSampler = m_State->m_Point; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiViewRenderModeConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      mapped->RenderMode = data.m_Mode;
      mapped->TexCoord1Offset = mapped->ColorOffset = 0;
      mapped->MotionVectorScale = data.m_fMotionVectorScale;
    }
    commands.SetPipelineState(data.m_pPipeline.Borrow());
    commands.ResolveAndSetSampler("g_sPointClamp", data.m_pSampler.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiViewRenderModeConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Source", execute.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_ObjectIds", execute.GetTexture(data.m_Objects)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Output", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind view visualization resources.");
      return;
    }
    commands.DispatchCompute({(data.m_Size.width + 7) / 8, (data.m_Size.height + 7) / 8, 1}); });
  return context.SetOutput(m_OutputDisplayLinearColor, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visualization_ViewRenderModePass);
