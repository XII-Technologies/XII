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
  xiiRenderGraphTextureHandle                    m_hTargetToBind;
  xiiRenderGraphBufferHandle                     m_hConstants;
  xiiFinalBlitConstants                          m_Constants;
  xiiDisplayOutputSettings                       m_AuthoredDisplay;
  bool                                           m_bUseViewDefaults = false;
  xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
  xiiSharedPtr<xiiGALSampler>                    m_Sampler;
  xiiSizeU32                                     m_Size;
  xiiVec2U32                                     m_vOffset;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPresentPass, 1, xiiRTTIDefaultAllocator<xiiPresentPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GradedColorInput", m_InputGradedColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GradedColor")),
    XII_MEMBER_PROPERTY("Target", m_InputTarget)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
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
  return XII_SUCCESS;
}

xiiResult xiiPresentPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
    return context.SetOutput(m_OutputGradedColor, context.GetInputTexture(m_InputGradedColor));
  xiiRenderPassResourceScope scope(context);
  if (!context.m_pView)
    return context.ReportFailure("Presentation requires a view with an explicit output target.");
  const auto  input  = context.GetInputTexture(m_InputGradedColor);
  const auto* source = context.m_Graph.GetTextureDescription(input);
  if (!source || source->m_Type != xiiGALResourceDimension::Texture2D || source->m_uiArraySizeOrDepth != 1 || source->m_uiSampleCount != 1 || !source->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALResourceFormat::IsDepthFormat(source->m_Format))
    return context.ReportFailure("Presentation requires one shader-readable, single-sample color image.");
  const auto                        target      = context.m_pView->GetRenderTargetView();
  const auto                        boundTarget = context.GetInputTexture(m_InputTarget);
  const auto                        swapchain   = target ? nullptr : context.m_pView->GetSwapChain();
  const xiiSharedPtr<xiiGALTexture> texture     = target ? target->GetTexture() : swapchain ? swapchain->GetBackBufferTexture() :
                                                                                              xiiSharedPtr<xiiGALTexture>();
  if (!texture && !boundTarget.IsValid())
    return context.ReportFailure("Presentation has no configured view target or acquired swapchain image.");
  const auto* targetDescription = boundTarget.IsValid() ? context.m_Graph.GetTextureDescription(boundTarget) : &texture->GetDescription();
  if (!targetDescription)
    return context.ReportFailure("Presentation received an invalid target binding.");
  const auto& description = *targetDescription;
  if (description.m_Type != xiiGALResourceDimension::Texture2D || description.m_uiArraySizeOrDepth != 1U ||
      description.m_uiSampleCount != 1U || !description.m_Size.HasNonZeroArea() || !description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
    return context.ReportFailure("Presentation requires a full single-sample 2D render target.");
  xiiSharedPtr<xiiGALSampler> pSampler;
  XII_SUCCEED_OR_RETURN(PointClampSampler(pSampler));

  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hDisplayLinear = builder.ReadTexture(input, xiiGALResourceStateFlags::ShaderResource);
    auto imported         = boundTarget;
    if (!imported.IsValid())
    {
      data.m_hTargetToBind = builder.DeclareImportedTexture(swapchain ? "Backbuffer" : "ViewOutput", description);
      imported             = data.m_hTargetToBind;
    }
    // The viewport can cover only part of the image. Keep the output contents outside it.
    data.m_hBackbuffer = builder.ReadWriteTexture(imported, xiiGALResourceStateFlags::RenderTarget);
    builder.ExportTexture(data.m_hBackbuffer, swapchain ? xiiGALResourceStateFlags::Present : description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ? xiiGALResourceStateFlags::ShaderResource : xiiGALResourceStateFlags::RenderTarget);
    data.m_hConstants = Constants(builder, "Final Blit Constants", sizeof(data.m_Constants));
    data.m_AuthoredDisplay.m_OutputMode          = m_OutputMode;
    data.m_AuthoredDisplay.m_fPaperWhiteNits     = m_fPaperWhiteNits;
    data.m_AuthoredDisplay.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
    data.m_bUseViewDefaults                      = m_bUseViewDefaults;
    data.m_Sampler                               = pSampler;
    builder.SetPassFramePreparation([&data](xiiRenderGraph& graph, const xiiView* pView, xiiStringBuilder& ref_error) {
      return PrepareFrame(data, graph, pView, ref_error);
    });
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

xiiResult xiiPresentPass::PrepareFrame(Data& ref_data, xiiRenderGraph& graph, const xiiView* pView, xiiStringBuilder& ref_error)
{
  if (pView == nullptr)
  {
    ref_error = "Presentation requires a current view.";
    return XII_FAILURE;
  }
  const auto* pTargetDescription = graph.GetTextureDescription(ref_data.m_hBackbuffer);
  const auto* pTargetView        = pView->GetRenderTargetView();
  if (pTargetDescription == nullptr ||
      (pTargetView != nullptr && (pTargetView->GetDescription().m_ViewType != xiiGALTextureViewType::RenderTarget || pTargetView->GetDescription().m_Format != pTargetDescription->m_Format || pTargetView->GetDescription().m_uiMostDetailedMip != 0U || pTargetView->GetDescription().m_uiFirstArrayOrDepthSlice != 0U || pTargetView->GetDescription().m_uiArrayOrDepthSlicesCount != 1U)))
  {
    ref_error = "Presentation requires a full render target view of the declared format.";
    return XII_FAILURE;
  }

  const auto& viewport = pView->GetViewport();
  if (!xiiMath::IsFinite(viewport.x) || !xiiMath::IsFinite(viewport.y) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) ||
      viewport.x < 0 || viewport.y < 0 || viewport.width < 1 || viewport.height < 1 ||
      static_cast<double>(viewport.x) + viewport.width > pTargetDescription->m_Size.width ||
      static_cast<double>(viewport.y) + viewport.height > pTargetDescription->m_Size.height)
  {
    ref_error = "The presentation viewport must fit inside its output target.";
    return XII_FAILURE;
  }

  if (ref_data.m_hTargetToBind.IsValid())
  {
    const auto* pSwapChain = pView->GetSwapChain();
    auto        pTexture   = pTargetView ? pTargetView->GetTexture() : pSwapChain ? pSwapChain->GetBackBufferTexture() :
                                                                                    xiiSharedPtr<xiiGALTexture>();
    if (pTexture == nullptr || graph.BindImportedTexture(ref_data.m_hTargetToBind, pTexture, pTexture->GetResourceState()).Failed())
    {
      ref_error = "The current view target does not match the presentation import contract.";
      return XII_FAILURE;
    }
  }

  const auto& display = ref_data.m_bUseViewDefaults ? pView->GetDisplayOutputSettings() : ref_data.m_AuthoredDisplay;
  if (!xiiDisplayOutputManager::IsValid(display))
  {
    ref_error = "Invalid display and exposure settings.";
    return XII_FAILURE;
  }
  const auto                                    hShader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/FinalBlit.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(hShader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
  {
    ref_error = "The presentation shader is unavailable.";
    return XII_FAILURE;
  }

  ref_data.m_Constants.OutputMode                             = display.m_OutputMode.GetValue();
  ref_data.m_Constants.ApplySRGBTransfer                      = xiiGALResourceFormat::IsSrgb(pTargetDescription->m_Format) ? 0U : 1U;
  ref_data.m_Constants.PaperWhiteNits                         = display.m_fPaperWhiteNits;
  ref_data.m_Constants.MaximumDisplayNits                     = display.m_fMaximumDisplayNits;
  ref_data.m_Size                                             = {static_cast<xiiUInt32>(viewport.width), static_cast<xiiUInt32>(viewport.height)};
  ref_data.m_vOffset                                          = {static_cast<xiiUInt32>(viewport.x), static_cast<xiiUInt32>(viewport.y)};
  ref_data.m_Pipeline.m_pPipelineResourceSignature            = shader->GetPipelineResourceSignature();
  ref_data.m_Pipeline.m_pVertexShader                         = shader->GetGALShader(xiiGALShaderType::Vertex);
  ref_data.m_Pipeline.m_pPixelShader                          = shader->GetGALShader(xiiGALShaderType::Pixel);
  ref_data.m_Pipeline.m_GraphicsPipeline.m_pBlendState        = shader->GetBlendState();
  ref_data.m_Pipeline.m_GraphicsPipeline.m_pRasterizerState   = shader->GetRasterizerState();
  ref_data.m_Pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
  ref_data.m_Pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
  return XII_SUCCESS;
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
  commands.SetViewport({float(data.m_vOffset.x), float(data.m_vOffset.y), float(data.m_Size.width), float(data.m_Size.height), 0, 1});
  commands.SetScissorRect({data.m_vOffset.x, data.m_vOffset.y, data.m_Size.width, data.m_Size.height});
  commands.SetPipelineState(pipeline.Borrow());
  commands.ResolveAndSetSampler("PointClampSampler", data.m_Sampler.Borrow());
  commands.ResolveAndSetConstantBuffer("xiiFinalBlitConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
  commands.ResolveAndSetShaderResourceTextureView("g_FinalColor", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
  commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
  commands.Draw({3U, 1U, 0U, 0U});
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_PresentPass);
