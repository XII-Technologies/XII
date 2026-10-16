/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>

using namespace xiiRenderPassUtils;
struct xiiAtmosphereCompositePass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;                 ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hAtmosphereTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphTextureHandle m_hAtmosphereMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;              ///< UnorderedAccess in/out (sky and aerial perspective composite).
  xiiRenderGraphBufferHandle  m_hConstants;                  ///< Physical atmosphere parameters.
  xiiSizeU32                  m_Size;
  xiiRenderGraphBufferHandle  m_hPerFrameCamera;
  xiiRenderGraphBufferHandle  m_hPerFrameLight;
};

struct xiiAtmosphereCompositePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear;
};

xiiAtmosphereCompositePass::xiiAtmosphereCompositePass()  = default;
xiiAtmosphereCompositePass::~xiiAtmosphereCompositePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereCompositePass, 1, xiiRTTIDefaultAllocator<xiiAtmosphereCompositePass>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Atmosphere")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void xiiAtmosphereCompositePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UnorderedAccess;
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"PerFrameLight", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"AtmosphereConstants", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiAtmosphereCompositePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/AtmosphereComposite.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("SceneDepth"), 0, 0, size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSceneDepth = context.GetInputTexture("SceneDepth");
    data.m_hAtmosphereTransmittanceLUT = context.GetInputTexture("AtmTransmittance");
    data.m_hAtmosphereMultiScatterLUT = context.GetInputTexture("AtmMultiScatter");
    data.m_hHDRSceneColor = context.GetInputTexture("HDRSceneColor");
    data.m_hPerFrameCamera = context.GetInputBuffer("PerFrameCamera");
    data.m_hPerFrameLight = context.GetInputBuffer("PerFrameLight");
    data.m_hConstants = context.GetInputBuffer("AtmosphereConstants");
    Setup(data, builder); }, xiiMakeDelegate(&xiiAtmosphereCompositePass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("HDRSceneColor", emitted.first->m_hHDRSceneColor));
  XII_SUCCEED_OR_RETURN(context.SetOutput("AtmTransmittance", context.GetInputTexture("AtmTransmittance")));
  XII_SUCCEED_OR_RETURN(context.SetOutput("AtmMultiScatter", context.GetInputTexture("AtmMultiScatter")));
  return XII_SUCCESS;
}

void xiiAtmosphereCompositePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth                 = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(data.m_hAtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(data.m_hAtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor              = builder.WriteTexture(builder.ReadTexture(data.m_hHDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants                  = builder.ReadBuffer(data.m_hConstants, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_hPerFrameCamera = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hPerFrameLight  = builder.ReadBuffer(data.m_hPerFrameLight, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiAtmosphereCompositePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereComposite");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("PerFrameLight", context.GetBuffer(data.m_hPerFrameLight), xiiGALShaderType::Compute);
    cmd.ResolveAndSetSampler("g_sLinearClamp", m_pState->m_Linear.Borrow());
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_Linear.Borrow());


    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hAtmosphereTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hAtmosphereMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Atmosphere_AtmosphereCompositePass);
