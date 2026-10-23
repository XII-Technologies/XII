/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsTemporalPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Reflections/SSRTemporalConstants.h>

using namespace xiiRenderPassUtils;
struct xiiScreenSpaceReflectionsTemporalPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALTexture>              m_History[2], m_Identity[2];
  xiiUInt32                                m_uiWriteIndex  = 0;
  bool                                     m_bHistoryValid = false;
};
struct xiiScreenSpaceReflectionsTemporalPass::Data
{
  xiiUInt32 m_uiHistoryWriteIndex = 0U;
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hCurrentReflection;
  xiiRenderGraphTextureHandle m_hPreviousReflection;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferObjectId;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hPreviousIdentitySignature;
  xiiRenderGraphTextureHandle m_hResolvedReflection;
  xiiRenderGraphTextureHandle m_hCurrentIdentitySignature;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;

  xiiSizeU32                 m_Size;
  xiiSSRTemporalConstants    m_Constants;
  xiiRenderGraphBufferHandle m_hPerFrameCamera;
};
xiiScreenSpaceReflectionsTemporalPass::xiiScreenSpaceReflectionsTemporalPass()  = default;
xiiScreenSpaceReflectionsTemporalPass::~xiiScreenSpaceReflectionsTemporalPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceReflectionsTemporalPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceReflectionsTemporalPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SSRRawTerm", m_InputSSRRawTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SSRRawTermOutput", m_OutputSSRRawTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SSRRawTerm")),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepthOutput", m_OutputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SceneDepth")),
    XII_MEMBER_PROPERTY("GBufferNormal", m_InputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormalOutput", m_OutputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferNormal")),
    XII_MEMBER_PROPERTY("GBufferMaterial", m_InputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferMaterialOutput", m_OutputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferMaterial")),
    XII_MEMBER_PROPERTY("GBufferObjectId", m_InputGBufferObjectId)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferObjectIdOutput", m_OutputGBufferObjectId)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferObjectId")),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_InputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBufferOutput", m_OutputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DilatedVelocityBuffer")),
    XII_MEMBER_PROPERTY("ReSTIRDISurfacePrevious", m_InputReSTIRDISurfacePrevious)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ReSTIRDISurfacePreviousOutput", m_OutputReSTIRDISurfacePrevious)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("ReSTIRDISurfacePrevious")),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_InputPerFrameCamera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SSRTerm", m_OutputSSRTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Identity", m_OutputIdentity)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Constants", m_OutputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("xiiSSRTemporalConstants")),
    XII_MEMBER_PROPERTY("PreviousReflection", m_OutputPreviousReflection)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PreviousIdentity", m_OutputPreviousIdentity)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight)->AddAttributes(new xiiDefaultValueAttribute(0.92f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("ConfidenceDecay", m_fConfidenceDecay)->AddAttributes(new xiiDefaultValueAttribute(0.96f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.02f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("NormalThreshold", m_fNormalThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.95f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Reflections") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiScreenSpaceReflectionsTemporalPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->m_bHistoryValid = false;
}
xiiResult xiiScreenSpaceReflectionsTemporalPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  for (float value : {m_fHistoryWeight, m_fConfidenceDecay, m_fDepthThreshold, m_fNormalThreshold})
    if (!xiiMath::IsFinite(value) || value < 0 || value > 1)
      return XII_FAILURE;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}
xiiResult xiiScreenSpaceReflectionsTemporalPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  xiiSizeU32                 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, m_OutputSSRTerm, context.GetInputTexture(m_InputSSRRawTerm), size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputSSRRawTerm));
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputSSRRawTerm)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputSceneDepth)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputGBufferNormal)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputGBufferMaterial)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputGBufferObjectId)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputDilatedVelocityBuffer)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputReSTIRDISurfacePrevious)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SSRTemporal.xiiShader"));
  xiiGALTextureCreationDescription radiance;
  radiance.m_Type      = xiiGALResourceDimension::Texture2D;
  radiance.m_Size      = size;
  radiance.m_Format    = xiiGALResourceFormat::RGBA16Float;
  radiance.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  auto identity        = radiance;
  identity.m_Format    = xiiGALResourceFormat::RG32UInt;
  if (!m_pState->m_History[0] || m_pState->m_History[0]->GetDescription() != radiance || !m_pState->m_Identity[0] || m_pState->m_Identity[0]->GetDescription() != identity)
  {
    // Publish all four images together. Failed replacement leaves the previous
    // allocation retained; the runtime invalidates history after failed Build.
    xiiSharedPtr<xiiGALTexture> histories[2], identities[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      histories[i]  = xiiGALDevice::GetDefaultDevice()->CreateTexture(radiance);
      identities[i] = xiiGALDevice::GetDefaultDevice()->CreateTexture(identity);
      if (!histories[i] || !identities[i])
        return XII_FAILURE;
    }
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      m_pState->m_History[i]  = std::move(histories[i]);
      m_pState->m_Identity[i] = std::move(identities[i]);
    }
    m_pState->m_bHistoryValid = false;
    m_pState->m_uiWriteIndex  = 0;
  }
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hPerFrameCamera = context.GetInputBuffer(m_InputPerFrameCamera);
    data.m_Constants.HistoryWeight = m_fHistoryWeight;
    data.m_Constants.ConfidenceDecay = m_fConfidenceDecay;
    data.m_Constants.DepthThreshold = m_fDepthThreshold;
    data.m_Constants.NormalThreshold = m_fNormalThreshold;
    data.m_hCurrentReflection = context.GetInputTexture(m_InputSSRRawTerm);
    data.m_hSceneDepth = context.GetInputTexture(m_InputSceneDepth);
    data.m_hGBufferNormal = context.GetInputTexture(m_InputGBufferNormal);
    data.m_hGBufferMaterial = context.GetInputTexture(m_InputGBufferMaterial);
    data.m_hGBufferObjectId = context.GetInputTexture(m_InputGBufferObjectId);
    data.m_hVelocity = context.GetInputTexture(m_InputDilatedVelocityBuffer);
    data.m_hPreviousSurface = context.GetInputTexture(m_InputReSTIRDISurfacePrevious);
    Setup(data, builder); }, xiiMakeDelegate(&xiiScreenSpaceReflectionsTemporalPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSSRRawTerm, emitted.first->m_hCurrentReflection));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSceneDepth, emitted.first->m_hSceneDepth));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGBufferNormal, emitted.first->m_hGBufferNormal));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGBufferMaterial, emitted.first->m_hGBufferMaterial));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGBufferObjectId, emitted.first->m_hGBufferObjectId));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputDilatedVelocityBuffer, emitted.first->m_hVelocity));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputReSTIRDISurfacePrevious, emitted.first->m_hPreviousSurface));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSSRTerm, emitted.first->m_hResolvedReflection));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputIdentity, emitted.first->m_hCurrentIdentitySignature));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputConstants, emitted.first->m_hConstants));
  return XII_SUCCESS;
}
void xiiScreenSpaceReflectionsTemporalPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  const xiiUInt32 uiCurrentSlot  = m_pState->m_uiWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = *m_pState;


  data.m_bHistoryValid              = resources.m_bHistoryValid;
  data.m_hCurrentReflection         = builder.ReadTexture(data.m_hCurrentReflection, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal             = builder.ReadTexture(data.m_hGBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial           = builder.ReadTexture(data.m_hGBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferObjectId           = builder.ReadTexture(data.m_hGBufferObjectId, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                  = builder.ReadTexture(data.m_hVelocity, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface           = builder.ReadTexture(data.m_hPreviousSurface, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReflection        = builder.ReadTexture(builder.ImportTexture("SSR Previous History", resources.m_History[uiPreviousSlot], resources.m_History[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousIdentitySignature = builder.ReadTexture(builder.ImportTexture("SSR Previous Identity Signature", resources.m_Identity[uiPreviousSlot], resources.m_Identity[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hResolvedReflection        = builder.WriteTexture(builder.ImportTexture("SSRTerm", resources.m_History[uiCurrentSlot], resources.m_History[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCurrentIdentitySignature  = builder.WriteTexture(builder.ImportTexture("SSR Current Identity Signature", resources.m_Identity[uiCurrentSlot], resources.m_Identity[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRTemporalConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRTemporalConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);



  data.m_hPerFrameCamera        = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_Constants.HistoryValid = data.m_bHistoryValid ? 1U : 0U;
  data.m_Constants._Padding     = xiiVec3U32::MakeZero();
}
void xiiScreenSpaceReflectionsTemporalPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSRTemporal");
  {
    {
      xiiGALMapHelper<xiiSSRTemporalConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_pState->m_Pipeline);

    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRTemporalConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentReflection", context.GetTexture(data.m_hCurrentReflection)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousReflection", context.GetTexture(data.m_hPreviousReflection)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferObjectId", context.GetTexture(data.m_hGBufferObjectId)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousIdentitySignature", context.GetTexture(data.m_hPreviousIdentitySignature)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResolvedReflection", context.GetTexture(data.m_hResolvedReflection)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_CurrentIdentitySignature", context.GetTexture(data.m_hCurrentIdentitySignature)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
    m_pState->m_bHistoryValid = true;
    m_pState->m_uiWriteIndex  = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}
xiiResult xiiScreenSpaceReflectionsTemporalPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_pState || !m_pState->m_bHistoryValid)
    return XII_SUCCESS;
  xiiRenderPassResourceScope scope(context);
  struct Previous
  {
    xiiRenderGraphTextureHandle m_Radiance, m_Identity;
  };
  const auto imported = context.m_Graph.AddPass<Previous>("SSR previous images", xiiGALCommandQueueFlags::Compute, [&](Previous& data, xiiRenderGraphBuilder& builder) {
    const auto index = 1U - m_pState->m_uiWriteIndex;
    const auto& radiance = m_pState->m_History[index];
    const auto& identity = m_pState->m_Identity[index];
    data.m_Radiance = builder.ImportTexture("PreviousReflection", radiance, radiance->GetResourceState());
    data.m_Identity = builder.ImportTexture("PreviousIdentity", identity, identity->GetResourceState()); }, [](const Previous&, xiiRenderGraphPassContext&) {});
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputPreviousReflection, imported.first->m_Radiance));
  return context.SetOutput(m_OutputPreviousIdentity, imported.first->m_Identity);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Reflections_ScreenSpaceReflectionsTemporalPass);
