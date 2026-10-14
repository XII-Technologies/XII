/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>
#include <Shaders/Pipeline/Passes/Output/ViewRenderModeConstants.h>

using namespace xiiBuiltinPipelineUtils;

// The shader dispatches on the serialized public mode values.
static_assert(xiiViewRenderMode::Wireframe == 1 && xiiViewRenderMode::InstanceID == 14 && xiiViewRenderMode::BaseColor == 19 && xiiViewRenderMode::Metallic == 20 &&
              xiiViewRenderMode::Roughness == 21 && xiiViewRenderMode::Specular == 22 && xiiViewRenderMode::Emissive == 23 &&
              xiiViewRenderMode::AmbientOcclusion == 24 && xiiViewRenderMode::NormalMap == 25 && xiiViewRenderMode::DepthLinear == 26 &&
              xiiViewRenderMode::DepthNonLinear == 27 && xiiViewRenderMode::MotionVectors == 28 &&
              xiiViewRenderMode::IndirectLighting == 36 && xiiViewRenderMode::ReflectionContribution == 37 && xiiViewRenderMode::Bloom == 44);

namespace
{
  struct ViewRenderModeData
  {
    xiiRenderGraphTextureHandle m_hSource, m_hDepth, m_hObjectIds, m_hOutput;
    xiiRenderGraphBufferHandle  m_hConstants;
  };
} // namespace

xiiResult xiiBuiltinRenderPipelineRuntime::BuildViewRenderMode(xiiRenderGraph& graph)
{
  const auto mode = static_cast<xiiViewRenderMode::Enum>(m_View.GetViewRenderMode().GetValue());
  if (mode == xiiViewRenderMode::Default)
    return XII_SUCCESS;
  if (!xiiViewRenderMode::IsSupportedByDefaultPipeline(mode))
  {
    xiiLog::Warning("Render mode {} is unavailable in the built-in pipeline; restoring the default view.", static_cast<xiiUInt32>(mode));
    m_View.SetViewRenderMode(xiiViewRenderMode::Default);
    return XII_SUCCESS;
  }

  xiiStringView source = xiiRGBlackboardKeys::k_SceneDepthTexture;
  switch (mode)
  {
    case xiiViewRenderMode::BaseColor:
      source = xiiRGBlackboardKeys::k_GBufferAlbedo;
      break;
    case xiiViewRenderMode::NormalMap:
      source = xiiRGBlackboardKeys::k_GBufferNormal;
      break;
    case xiiViewRenderMode::Metallic:
    case xiiViewRenderMode::Roughness:
    case xiiViewRenderMode::Specular:
      source = xiiRGBlackboardKeys::k_GBufferMaterial;
      break;
    case xiiViewRenderMode::Emissive:
      source = xiiRGBlackboardKeys::k_GBufferEmissive;
      break;
    case xiiViewRenderMode::AmbientOcclusion:
      source = xiiRGBlackboardKeys::k_StableAOTexture;
      break;
    case xiiViewRenderMode::MotionVectors:
      source = xiiRGBlackboardKeys::k_DilatedVelocityBuffer;
      break;
    case xiiViewRenderMode::IndirectLighting:
      source = xiiRGBlackboardKeys::k_IndirectLightingBuffer;
      break;
    case xiiViewRenderMode::ReflectionContribution:
      source = xiiRGBlackboardKeys::k_EnvironmentSpecular;
      break;
    case xiiViewRenderMode::Bloom:
      source = xiiRGBlackboardKeys::k_BloomTexture;
      break;
    default:
      break;
  }

  if (!EnsureComputePipeline(m_Resources.m_PostProcessPasses.m_pViewRenderModePipeline, "Shaders/Pipeline/ViewRenderMode.xiiShader"))
    return XII_FAILURE;
  xiiRenderGraphTextureHandle wireframeSurface;
  if (mode == xiiViewRenderMode::Wireframe)
  {
    auto description       = m_Frame.gpuRasterDescription;
    description.m_uiWidth  = static_cast<xiiUInt32>(m_View.GetViewport().width);
    description.m_uiHeight = static_cast<xiiUInt32>(m_View.GetViewport().height);
    XII_SUCCEED_OR_RETURN(xiiGpuSceneRasterManager::AddPickingPass(graph, "Wireframe surface", m_Frame.mainVisibility, m_Frame.geometry, m_Frame.materials, description, wireframeSurface, true, xiiGALFillMode::Wireframe));
  }
  auto setup = [source, wireframeSurface](ViewRenderModeData& data, xiiRenderGraphBuilder& builder) {
    data.m_hSource    = builder.ReadTexture(source, xiiGALResourceStateFlags::ShaderResource);
    data.m_hDepth     = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
    data.m_hObjectIds = wireframeSurface.IsValid() ? builder.ReadTexture(wireframeSurface, xiiGALResourceStateFlags::ShaderResource) :
                                                     builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferObjectId, xiiGALResourceStateFlags::ShaderResource);
    data.m_hOutput    = builder.ReadWriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
    xiiGALBufferCreationDescription constants;
    constants.m_uiSize         = sizeof(xiiViewRenderModeConstants);
    constants.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
    constants.m_Usage          = xiiGALResourceUsage::Dynamic;
    constants.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    data.m_hConstants          = builder.WriteBuffer("ViewRenderModeConstants", constants, xiiGALResourceStateFlags::ConstantBuffer);
  };
  auto execute = [this, mode](const ViewRenderModeData& data, xiiRenderGraphPassContext& context) {
    auto& cmd = context.GetCommandList();
    {
      xiiGALMapHelper<xiiViewRenderModeConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->RenderMode = mode;
      constants->_Padding   = xiiVec3::MakeZero();
    }
    cmd.SetPipelineState(m_Resources.m_PostProcessPasses.m_pViewRenderModePipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiViewRenderModeConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Source", context.GetTexture(data.m_hSource)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Depth", context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ObjectIds", context.GetTexture(data.m_hObjectIds)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hOutput)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Succeeded())
      cmd.DispatchCompute({(static_cast<xiiUInt32>(m_View.GetViewport().width) + 7) / 8, (static_cast<xiiUInt32>(m_View.GetViewport().height) + 7) / 8, 1});
  };
  graph.AddPass<ViewRenderModeData>("ViewRenderMode", xiiGALCommandQueueFlags::Compute, setup, execute);
  // The rest of this built-in stage must consume the visualization, rather than
  // the original pin binding supplied by ToneMapping.
  graph.SetSetupBinding(xiiRGBlackboardKeys::k_DisplayLinearColor, graph.FindTexture(xiiRGBlackboardKeys::k_DisplayLinearColor));
  return XII_SUCCESS;
}
