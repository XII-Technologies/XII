/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/UpscalePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

struct xiiUpscalePass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTAAIn;    ///< ShaderResource in (TAA resolved color).
  xiiRenderGraphTextureHandle m_hUpscaled; ///< UnorderedAccess out (upscaled HDR color).
  xiiSizeU32                  m_Size;
};
struct xiiUpscalePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Sampler;
};

xiiUpscalePass::xiiUpscalePass()  = default;
xiiUpscalePass::~xiiUpscalePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUpscalePass, 1, xiiRTTIDefaultAllocator<xiiUpscalePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TAAResolved", m_InputTAAResolved)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UpscaledColor", m_OutputUpscaledColor),


  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Output")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


xiiResult xiiUpscalePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (m_OutputUpscaledColor.GetResolution().Validate().Failed())
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiResult xiiUpscalePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/CASUpscale.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_pState->m_Sampler));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputUpscaledColor, context.GetInputTexture(m_InputTAAResolved), size, xiiRenderPipelineTextureResolutionMode::Viewport));
  auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size=size;
    data.m_hTAAIn=context.GetInputTexture(m_InputTAAResolved);
    Setup(data,builder); }, xiiMakeDelegate(&xiiUpscalePass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputUpscaledColor, emitted.first->m_hUpscaled));
  return XII_SUCCESS;
}

void xiiUpscalePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiOutputWidth  = data.m_Size.width;
  const xiiUInt32 uiOutputHeight = data.m_Size.height;

  data.m_hTAAIn = builder.ReadTexture(data.m_hTAAIn, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hUpscaled          = builder.WriteTexture("UpscaledColor", description, xiiGALResourceStateFlags::UnorderedAccess);
}

void xiiUpscalePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = data.m_Size.width;
  const xiiUInt32    uiOutputHeight = data.m_Size.height;

  cmd.BeginDebugGroup("Upscale");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetSampler("g_sLinearClamp", m_pState->m_Sampler.Borrow());
    cmd.ResolveAndSetShaderResourceTextureView("g_TAAIn", context.GetTexture(data.m_hTAAIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Upscaled", context.GetTexture(data.m_hUpscaled)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_UpscalePass);
