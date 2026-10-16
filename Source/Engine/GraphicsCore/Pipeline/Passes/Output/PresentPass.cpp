/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/FinalBlitConstants.h>

using namespace xiiRenderPassUtils;
struct xiiPresentPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDisplayLinear; ///< ShaderResource in (tone-mapped scene-linear display signal).
  xiiRenderGraphTextureHandle m_hBackbuffer;    ///< RenderTarget out (swapchain backbuffer).
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bApplySRGBTransfer = false;
  xiiDisplayOutputSettings    m_DisplaySettings;
  xiiSizeU32                  m_Size;
};

struct xiiPresentPass::State
{
  xiiSharedPtr<xiiGALGraphicsPipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>               m_PointSampler;
  DisplayConfiguration                      m_Display;
};

xiiPresentPass::xiiPresentPass()  = default;
xiiPresentPass::~xiiPresentPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPresentPass, 1, xiiRTTIDefaultAllocator<xiiPresentPass>)
{
  XII_BEGIN_PROPERTIES
  {
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
void xiiPresentPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"GradedColor", xiiRenderPipelinePin::Texture, true, false});
  pins.PushBack({"Final Blit Constants", xiiRenderPipelinePin::Buffer, false, true});
  pins.PushBack({"GradedColor", xiiRenderPipelinePin::Texture, false, true});
  pins.PushBack({"ViewOutput", xiiRenderPipelinePin::Texture, false, true});
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
      if (pin.m_bInput)
        pin.m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
    }
}
xiiResult xiiPresentPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  if (!context.m_pView)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(PointClampSampler(m_pState->m_PointSampler));
  xiiDisplayOutputSettings authored;
  authored.m_OutputMode          = m_OutputMode;
  authored.m_fPaperWhiteNits     = m_fPaperWhiteNits;
  authored.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, authored, m_bUseViewDefaults));
  if (!context.m_Graph.GetTextureDescription(context.GetInputTexture("GradedColor")))
    return XII_FAILURE;
  const auto                        target    = context.m_pView->GetRenderTargetView();
  const auto                        swapchain = context.m_pView->GetSwapChain();
  const xiiSharedPtr<xiiGALTexture> texture   = target ? target->GetTexture() : swapchain ? swapchain->GetBackBufferTexture() :
                                                                                            xiiSharedPtr<xiiGALTexture>();
  if (!texture)
    return XII_FAILURE;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hDisplayLinear = context.GetInputTexture("GradedColor");
    data.m_DisplaySettings = m_pState->m_Display.m_Settings;
    data.m_Size = texture->GetDescription().m_Size;
    data.m_hBackbuffer = builder.ImportTexture(swapchain ? "Backbuffer" : "ViewOutput", texture, texture->GetResourceState());
    data.m_hBackbuffer = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
    data.m_bApplySRGBTransfer = !xiiGALResourceFormat::IsSrgb(texture->GetDescription().m_Format);
    if (!swapchain) builder.ExportTexture(data.m_hBackbuffer, texture->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ? xiiGALResourceStateFlags::ShaderResource : xiiGALResourceStateFlags::RenderTarget);
    Setup(data, builder); }, xiiMakeDelegate(&xiiPresentPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("Final Blit Constants", emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput("GradedColor", context.GetInputTexture("GradedColor")));
  XII_SUCCEED_OR_RETURN(context.SetOutput("ViewOutput", emitted.first->m_hBackbuffer));
  return XII_SUCCESS;
}

void xiiPresentPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDisplayLinear = builder.ReadTexture(data.m_hDisplayLinear, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiFinalBlitConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Final Blit Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
  builder.SetPassRenderPassManaged(data.m_hBackbuffer.IsValid());
  builder.SetPassPreparation([&data](xiiRenderGraphPassContext& context) {
    auto& cmd = context.GetCommandList();
    {
      xiiGALMapHelper<xiiFinalBlitConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->OutputMode         = data.m_DisplaySettings.m_OutputMode.GetValue();
      pConstants->ApplySRGBTransfer  = data.m_bApplySRGBTransfer ? 1U : 0U;
      pConstants->PaperWhiteNits     = data.m_DisplaySettings.m_fPaperWhiteNits;
      pConstants->MaximumDisplayNits = data.m_DisplaySettings.m_fMaximumDisplayNits;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = context.GetBuffer(data.m_hConstants);
    transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  });
}

void xiiPresentPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList&                        cmd = context.GetCommandList();
  xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline;
  if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && context.GetRenderPass() != nullptr)
  {
    pPipeline = LoadGraphicsPipeline(m_pState->m_Pipeline, "Shaders/Pipeline/FinalBlit.xiiShader", context.GetRenderPass(), context.GetSubpassIndex());
  }

  cmd.BeginDebugGroup("BackbufferPresent");
  {
    if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && pPipeline != nullptr)
    {


      ClearRenderTargetPreservingState(cmd, context.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(data.m_Size.width), static_cast<float>(data.m_Size.height), 0.0f, 1.0f});
      cmd.SetPipelineState(pPipeline.Borrow());
      cmd.ResolveAndSetSampler("PointClampSampler", m_pState->m_PointSampler.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiFinalBlitConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_FinalColor", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).IgnoreResult();
      cmd.Draw({3U, 1U, 0U, 0U});
    }
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_PresentPass);
