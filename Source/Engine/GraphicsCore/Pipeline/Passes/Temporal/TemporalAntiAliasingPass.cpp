/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/TemporalHistory.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Temporal/TAAConstants.h>

using namespace xiiRenderPassUtils;
struct xiiTemporalAntiAliasingPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRIn;    ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hVelocity; ///< ShaderResource in (motion vectors).
  xiiRenderGraphTextureHandle m_hDepth;    ///< ShaderResource in (reversed-Z scene depth).
  xiiRenderGraphTextureHandle m_hHistory;  ///< ShaderResource in (history color).
  xiiRenderGraphTextureHandle m_hTAAOut;   ///< UnorderedAccess out (TAA resolved color).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiTAAConstants             m_Constants           = {};
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  bool                        m_bHistoryValid       = false;
  xiiSizeU32                  m_Size;
};

struct xiiTemporalAntiAliasingPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALTexture>              m_History[2];
  xiiSharedPtr<xiiGALSampler>              m_LinearSampler;
  xiiSharedPtr<xiiGALSampler>              m_PointSampler;
  xiiUInt32                                m_uiWriteIndex  = 0;
  bool                                     m_bHistoryValid = false;
};

xiiTemporalAntiAliasingPass::xiiTemporalAntiAliasingPass()  = default;
xiiTemporalAntiAliasingPass::~xiiTemporalAntiAliasingPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalAntiAliasingPass, 1, xiiRTTIDefaultAllocator<xiiTemporalAntiAliasingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HDRSceneColor", m_InputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_InputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("TAAResolved", m_OutputTAAResolved)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PreviousColor", m_OutputPreviousColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BaseBlendAlpha", m_fBaseBlendAlpha)->AddAttributes(new xiiDefaultValueAttribute(0.08f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MotionBlendScale", m_fMotionBlendScale)->AddAttributes(new xiiDefaultValueAttribute(12.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Temporal")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiTemporalAntiAliasingPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->m_bHistoryValid = false;
}
xiiResult xiiTemporalAntiAliasingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fBaseBlendAlpha) || m_fBaseBlendAlpha < 0 || m_fBaseBlendAlpha > 1 || !xiiMath::IsFinite(m_fMotionBlendScale) || m_fMotionBlendScale < 0)
    return XII_FAILURE;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiTemporalAntiAliasingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/TAA.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_LinearSampler));
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_pState->m_PointSampler));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture(m_InputHDRSceneColor), 0, 0, size));
  for (const auto name : {"DilatedVelocityBuffer", "SceneDepth"})
  {
    const auto* input = context.m_Graph.GetTextureDescription(context.GetInputTexture(name));
    if (!input || input->m_Size != size)
      return XII_FAILURE;
  }
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Format    = xiiGALResourceFormat::RGBA16Float;
  description.m_Size      = size;
  description.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  const auto* previous    = m_pState->m_History[0].Borrow();
  XII_SUCCEED_OR_RETURN(EnsureTemporalHistoryTextures(m_pState->m_History, description, m_pState->m_bHistoryValid, [](const auto& desc) { return xiiGALDevice::GetDefaultDevice()->CreateTexture(desc); }));
  if (previous != m_pState->m_History[0].Borrow())
    m_pState->m_uiWriteIndex = 0;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hHDRIn = context.GetInputTexture(m_InputHDRSceneColor);
    data.m_hVelocity = context.GetInputTexture(m_InputDilatedVelocityBuffer);
    data.m_hDepth = context.GetInputTexture(m_InputSceneDepth);
    data.m_Constants.BaseBlendAlpha = m_fBaseBlendAlpha;
    data.m_Constants.MotionBlendScale = m_fMotionBlendScale;
    Setup(data, builder); }, xiiMakeDelegate(&xiiTemporalAntiAliasingPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputTAAResolved, emitted.first->m_hTAAOut));
  return XII_SUCCESS;
}

void xiiTemporalAntiAliasingPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRIn    = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity = builder.ReadTexture(data.m_hVelocity, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDepth    = builder.ReadTexture(data.m_hDepth, xiiGALResourceStateFlags::ShaderResource);

  auto& temporalResources    = *m_pState;
  data.m_uiHistoryWriteIndex = temporalResources.m_uiWriteIndex;
  data.m_bHistoryValid       = temporalResources.m_bHistoryValid;

  const xiiUInt32 uiHistoryReadIndex = 1U - data.m_uiHistoryWriteIndex;
  data.m_hHistory                    = builder.ImportTexture("TAAHistory", temporalResources.m_History[uiHistoryReadIndex], temporalResources.m_History[uiHistoryReadIndex]->GetResourceState());
  data.m_hHistory                    = builder.ReadTexture(data.m_hHistory, xiiGALResourceStateFlags::ShaderResource);

  data.m_hTAAOut = builder.ImportTexture("TAAResolved", temporalResources.m_History[data.m_uiHistoryWriteIndex], temporalResources.m_History[data.m_uiHistoryWriteIndex]->GetResourceState());
  data.m_hTAAOut = builder.WriteTexture(data.m_hTAAOut, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiTAAConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("TAA Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_Constants.HistoryValid = data.m_bHistoryValid ? 1U : 0U;
  data.m_Constants.Padding      = 0.0f;
}

void xiiTemporalAntiAliasingPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = data.m_Size.width;
  const xiiUInt32    uiRenderHeight = data.m_Size.height;

  cmd.BeginDebugGroup("TAA");
  {
    {
      xiiGALMapHelper<xiiTAAConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }

    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_LinearSampler.Borrow());
    cmd.ResolveAndSetSampler("PointClampSampler", m_pState->m_PointSampler.Borrow());

    cmd.ResolveAndSetConstantBuffer("xiiTAAConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentFrame", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VelocityBuffer", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HistoryFrame", context.GetTexture(data.m_bHistoryValid ? data.m_hHistory : data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Resolved", context.GetTexture(data.m_hTAAOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});

    m_pState->m_uiWriteIndex  = 1U - data.m_uiHistoryWriteIndex;
    m_pState->m_bHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_TemporalAntiAliasingPass);

xiiResult xiiTemporalAntiAliasingPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_pState || !m_pState->m_bHistoryValid)
    return XII_SUCCESS;
  xiiRenderPassResourceScope scope(context);
  const auto&                previous = m_pState->m_History[1U - m_pState->m_uiWriteIndex];
  if (!previous)
    return XII_SUCCESS;
  const auto imported = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>("TAA previous color", xiiGALCommandQueueFlags::Compute, [&](xiiRenderGraphTextureHandle& data, xiiRenderGraphBuilder& builder) { data = builder.ImportTexture("PreviousColor", previous, previous->GetResourceState()); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_OutputPreviousColor, *imported.first);
}
