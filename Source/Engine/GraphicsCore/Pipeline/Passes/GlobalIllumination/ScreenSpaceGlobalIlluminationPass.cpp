/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenSpaceGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/SSGIConstants.h>

using namespace xiiRenderPassUtils;
struct xiiScreenSpaceGlobalIlluminationPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;   ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;   ///< ShaderResource in (surface albedo and material AO).
  xiiRenderGraphTextureHandle m_hGBufferMaterial; ///< ShaderResource in (surface metallic response).
  xiiRenderGraphTextureHandle m_hHDRIn;           ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hSSGIOut;         ///< UnorderedAccess out (screen-space diffuse GI contribution).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSSGIConstants            m_Constants;
  xiiSizeU32                  m_Size;
  xiiRenderGraphBufferHandle  m_hPerFrameCamera;
  xiiRenderGraphBufferHandle  m_hPerFrameGlobal;
};

struct xiiScreenSpaceGlobalIlluminationPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Point, m_Linear;
};

xiiScreenSpaceGlobalIlluminationPass::xiiScreenSpaceGlobalIlluminationPass()  = default;
xiiScreenSpaceGlobalIlluminationPass::~xiiScreenSpaceGlobalIlluminationPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceGlobalIlluminationPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceGlobalIlluminationPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RayLength", m_fRayLength)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(8U), new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.15f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
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

xiiResult xiiScreenSpaceGlobalIlluminationPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fRayLength) && m_fRayLength >= 0 && m_uiSampleCount > 0 && m_uiSampleCount <= 32 && xiiMath::IsFinite(m_fThickness) && m_fThickness >= 0.001f && xiiMath::IsFinite(m_fIntensity) && m_fIntensity >= 0 ? XII_SUCCESS : XII_FAILURE;
}

void xiiScreenSpaceGlobalIlluminationPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"PerFrameGlobal", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"SSGITerm", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  pins.PushBack({"xiiSSGIConstants", xiiRenderPipelinePin::Buffer, false, false});
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  pins.PushBack({"HardwareGI", xiiRenderPipelinePin::Texture, true, true});
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiScreenSpaceGlobalIlluminationPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  const float intensity = m_fRayLength == 0 || (m_bSkipWithHardwareGI && context.GetInputTexture("HardwareGI").IsValid()) ? 0.0f : m_fIntensity;
  if (intensity > 0)
  {
    XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SSGI.xiiShader"));
    XII_SUCCEED_OR_RETURN(PointClampSampler(m_pState->m_Point));
    XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  }
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("SceneDepth"), 0, 0, size));
  const auto queue   = intensity > 0 ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, queue, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSceneDepth = context.GetInputTexture("SceneDepth");
    data.m_hGBufferNormal = context.GetInputTexture("GBufferNormal");
    data.m_hGBufferAlbedo = context.GetInputTexture("GBufferAlbedo");
    data.m_hGBufferMaterial = context.GetInputTexture("GBufferMaterial");
    data.m_hHDRIn = context.GetInputTexture("HDRSceneColor");
    data.m_hPerFrameCamera = context.GetInputBuffer("PerFrameCamera");
    data.m_hPerFrameGlobal = context.GetInputBuffer("PerFrameGlobal");
    data.m_Constants.RayLength = m_fRayLength;
    data.m_Constants.SampleCount = m_uiSampleCount;
    data.m_Constants.Thickness = m_fThickness;
    data.m_Constants.Intensity = intensity;
    Setup(data, builder); }, xiiMakeDelegate(&xiiScreenSpaceGlobalIlluminationPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("SSGITerm", emitted.first->m_hSSGIOut));
  XII_SUCCEED_OR_RETURN(context.SetOutput("xiiSSGIConstants", emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput("SceneDepth", context.GetInputTexture("SceneDepth")));
  XII_SUCCEED_OR_RETURN(context.SetOutput("GBufferNormal", context.GetInputTexture("GBufferNormal")));
  XII_SUCCEED_OR_RETURN(context.SetOutput("GBufferAlbedo", context.GetInputTexture("GBufferAlbedo")));
  XII_SUCCEED_OR_RETURN(context.SetOutput("GBufferMaterial", context.GetInputTexture("GBufferMaterial")));
  return XII_SUCCESS;
}

void xiiScreenSpaceGlobalIlluminationPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  const xiiUInt32 uiRenderWidth  = data.m_Size.width;
  const xiiUInt32 uiRenderHeight = data.m_Size.height;

  data.m_hSceneDepth      = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(data.m_hGBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo   = builder.ReadTexture(data.m_hGBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(data.m_hGBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn           = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSSGIOut           = builder.WriteTexture("SSGITerm", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSGIConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);


  data.m_hPerFrameCamera = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hPerFrameGlobal = builder.ReadBuffer(data.m_hPerFrameGlobal, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiScreenSpaceGlobalIlluminationPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  {
    auto& commands = context.GetCommandList();
    {
      xiiGALMapHelper<xiiSSGIConstants> mapped(commands, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Constants;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = context.GetBuffer(data.m_hConstants);
    transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  }
  if (data.m_Constants.Intensity == 0)
  {
    auto& commands = context.GetCommandList();
    ClearRenderTargetPreservingState(commands, context.GetTexture(data.m_hSSGIOut)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    return;
  }
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = data.m_Size.width;
  const xiiUInt32    uiRenderHeight = data.m_Size.height;

  cmd.BeginDebugGroup("SSGI");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("PerFrameGlobal", context.GetBuffer(data.m_hPerFrameGlobal), xiiGALShaderType::Compute);
    cmd.ResolveAndSetSampler("g_sPointClamp", m_pState->m_Point.Borrow());
    cmd.ResolveAndSetSampler("g_sLinearClamp", m_pState->m_Linear.Borrow());
    cmd.ResolveAndSetSampler("PointClampSampler", m_pState->m_Point.Borrow());
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_Linear.Borrow());


    cmd.ResolveAndSetConstantBuffer("xiiSSGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSGIOut", context.GetTexture(data.m_hSSGIOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_ScreenSpaceGlobalIlluminationPass);
