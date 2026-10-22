/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/FinalBlitConstants.h>

using namespace xiiRenderPassUtils;
struct xiiPresentPass::Data
{
  xiiRenderGraphTextureHandle                    m_hDisplayLinear, m_hBackbuffer;
  xiiRenderGraphBufferHandle                     m_hConstants;
  xiiFinalBlitConstants                          m_Constants;
  xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
  xiiSharedPtr<xiiGALSampler>                    m_Sampler;
  xiiSizeU32                                     m_Size;
};

struct xiiPresentPass::State
{
  xiiSharedPtr<xiiGALSampler> m_PointSampler;
  DisplayConfiguration        m_Display;
};

xiiPresentPass::xiiPresentPass()  = default;
xiiPresentPass::~xiiPresentPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPresentPass, 1, xiiRTTIDefaultAllocator<xiiPresentPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GradedColorInput", m_InputGradedColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GradedColor")),
    XII_MEMBER_PROPERTY("FinalBlitConstants", m_OutputFinalBlitConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Final Blit Constants")),
    XII_MEMBER_PROPERTY("GradedColorOutput", m_OutputGradedColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GradedColor")),
    XII_MEMBER_PROPERTY("ViewOutput", m_OutputViewOutput)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("OutputMode", xiiDisplayOutputMode, m_OutputMode),
    XII_MEMBER_PROPERTY("PaperWhiteNits", m_fPaperWhiteNits)->AddAttributes(new xiiDefaultValueAttribute(203.0f)),
    XII_MEMBER_PROPERTY("MaximumDisplayNits", m_fMaximumDisplayNits)->AddAttributes(new xiiDefaultValueAttribute(1000.0f)),
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

xiiResult xiiPresentPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiDisplayOutputSettings authored;
  authored.m_OutputMode          = m_OutputMode;
  authored.m_fPaperWhiteNits     = m_fPaperWhiteNits;
  authored.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
  if (!xiiDisplayOutputManager::IsValid(authored))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}

xiiResult xiiPresentPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  if (!context.m_pView)
    return context.ReportFailure("Presentation requires a view with an explicit output target.");
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_pState->m_PointSampler));
  xiiDisplayOutputSettings authored;
  authored.m_OutputMode          = m_OutputMode;
  authored.m_fPaperWhiteNits     = m_fPaperWhiteNits;
  authored.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, authored, m_bUseViewDefaults));
  const auto  input  = context.GetInputTexture(m_InputGradedColor);
  const auto* source = context.m_Graph.GetTextureDescription(input);
  if (!source || source->m_Type != xiiGALResourceDimension::Texture2D || source->m_uiArraySizeOrDepth != 1 || source->m_uiSampleCount != 1 || !source->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALResourceFormat::IsDepthFormat(source->m_Format))
    return context.ReportFailure("Presentation requires one shader-readable, single-sample color image.");
  const auto                        target    = context.m_pView->GetRenderTargetView();
  const auto                        swapchain = target ? nullptr : context.m_pView->GetSwapChain();
  const xiiSharedPtr<xiiGALTexture> texture   = target ? target->GetTexture() : swapchain ? swapchain->GetBackBufferTexture() :
                                                                                            xiiSharedPtr<xiiGALTexture>();
  if (!texture)
    return context.ReportFailure("Presentation has no configured view target or acquired swapchain image.");
  const auto& description = texture->GetDescription();
  if (description.m_Type != xiiGALResourceDimension::Texture2D || description.m_uiArraySizeOrDepth != 1 || description.m_uiSampleCount != 1 || !description.m_Size.width || !description.m_Size.height || !description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || (target && (target->GetDescription().m_ViewType != xiiGALTextureViewType::RenderTarget || target->GetDescription().m_Format != description.m_Format || target->GetDescription().m_uiMostDetailedMip || target->GetDescription().m_uiFirstArrayOrDepthSlice || target->GetDescription().m_uiArrayOrDepthSlicesCount != 1)))
    return context.ReportFailure("Presentation requires a full single-sample 2D render target.");
  const auto                                    handle = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/FinalBlit.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(handle, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("The presentation shader is unavailable.");
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hDisplayLinear = builder.ReadTexture(input, xiiGALResourceStateFlags::ShaderResource);
    data.m_hBackbuffer = builder.WriteTexture(builder.ImportTexture(swapchain ? "Backbuffer" : "ViewOutput", texture, texture->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
    builder.ExportTexture(data.m_hBackbuffer, swapchain ? xiiGALResourceStateFlags::Present : description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ? xiiGALResourceStateFlags::ShaderResource : xiiGALResourceStateFlags::RenderTarget);
    data.m_hConstants = Constants(builder, "Final Blit Constants", sizeof(data.m_Constants));
    const auto& display = m_pState->m_Display.m_Settings;
    data.m_Constants.OutputMode = display.m_OutputMode.GetValue();
    data.m_Constants.ApplySRGBTransfer = xiiGALResourceFormat::IsSrgb(description.m_Format) ? 0U : 1U;
    data.m_Constants.PaperWhiteNits = display.m_fPaperWhiteNits;
    data.m_Constants.MaximumDisplayNits = display.m_fMaximumDisplayNits;
    data.m_Size = description.m_Size;
    data.m_Sampler = m_pState->m_PointSampler;
    data.m_Pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
    data.m_Pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex);
    data.m_Pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
    data.m_Pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState();
    data.m_Pipeline.m_GraphicsPipeline.m_pRasterizerState = shader->GetRasterizerState();
    data.m_Pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
    data.m_Pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
    builder.SetPassAllowMerge(false);
    builder.SetPassRenderPassManaged();
    builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiFinalBlitConstants> mapped(commands, execute.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Constants;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execute.GetBuffer(data.m_hConstants);
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    }); }, Execute, true);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputFinalBlitConstants, emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGradedColor, input));
  return context.SetOutput(m_OutputViewOutput, emitted.first->m_hBackbuffer);
}

void xiiPresentPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  auto description                                = data.m_Pipeline;
  description.m_GraphicsPipeline.m_pRenderPass    = context.GetRenderPass();
  description.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(context.GetSubpassIndex());
  const auto pipeline                             = xiiGALPipelineCache::GetPipeline(description);
  if (!pipeline)
  {
    xiiLog::Error("Could not create the authored presentation pipeline.");
    return;
  }
  auto& commands = context.GetCommandList();
  commands.SetViewport({0, 0, float(data.m_Size.width), float(data.m_Size.height), 0, 1});
  commands.SetScissorRect({0, 0, data.m_Size.width, data.m_Size.height});
  commands.SetPipelineState(pipeline.Borrow());
  commands.ResolveAndSetSampler("PointClampSampler", data.m_Sampler.Borrow());
  commands.ResolveAndSetConstantBuffer("xiiFinalBlitConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
  commands.ResolveAndSetShaderResourceTextureView("g_FinalColor", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
  commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
  commands.Draw({3U, 1U, 0U, 0U});
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_PresentPass);
