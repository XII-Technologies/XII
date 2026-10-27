/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Reflections/SSRConstants.h>

using namespace xiiRenderPassUtils;
static_assert(sizeof(xiiSSRConstants) == 48);
struct xiiScreenSpaceReflectionsPass::Data
{
  xiiRenderGraphTextureHandle m_hSceneDepth;             ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;          ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;        ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;          ///< ShaderResource in (base color and metallic F0 source).
  xiiRenderGraphTextureHandle m_hBRDFLut;                ///< ShaderResource in (split-sum material response).
  xiiRenderGraphTextureHandle m_hHiZClosestPyramid;      ///< ShaderResource in (reversed-Z maximum hierarchy used for near depth bounds).
  xiiRenderGraphTextureHandle m_hHiZFarthestPyramid;     ///< ShaderResource in (reversed-Z minimum hierarchy used for far depth bounds).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;          ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections; ///< UnorderedAccess out (screen-space reflections texture).
  xiiRenderGraphBufferHandle  m_hConstants;              ///< ConstantBuffer in (SSR quality and traversal settings).
  xiiSSRConstants             m_Constants;

  xiiSizeU32                 m_Size;
  xiiRenderGraphBufferHandle               m_hPerFrameCamera;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear, m_Point;
};
struct xiiScreenSpaceReflectionsPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear, m_Point;
};
xiiScreenSpaceReflectionsPass::xiiScreenSpaceReflectionsPass()
{
  m_OutputSSRRawTerm.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiScreenSpaceReflectionsPass::~xiiScreenSpaceReflectionsPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceReflectionsPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceReflectionsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_InputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferMaterial", m_InputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferAlbedo", m_InputGBufferAlbedo)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BRDFLut", m_InputBRDFLut)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HiZClosestPyramid", m_InputHiZClosestPyramid)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HiZPyramid", m_InputHiZPyramid)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HDRSceneColor", m_InputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SSRRawTerm", m_OutputSSRRawTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepthOutput", m_OutputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SceneDepth")),
    XII_MEMBER_PROPERTY("GBufferNormalOutput", m_OutputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferNormal")),
    XII_MEMBER_PROPERTY("GBufferMaterialOutput", m_OutputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferMaterial")),
    XII_MEMBER_PROPERTY("GBufferAlbedoOutput", m_OutputGBufferAlbedo)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferAlbedo")),
    XII_MEMBER_PROPERTY("BRDFLutOutput", m_OutputBRDFLut)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("BRDFLut")),
    XII_MEMBER_PROPERTY("HiZClosestPyramidOutput", m_OutputHiZClosestPyramid)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HiZClosestPyramid")),
    XII_MEMBER_PROPERTY("HiZPyramidOutput", m_OutputHiZPyramid)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HiZPyramid")),
    XII_MEMBER_PROPERTY("HDRSceneColorOutput", m_OutputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_InputPerFrameCamera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameGlobal", m_InputPerFrameGlobal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Constants", m_OutputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("xiiSSRConstants")),
    XII_MEMBER_PROPERTY("AllowAbsentDepth", m_bAllowAbsentDepth)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new xiiDefaultValueAttribute(50.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_MEMBER_PROPERTY("SurfaceBias", m_fSurfaceBias)->AddAttributes(new xiiDefaultValueAttribute(0.01f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("MaxSteps", m_uiMaxSteps)->AddAttributes(new xiiDefaultValueAttribute(96U), new xiiClampValueAttribute(1U, 4096U)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.0025f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MaxRoughness", m_fMaxRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.72f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("StrideZCutoff", m_fStrideZCutoff)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Reflections") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiScreenSpaceReflectionsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fMaxDistance) && m_fMaxDistance > 0 && xiiMath::IsFinite(m_fSurfaceBias) && m_fSurfaceBias >= 0 && m_fSurfaceBias < m_fMaxDistance && m_uiMaxSteps > 0 && m_uiMaxSteps <= 4096 && xiiMath::IsFinite(m_fThickness) && m_fThickness >= 0 && m_fThickness <= 1 &&
      xiiMath::IsFinite(m_fMaxRoughness) && m_fMaxRoughness >= 0 && m_fMaxRoughness <= 1 && xiiMath::IsFinite(m_fStrideZCutoff) && m_fStrideZCutoff >= 0 && m_fStrideZCutoff <= 1 ?
    XII_SUCCESS :
    XII_FAILURE;
}
void xiiScreenSpaceReflectionsPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_sName == "SceneDepth" || out_pins[i].m_sName == "HiZClosestPyramid" || out_pins[i].m_sName == "HiZPyramid" || out_pins[i].m_sName == "HDRSceneColor" || out_pins[i].m_sName == "BRDFLut" || out_pins[i].m_sName == "SSRRawTerm" || out_pins[i].m_sName == "xiiSSRConstants")
      out_pins[i].m_bOptional = m_bAllowAbsentDepth;
}

xiiResult xiiScreenSpaceReflectionsPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  const auto                 forward = [&](const xiiRenderPipelineTextureInputPin& input, const xiiRenderPipelineTextureOutputPin& output) {
    const auto handle = context.GetInputTexture(input);
    return handle.IsValid() ? context.SetOutput(output, handle) : XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(forward(m_InputSceneDepth, m_OutputSceneDepth));
  XII_SUCCEED_OR_RETURN(forward(m_InputGBufferNormal, m_OutputGBufferNormal));
  XII_SUCCEED_OR_RETURN(forward(m_InputGBufferMaterial, m_OutputGBufferMaterial));
  XII_SUCCEED_OR_RETURN(forward(m_InputGBufferAlbedo, m_OutputGBufferAlbedo));
  XII_SUCCEED_OR_RETURN(forward(m_InputBRDFLut, m_OutputBRDFLut));
  XII_SUCCEED_OR_RETURN(forward(m_InputHiZClosestPyramid, m_OutputHiZClosestPyramid));
  XII_SUCCEED_OR_RETURN(forward(m_InputHiZPyramid, m_OutputHiZPyramid));
  XII_SUCCEED_OR_RETURN(forward(m_InputHDRSceneColor, m_OutputHDRSceneColor));
  if (!context.GetInputTexture(m_InputSceneDepth).IsValid() && m_bAllowAbsentDepth)
    return XII_SUCCESS;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SSR.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_pState->m_Point));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, m_OutputSSRRawTerm, context.GetInputTexture(m_InputHDRSceneColor), size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputHDRSceneColor));
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  if (!xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputSceneDepth), 1, &size) ||
      !xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputGBufferNormal), 2, &size) ||
      !xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputGBufferMaterial), 4, &size) ||
      !xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputGBufferAlbedo), 4, &size) ||
      !xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputHDRSceneColor), 4, &size))
    return context.ReportFailure("Screen reflections require matching readable depth, normal, material, albedo and HDR images.");
  const auto* closest  = xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputHiZClosestPyramid), 1, &size);
  const auto* farthest = xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputHiZPyramid), 1, &size);
  if (!closest || !farthest || closest->m_Format != xiiGALResourceFormat::R32Float || farthest->m_Format != xiiGALResourceFormat::R32Float ||
      !xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputBRDFLut), 2) ||
      !xiiScreenSpacePassUtils::Uniform(context, context.GetInputBuffer(m_InputPerFrameCamera), sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Screen reflections require viewport-sized R32Float closest/farthest hierarchies, a readable BRDF lookup and exact camera constants.");
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSceneDepth = context.GetInputTexture(m_InputSceneDepth);
    data.m_hGBufferNormal = context.GetInputTexture(m_InputGBufferNormal);
    data.m_hGBufferMaterial = context.GetInputTexture(m_InputGBufferMaterial);
    data.m_hGBufferAlbedo = context.GetInputTexture(m_InputGBufferAlbedo);
    data.m_hBRDFLut = context.GetInputTexture(m_InputBRDFLut);
    data.m_hHiZClosestPyramid = context.GetInputTexture(m_InputHiZClosestPyramid);
    data.m_hHiZFarthestPyramid = context.GetInputTexture(m_InputHiZPyramid);
    data.m_hHDRSceneColor = context.GetInputTexture(m_InputHDRSceneColor);
    data.m_hPerFrameCamera = context.GetInputBuffer(m_InputPerFrameCamera);
    data.m_Pipeline = m_pState->m_Pipeline;data.m_Linear=m_pState->m_Linear;data.m_Point=m_pState->m_Point;
    const auto* closest = context.m_Graph.GetTextureDescription(data.m_hHiZClosestPyramid);
    const auto* farthest = context.m_Graph.GetTextureDescription(data.m_hHiZFarthestPyramid);
    data.m_Constants.HiZMipCount = xiiMath::Min(closest->m_uiMipLevels, farthest->m_uiMipLevels);
    data.m_Constants.MaxSteps = m_uiMaxSteps;
    data.m_Constants.Thickness = m_fThickness;
    data.m_Constants.MaxRoughness = m_fMaxRoughness;
    data.m_Constants.StrideZCutoff = m_fStrideZCutoff;
    data.m_Constants.MaxDistance = m_fMaxDistance;data.m_Constants.SurfaceBias = m_fSurfaceBias;
    Setup(data, builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSSRRawTerm, emitted.first->m_hScreenSpaceReflections));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputConstants, emitted.first->m_hConstants));
  return XII_SUCCESS;
}
void xiiScreenSpaceReflectionsPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth         = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal      = builder.ReadTexture(data.m_hGBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial    = builder.ReadTexture(data.m_hGBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo      = builder.ReadTexture(data.m_hGBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut            = builder.ReadTexture(data.m_hBRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHiZClosestPyramid  = builder.ReadTexture(data.m_hHiZClosestPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHiZFarthestPyramid = builder.ReadTexture(data.m_hHiZFarthestPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor      = builder.ReadTexture(data.m_hHDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA32Float;
  description.m_Size.width       = data.m_Size.width;
  description.m_Size.height      = data.m_Size.height;
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hScreenSpaceReflections = builder.WriteTexture("SSRRawTerm", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiUInt32 uiMipWidth  = data.m_Size.width;
  xiiUInt32 uiMipHeight = data.m_Size.height;
  xiiUInt32 uiMipCount  = 1U;
  while (uiMipWidth > 1U || uiMipHeight > 1U)
  {
    uiMipWidth  = xiiMath::Max(uiMipWidth >> 1U, 1U);
    uiMipHeight = xiiMath::Max(uiMipHeight >> 1U, 1U);
    ++uiMipCount;
  }

  data.m_Constants.InvResolution = xiiVec2(1.0f / static_cast<float>(data.m_Size.width), 1.0f / static_cast<float>(data.m_Size.height));
  data.m_Constants.HiZMipCount   = xiiMath::Min(data.m_Constants.HiZMipCount, uiMipCount);
  data.m_Constants._Padding      = xiiVec3::MakeZero();


  data.m_hPerFrameCamera = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
}
void xiiScreenSpaceReflectionsPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSR");
  {
    {
      xiiGALMapHelper<xiiSSRConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());

    cmd.ResolveAndSetSampler("LinearClampSampler", data.m_Linear.Borrow());
    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);

    cmd.ResolveAndSetSampler("PointClampSampler", data.m_Point.Borrow());

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HiZClosest", context.GetTexture(data.m_hHiZClosestPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HiZFarthest", context.GetTexture(data.m_hHiZFarthestPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSROut", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      cmd.EndDebugGroup();
      context.ReportFailure("Could not bind screen reflection resources.");
      return;
    }
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Reflections_ScreenSpaceReflectionsPass);
