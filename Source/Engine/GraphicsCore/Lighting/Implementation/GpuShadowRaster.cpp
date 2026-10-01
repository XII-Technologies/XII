/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/GpuShadowRaster.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/LocalShadow/GpuShadowRasterConstants.h>

class xiiGpuShadowRasterManagerState
{
public:
  xiiShaderPermutationResourceHandle m_hShaderPermutation;
  xiiShaderPermutationResourceHandle m_hClearShaderPermutation;
  bool                               m_bEngineStarted        = false;
  bool                               m_bMeshShadersSupported = false;
};

xiiUniquePtr<xiiGpuShadowRasterManagerState> xiiGpuShadowRasterManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GpuShadowRasterManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable",
    "GeometryResidencyManager",
    "PipelineCache",
    "ShaderPermutationUtilities"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGpuShadowRasterManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGpuShadowRasterManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGpuShadowRasterManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGpuShadowRasterManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuShadowRasterDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuShadowRasterDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
    XII_MEMBER_PROPERTY("Viewport", m_Viewport),
    XII_MEMBER_PROPERTY("VertexStride", m_uiVertexStride),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountX", m_uiMeshDispatchGroupCountX),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountY", m_uiMeshDispatchGroupCountY),
    XII_MEMBER_PROPERTY("ClearViewport", m_bClearViewport),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct ShadowRasterConstantsPassData
  {
    xiiRenderGraphBufferHandle    m_hConstants;
    xiiGpuShadowRasterDescription m_Description;
    xiiUInt32                     m_uiGeometryBaseIndex = 0U;
  };

  struct ShadowRasterPassData
  {
    xiiRenderGraphTextureHandle        m_hDepthAtlas;
    xiiRenderGraphBufferHandle         m_hConstants;
    xiiRenderGraphBufferHandle         m_hSceneInstances;
    xiiRenderGraphBufferHandle         m_hGeometry;
    xiiRenderGraphBufferHandle         m_hMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle         m_hIndirectCommands;
    xiiRenderGraphBufferHandle         m_hIndirectCommandCount;
    xiiGpuShadowRasterDescription      m_Description;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
    xiiShaderPermutationResourceHandle m_hClearShaderPermutation;
  };
} // namespace

bool xiiGpuShadowRasterManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGpuShadowRasterManager::IsSupported()
{
  return IsInitialized() && s_pState->m_bMeshShadersSupported && s_pState->m_hShaderPermutation.IsValid() && s_pState->m_hClearShaderPermutation.IsValid();
}

bool xiiGpuShadowRasterManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted;
}

xiiRenderGraphTextureHandle xiiGpuShadowRasterManager::AddPass(xiiRenderGraph& graph, xiiStringView sName,
                                                               xiiRenderGraphTextureHandle hDepthAtlas, const xiiGpuVisibilityOutputs& visibility,
                                                               const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuShadowRasterDescription& description)
{
  if (!IsSupported() || !hDepthAtlas.IsValid() || !visibility.m_hSceneInstances.IsValid() || !visibility.m_hVisibleMeshlets.IsValid() ||
      !visibility.m_hVisibleMeshletCount.IsValid() || !visibility.m_hIndirectCommands.IsValid() || !visibility.m_hIndirectCommandCount.IsValid() ||
      !geometry.m_hGeometryMetadata.IsValid() || !geometry.m_hMeshletMetadata.IsValid() || description.m_Viewport.z == 0U ||
      description.m_Viewport.w == 0U || description.m_uiVertexStride == 0U)
    return hDepthAtlas;

  xiiStringBuilder constantsPassName(sName, " Constants");
  xiiStringBuilder constantsResourceName(sName, " Constants Buffer");
  auto             constantsPass = graph.AddPass<ShadowRasterConstantsPassData>(
    constantsPassName, xiiGALCommandQueueFlags::Graphics,
    [constantsResourceName](ShadowRasterConstantsPassData& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiGpuShadowRasterConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer(constantsResourceName, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
    },
    [](const ShadowRasterConstantsPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALMapHelper<xiiGpuShadowRasterConstants> constants(context.GetCommandList(), context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->ViewProjectionMatrix    = data.m_Description.m_ViewProjectionMatrix;
      constants->GeometryBaseIndex       = data.m_uiGeometryBaseIndex;
      constants->VertexStride            = data.m_Description.m_uiVertexStride;
      constants->MeshDispatchGroupCountX = data.m_Description.m_uiMeshDispatchGroupCountX;
      constants->MeshDispatchGroupCountY = data.m_Description.m_uiMeshDispatchGroupCountY;
    });
  constantsPass.first->m_Description         = description;
  constantsPass.first->m_uiGeometryBaseIndex = geometry.m_uiGeometryBaseIndex;

  auto pass = graph.AddPass<ShadowRasterPassData>(
    sName, xiiGALCommandQueueFlags::Graphics,
    [hDepthAtlas, visibility, geometry, hConstants = constantsPass.first->m_hConstants](ShadowRasterPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hDepthAtlas           = builder.WriteTexture(hDepthAtlas, xiiGALResourceStateFlags::DepthWrite);
      data.m_hConstants            = builder.ReadBuffer(hConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_hSceneInstances       = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry             = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshlets             = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshlets      = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshletCount  = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hIndirectCommands     = builder.ReadBuffer(visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hIndirectCommandCount = builder.ReadBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);

      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged(true);
    },
    [](const ShadowRasterPassData& data, xiiRenderGraphPassContext& context) {
      xiiResourceLock<xiiShaderPermutationResource> permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      xiiResourceLock<xiiShaderPermutationResource> clearPermutation(data.m_hClearShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      if (!permutation.IsValid() || !permutation->IsShaderValid() || !clearPermutation.IsValid() || !clearPermutation->IsShaderValid() || context.GetRenderPass() == nullptr)
        return;

      xiiGALGraphicsPipelineStateCreationDescription pipelineDescription;
      pipelineDescription.m_PipelineType                          = xiiGALPipelineType::Mesh;
      pipelineDescription.m_pPipelineResourceSignature            = permutation->GetPipelineResourceSignature();
      pipelineDescription.m_pMeshShader                           = permutation->GetGALShader(xiiGALShaderType::Mesh);
      pipelineDescription.m_pPixelShader                          = permutation->GetGALShader(xiiGALShaderType::Pixel);
      pipelineDescription.m_GraphicsPipeline.m_pBlendState        = permutation->GetBlendState();
      pipelineDescription.m_GraphicsPipeline.m_pRasterizerState   = permutation->GetRasterizerState();
      pipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = permutation->GetDepthStencilState();
      pipelineDescription.m_GraphicsPipeline.m_pRenderPass        = context.GetRenderPass();
      pipelineDescription.m_GraphicsPipeline.m_uiSubpassIndex     = static_cast<xiiUInt8>(context.GetSubpassIndex());
      pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
      const xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline   = xiiGALPipelineCache::GetPipeline(pipelineDescription);
      if (pPipeline == nullptr)
        return;

      xiiGALCommandList& cmd      = context.GetCommandList();
      const xiiVec4U32&  viewport = data.m_Description.m_Viewport;
      const xiiRectU32   scissor(viewport.x, viewport.y, viewport.z, viewport.w);
      cmd.SetViewport({static_cast<float>(viewport.x), static_cast<float>(viewport.y), static_cast<float>(viewport.z), static_cast<float>(viewport.w), 0.0f, 1.0f});
      cmd.SetScissorRect(scissor);

      if (data.m_Description.m_bClearViewport)
      {
        xiiGALGraphicsPipelineStateCreationDescription clearPipelineDescription;
        clearPipelineDescription.m_PipelineType                          = xiiGALPipelineType::Graphics;
        clearPipelineDescription.m_pPipelineResourceSignature            = clearPermutation->GetPipelineResourceSignature();
        clearPipelineDescription.m_pVertexShader                          = clearPermutation->GetGALShader(xiiGALShaderType::Vertex);
        clearPipelineDescription.m_pPixelShader                           = clearPermutation->GetGALShader(xiiGALShaderType::Pixel);
        clearPipelineDescription.m_GraphicsPipeline.m_pBlendState        = clearPermutation->GetBlendState();
        clearPipelineDescription.m_GraphicsPipeline.m_pRasterizerState   = clearPermutation->GetRasterizerState();
        clearPipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = clearPermutation->GetDepthStencilState();
        clearPipelineDescription.m_GraphicsPipeline.m_pRenderPass        = context.GetRenderPass();
        clearPipelineDescription.m_GraphicsPipeline.m_uiSubpassIndex     = static_cast<xiiUInt8>(context.GetSubpassIndex());
        clearPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
        const xiiSharedPtr<xiiGALGraphicsPipelineState> pClearPipeline   = xiiGALPipelineCache::GetPipeline(clearPipelineDescription);
        if (pClearPipeline == nullptr)
          return;

        cmd.SetPipelineState(pClearPipeline.Borrow());
        cmd.Draw({3U});
      }

      cmd.SetPipelineState(pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiGpuShadowRasterConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      if (xiiGALBindlessResourceTable::IsInitialized())
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_Buffers", xiiGALShaderType::Mesh);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
      cmd.DrawMeshIndirect({context.GetBuffer(data.m_hIndirectCommands), 1U, 0U, xiiGALStateTransitionMode::None, context.GetBuffer(data.m_hIndirectCommandCount)});
    });

  pass.first->m_Description             = description;
  pass.first->m_hShaderPermutation      = s_pState->m_hShaderPermutation;
  pass.first->m_hClearShaderPermutation = s_pState->m_hClearShaderPermutation;
  return pass.first->m_hDepthAtlas;
}

void xiiGpuShadowRasterManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "GPU shadow raster manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiGpuShadowRasterManagerState);
}

void xiiGpuShadowRasterManager::EngineStartup()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted                = true;
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  s_pState->m_bMeshShadersSupported        = pDevice != nullptr && pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled;
  if (!s_pState->m_bMeshShadersSupported)
    return;

  const xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/GpuShadowDepth.xiiShader");
  s_pState->m_hShaderPermutation        = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, {}, true);
  const xiiShaderResourceHandle hClearShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/GpuShadowPageClear.xiiShader");
  s_pState->m_hClearShaderPermutation        = xiiShaderPermutationUtilities::PreloadSinglePermutation(hClearShader, {}, true);
}

void xiiGpuShadowRasterManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_hShaderPermutation.Invalidate();
  s_pState->m_hClearShaderPermutation.Invalidate();
  s_pState->m_bMeshShadersSupported = false;
  s_pState->m_bEngineStarted        = false;
}

void xiiGpuShadowRasterManager::Shutdown()
{
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_GpuShadowRaster);
