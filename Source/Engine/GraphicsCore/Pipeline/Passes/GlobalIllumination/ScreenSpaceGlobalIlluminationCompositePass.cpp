/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenSpaceGlobalIlluminationCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

using namespace xiiRenderPassUtils;
struct xiiScreenSpaceGlobalIlluminationCompositePass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSSGI;
  xiiRenderGraphTextureHandle m_hHDRScene;
  xiiSizeU32                  m_Size;
};

struct xiiScreenSpaceGlobalIlluminationCompositePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};

xiiScreenSpaceGlobalIlluminationCompositePass::xiiScreenSpaceGlobalIlluminationCompositePass()  = default;
xiiScreenSpaceGlobalIlluminationCompositePass::~xiiScreenSpaceGlobalIlluminationCompositePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceGlobalIlluminationCompositePass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceGlobalIlluminationCompositePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("SkipWithHardwareGI", m_bSkipWithHardwareGI)->AddAttributes(new xiiDefaultValueAttribute(false)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/GlobalIllumination")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiScreenSpaceGlobalIlluminationCompositePass::Configure(const xiiVariantDictionary& properties) { return xiiRenderPipelinePass::Configure(properties); }

void xiiScreenSpaceGlobalIlluminationCompositePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"SSGITerm", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UnorderedAccess;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  pins.PushBack({"HardwareGI", xiiRenderPipelinePin::Texture, true, true});
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiScreenSpaceGlobalIlluminationCompositePass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled || (m_bSkipWithHardwareGI && context.GetInputTexture("HardwareGI").IsValid()))
    return context.SetOutput("HDRSceneColor", context.GetInputTexture("HDRSceneColor"));
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SSGIComposite.xiiShader"));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("SSGITerm"), 0, 0, size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSSGI = context.GetInputTexture("SSGITerm");
    data.m_hHDRScene = context.GetInputTexture("HDRSceneColor");
    Setup(data, builder); }, xiiMakeDelegate(&xiiScreenSpaceGlobalIlluminationCompositePass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("HDRSceneColor", emitted.first->m_hHDRScene));
  return XII_SUCCESS;
}

void xiiScreenSpaceGlobalIlluminationCompositePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);
  data.m_hSSGI     = builder.ReadTexture(data.m_hSSGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRScene = builder.WriteTexture(builder.ReadTexture(data.m_hHDRScene, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
}

void xiiScreenSpaceGlobalIlluminationCompositePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SSGIComposite");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);


    cmd.ResolveAndSetShaderResourceTextureView("g_SSGI", context.GetTexture(data.m_hSSGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRScene)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_ScreenSpaceGlobalIlluminationCompositePass);
