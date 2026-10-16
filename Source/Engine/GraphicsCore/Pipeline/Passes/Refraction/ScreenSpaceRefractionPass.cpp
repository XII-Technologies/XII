/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenSpaceRefractionPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Refraction/SSRefractionConstants.h>

using namespace xiiRenderPassUtils;
struct xiiScreenSpaceRefractionPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;   ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hGBufferMaterial; ///< ShaderResource in (material feature flags).
  xiiRenderGraphTextureHandle m_hGBufferEmissive; ///< ShaderResource in (RGB emissive, A transmission factor).
  xiiRenderGraphTextureHandle m_hHDRIn;           ///< ShaderResource in (immutable pre-refraction HDR snapshot).
  xiiRenderGraphTextureHandle m_hHDROut;          ///< UnorderedAccess out (authoritative HDR scene color target).
  xiiRenderGraphBufferHandle  m_hConstants;       ///< ConstantBuffer in (refraction tuning parameters).
  xiiSSRefractionConstants    m_Constants;
  xiiSizeU32                  m_Size;
  xiiRenderGraphBufferHandle  m_hPerFrameCamera;
};

struct xiiScreenSpaceRefractionPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear;
};

xiiScreenSpaceRefractionPass::xiiScreenSpaceRefractionPass()  = default;
xiiScreenSpaceRefractionPass::~xiiScreenSpaceRefractionPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceRefractionPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceRefractionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PerturbScale", m_fPerturbScale)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, 0.5f)),
    XII_MEMBER_PROPERTY("MaximumDistance", m_fMaximumDistance)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0f, 0.5f)),
    XII_MEMBER_PROPERTY("Chromatic", m_fChromatic)->AddAttributes(new xiiDefaultValueAttribute(0.002f), new xiiClampValueAttribute(0.0f, 0.1f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Refraction")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiScreenSpaceRefractionPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fPerturbScale) && m_fPerturbScale >= 0 && m_fPerturbScale <= 0.5f && xiiMath::IsFinite(m_fMaximumDistance) && m_fMaximumDistance >= 0 && m_fMaximumDistance <= 0.5f && xiiMath::IsFinite(m_fChromatic) && m_fChromatic >= 0 && m_fChromatic <= 0.1f ? XII_SUCCESS : XII_FAILURE;
}

void xiiScreenSpaceRefractionPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, true, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"RefractionSceneColorInput", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UnorderedAccess;
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, true});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  pins.PushBack({"xiiSSRefractionConstants", xiiRenderPipelinePin::Buffer, false, false});
  pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiScreenSpaceRefractionPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SSRefraction.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("SceneDepth"), 0, 0, size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSceneDepth = context.GetInputTexture("SceneDepth");
    data.m_hGBufferNormal = context.GetInputTexture("GBufferNormal");
    data.m_hGBufferMaterial = context.GetInputTexture("GBufferMaterial");
    data.m_hGBufferEmissive = context.GetInputTexture("GBufferEmissive");
    data.m_hHDRIn = context.GetInputTexture("RefractionSceneColorInput");
    data.m_hHDROut = context.GetInputTexture("HDRSceneColor");
    data.m_hPerFrameCamera = context.GetInputBuffer("PerFrameCamera");
    data.m_Constants.PerturbScale = m_fPerturbScale;
    data.m_Constants.MaxDistance = m_fMaximumDistance;
    data.m_Constants.Chromatic = m_fChromatic;
    data.m_Constants.Padding = 0;
    Setup(data, builder); }, xiiMakeDelegate(&xiiScreenSpaceRefractionPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("HDRSceneColor", emitted.first->m_hHDROut));
  XII_SUCCEED_OR_RETURN(context.SetOutput("xiiSSRefractionConstants", emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput("GBufferEmissive", context.GetInputTexture("GBufferEmissive")));
  return XII_SUCCESS;
}

void xiiScreenSpaceRefractionPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(data.m_hGBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(data.m_hGBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive = builder.ReadTexture(data.m_hGBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn           = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDROut          = builder.WriteTexture(builder.ReadTexture(data.m_hHDROut, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRefractionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRefractionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);


  builder.SetPassAllowMerge(false);
  data.m_hPerFrameCamera = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiScreenSpaceRefractionPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = data.m_Size.width;
  const xiiUInt32    uiRenderHeight = data.m_Size.height;

  cmd.BeginDebugGroup("SSRefraction");
  {
    {
      xiiGALMapHelper<xiiSSRefractionConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }

    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);
    cmd.ResolveAndSetSampler("g_sLinearClamp", m_pState->m_Linear.Borrow());
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_Linear.Borrow());


    cmd.ResolveAndSetConstantBuffer("xiiSSRefractionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDROut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Refraction_ScreenSpaceRefractionPass);
