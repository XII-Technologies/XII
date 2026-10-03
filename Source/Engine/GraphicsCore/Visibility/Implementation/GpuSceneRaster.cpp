/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneRasterConstants.h>

class xiiGpuSceneRasterManagerState
{
public:
  xiiShaderPermutationResourceHandle m_hDepthShaderPermutation;
  xiiShaderPermutationResourceHandle m_hGBufferShaderPermutation;
  bool                               m_bEngineStarted        = false;
  bool                               m_bMeshShadersSupported = false;
};

xiiUniquePtr<xiiGpuSceneRasterManagerState> xiiGpuSceneRasterManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GpuSceneRasterManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable",
    "GeometryResidencyManager",
    "GpuVisibilityManager",
    "PipelineCache",
    "ShaderPermutationUtilities"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGpuSceneRasterManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGpuSceneRasterManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGpuSceneRasterManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGpuSceneRasterManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSceneDepthRasterDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuSceneDepthRasterDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
    XII_MEMBER_PROPERTY("Width", m_uiWidth),
    XII_MEMBER_PROPERTY("Height", m_uiHeight),
    XII_MEMBER_PROPERTY("VertexStride", m_uiVertexStride),
    XII_MEMBER_PROPERTY("TexCoordOffset", m_uiTexCoordOffset),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountX", m_uiMeshDispatchGroupCountX),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountY", m_uiMeshDispatchGroupCountY),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSceneGBufferRasterDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuSceneGBufferRasterDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
    XII_MEMBER_PROPERTY("PreviousViewProjectionMatrix", m_PreviousViewProjectionMatrix),
    XII_MEMBER_PROPERTY("CurrentJitter", m_vCurrentJitter),
    XII_MEMBER_PROPERTY("PreviousJitter", m_vPreviousJitter),
    XII_MEMBER_PROPERTY("Width", m_uiWidth),
    XII_MEMBER_PROPERTY("Height", m_uiHeight),
    XII_MEMBER_PROPERTY("VertexStride", m_uiVertexStride),
    XII_MEMBER_PROPERTY("NormalOffset", m_uiNormalOffset),
    XII_MEMBER_PROPERTY("TangentOffset", m_uiTangentOffset),
    XII_MEMBER_PROPERTY("TexCoordOffset", m_uiTexCoordOffset),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountX", m_uiMeshDispatchGroupCountX),
    XII_MEMBER_PROPERTY("MeshDispatchGroupCountY", m_uiMeshDispatchGroupCountY),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct DepthConstantsPassData
  {
    xiiRenderGraphBufferHandle         m_hConstants;
    xiiGpuSceneDepthRasterDescription m_Description;
    xiiUInt32                          m_uiGeometryBaseIndex = 0U;
    xiiUInt32                          m_uiMaterialBaseIndex = 0U;
  };

  struct DepthRasterPassData
  {
    xiiRenderGraphTextureHandle        m_hDepth;
    xiiRenderGraphBufferHandle         m_hConstants;
    xiiRenderGraphBufferHandle         m_hSceneInstances;
    xiiRenderGraphBufferHandle         m_hGeometry;
    xiiRenderGraphBufferHandle         m_hMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle         m_hIndirectCommands;
    xiiRenderGraphBufferHandle         m_hIndirectCommandCount;
    xiiRenderGraphBufferHandle         m_hSurfaceMaterials;
    xiiGpuSceneDepthRasterDescription m_Description;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
  };

  struct GBufferConstantsPassData
  {
    xiiRenderGraphBufferHandle           m_hConstants;
    xiiGpuSceneGBufferRasterDescription m_Description;
    xiiUInt32                            m_uiGeometryBaseIndex = 0U;
    xiiUInt32                            m_uiMaterialBaseIndex = 0U;
  };

  struct GBufferRasterPassData
  {
    xiiRenderGraphTextureHandle          m_hDepth;
    xiiRenderGraphTextureHandle          m_hAlbedo;
    xiiRenderGraphTextureHandle          m_hNormal;
    xiiRenderGraphTextureHandle          m_hMaterial;
    xiiRenderGraphTextureHandle          m_hEmissive;
    xiiRenderGraphTextureHandle          m_hNormalRoughness;
    xiiRenderGraphTextureHandle          m_hVelocity;
    xiiRenderGraphBufferHandle           m_hConstants;
    xiiRenderGraphBufferHandle           m_hSceneInstances;
    xiiRenderGraphBufferHandle           m_hGeometry;
    xiiRenderGraphBufferHandle           m_hMeshlets;
    xiiRenderGraphBufferHandle           m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle           m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle           m_hIndirectCommands;
    xiiRenderGraphBufferHandle           m_hIndirectCommandCount;
    xiiRenderGraphBufferHandle           m_hSurfaceMaterials;
    xiiGpuSceneGBufferRasterDescription m_Description;
    xiiShaderPermutationResourceHandle   m_hShaderPermutation;
  };
} // namespace

bool xiiGpuSceneRasterManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGpuSceneRasterManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted;
}

bool xiiGpuSceneRasterManager::IsSupported()
{
  return IsInitialized() && s_pState->m_bMeshShadersSupported && s_pState->m_hDepthShaderPermutation.IsValid() &&
         s_pState->m_hGBufferShaderPermutation.IsValid();
}

xiiRenderGraphTextureHandle xiiGpuSceneRasterManager::AddDepthPrepass(xiiRenderGraph& graph, xiiStringView sName,
                                                                      const xiiGpuVisibilityOutputs& visibility,
                                                                      const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                                      const xiiMaterialGpuStorage::UploadHandles& materials,
                                                                      const xiiGpuSceneDepthRasterDescription& description)
{
  if (!IsSupported() || !visibility.m_hSceneInstances.IsValid() || !visibility.m_hVisibleMeshlets.IsValid() ||
      !visibility.m_hVisibleMeshletCount.IsValid() || !visibility.m_hIndirectCommands.IsValid() ||
      !visibility.m_hIndirectCommandCount.IsValid() || !geometry.m_hGeometryMetadata.IsValid() ||
      !geometry.m_hMeshletMetadata.IsValid() || !materials.m_hSurfaceData.IsValid() || description.m_uiWidth == 0U || description.m_uiHeight == 0U ||
      description.m_uiVertexStride == 0U)
    return {};

  xiiStringBuilder constantsPassName(sName, " Constants");
  xiiStringBuilder constantsResourceName(sName, " Constants Buffer");
  auto             constantsPass = graph.AddPass<DepthConstantsPassData>(
    constantsPassName, xiiGALCommandQueueFlags::Graphics,
    [constantsResourceName](DepthConstantsPassData& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiGpuSceneRasterConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer(constantsResourceName, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
    },
    [](const DepthConstantsPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALMapHelper<xiiGpuSceneRasterConstants> constants(context.GetCommandList(), context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->ViewProjectionMatrix    = data.m_Description.m_ViewProjectionMatrix;
      constants->GeometryBaseIndex       = data.m_uiGeometryBaseIndex;
      constants->MaterialBaseIndex       = data.m_uiMaterialBaseIndex;
      constants->VertexStride            = data.m_Description.m_uiVertexStride;
      constants->TexCoordOffset          = data.m_Description.m_uiTexCoordOffset;
      constants->MeshDispatchGroupCountX = data.m_Description.m_uiMeshDispatchGroupCountX;
      constants->MeshDispatchGroupCountY = data.m_Description.m_uiMeshDispatchGroupCountY;
    });
  constantsPass.first->m_Description         = description;
  constantsPass.first->m_uiGeometryBaseIndex = geometry.m_uiGeometryBaseIndex;
  constantsPass.first->m_uiMaterialBaseIndex = materials.m_uiSurfaceBaseIndex;

  auto pass = graph.AddPass<DepthRasterPassData>(
    sName, xiiGALCommandQueueFlags::Graphics,
    [visibility, geometry, materials, hConstants = constantsPass.first->m_hConstants, description](DepthRasterPassData& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription depthDescription;
      depthDescription.m_Type        = xiiGALResourceDimension::Texture2D;
      depthDescription.m_Format      = xiiGALResourceFormat::D32Float;
      depthDescription.m_Size.width  = description.m_uiWidth;
      depthDescription.m_Size.height = description.m_uiHeight;
      depthDescription.m_uiMipLevels = 1U;
      depthDescription.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
      depthDescription.m_Usage       = xiiGALResourceUsage::Default;

      data.m_hDepth                = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, depthDescription, xiiGALResourceStateFlags::DepthWrite);
      data.m_hConstants            = builder.ReadBuffer(hConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_hSceneInstances       = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry             = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshlets             = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshlets      = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshletCount  = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hIndirectCommands     = builder.ReadBuffer(visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hIndirectCommandCount = builder.ReadBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hSurfaceMaterials     = builder.ReadBuffer(materials.m_hSurfaceData, xiiGALResourceStateFlags::ShaderResource);

      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged(true);
    },
    [](const DepthRasterPassData& data, xiiRenderGraphPassContext& context) {
      xiiResourceLock<xiiShaderPermutationResource> permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      if (!permutation.IsValid() || !permutation->IsShaderValid() || context.GetRenderPass() == nullptr)
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

      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.ClearDepthStencilView(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(data.m_Description.m_uiWidth), static_cast<float>(data.m_Description.m_uiHeight), 0.0f, 1.0f});
      cmd.SetPipelineState(pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiGpuSceneRasterConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Mesh | xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", context.GetBuffer(data.m_hSurfaceMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_Buffers", xiiGALShaderType::Mesh);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_Textures", xiiGALShaderType::Pixel);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
      cmd.DrawMeshIndirect({context.GetBuffer(data.m_hIndirectCommands), 1U, 0U, xiiGALStateTransitionMode::None, context.GetBuffer(data.m_hIndirectCommandCount)});
    });

  pass.first->m_Description        = description;
  pass.first->m_hShaderPermutation = s_pState->m_hDepthShaderPermutation;
  return pass.first->m_hDepth;
}

xiiGpuSceneGBufferOutputs xiiGpuSceneRasterManager::AddGBufferPass(xiiRenderGraph& graph, xiiStringView sName,
                                                                   xiiRenderGraphTextureHandle hSceneDepth,
                                                                   xiiRenderGraphTextureHandle hVelocity,
                                                                   const xiiGpuVisibilityOutputs& visibility,
                                                                   const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                                   const xiiMaterialGpuStorage::UploadHandles& materials,
                                                                   const xiiGpuSceneGBufferRasterDescription& description)
{
  if (!IsSupported() || !hSceneDepth.IsValid() || !hVelocity.IsValid() || !visibility.m_hSceneInstances.IsValid() || !visibility.m_hVisibleMeshlets.IsValid() ||
      !visibility.m_hVisibleMeshletCount.IsValid() || !visibility.m_hIndirectCommands.IsValid() ||
      !visibility.m_hIndirectCommandCount.IsValid() || !geometry.m_hGeometryMetadata.IsValid() ||
      !geometry.m_hMeshletMetadata.IsValid() || !materials.m_hSurfaceData.IsValid() || description.m_uiWidth == 0U ||
      description.m_uiHeight == 0U || description.m_uiVertexStride == 0U)
    return {};

  xiiStringBuilder constantsPassName(sName, " Constants");
  xiiStringBuilder constantsResourceName(sName, " Constants Buffer");
  auto             constantsPass = graph.AddPass<GBufferConstantsPassData>(
    constantsPassName, xiiGALCommandQueueFlags::Graphics,
    [constantsResourceName](GBufferConstantsPassData& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiGpuSceneGBufferConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer(constantsResourceName, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
    },
    [](const GBufferConstantsPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALMapHelper<xiiGpuSceneGBufferConstants> constants(context.GetCommandList(), context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->ViewProjectionMatrix         = data.m_Description.m_ViewProjectionMatrix;
      constants->PreviousViewProjectionMatrix = data.m_Description.m_PreviousViewProjectionMatrix;
      constants->InvRenderSize                = xiiVec2(1.0f / static_cast<float>(data.m_Description.m_uiWidth), 1.0f / static_cast<float>(data.m_Description.m_uiHeight));
      constants->CurrentJitter                 = data.m_Description.m_vCurrentJitter;
      constants->PreviousJitter                = data.m_Description.m_vPreviousJitter;
      constants->GeometryBaseIndex            = data.m_uiGeometryBaseIndex;
      constants->MaterialBaseIndex            = data.m_uiMaterialBaseIndex;
      constants->VertexStride                 = data.m_Description.m_uiVertexStride;
      constants->NormalOffset                 = data.m_Description.m_uiNormalOffset;
      constants->TangentOffset                = data.m_Description.m_uiTangentOffset;
      constants->TexCoordOffset               = data.m_Description.m_uiTexCoordOffset;
      constants->MeshDispatchGroupCountX      = data.m_Description.m_uiMeshDispatchGroupCountX;
      constants->MeshDispatchGroupCountY      = data.m_Description.m_uiMeshDispatchGroupCountY;
    });
  constantsPass.first->m_Description         = description;
  constantsPass.first->m_uiGeometryBaseIndex = geometry.m_uiGeometryBaseIndex;
  constantsPass.first->m_uiMaterialBaseIndex = materials.m_uiSurfaceBaseIndex;

  auto pass = graph.AddPass<GBufferRasterPassData>(
    sName, xiiGALCommandQueueFlags::Graphics,
    [hSceneDepth, hVelocity, visibility, geometry, materials, hConstants = constantsPass.first->m_hConstants, description](GBufferRasterPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hDepth                = builder.ReadTexture(hSceneDepth, xiiGALResourceStateFlags::DepthRead);
      data.m_hVelocity             = builder.WriteTexture(builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
      data.m_hConstants            = builder.ReadBuffer(hConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_hSceneInstances       = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry             = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshlets             = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshlets      = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshletCount  = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hIndirectCommands     = builder.ReadBuffer(visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hIndirectCommandCount = builder.ReadBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hSurfaceMaterials     = builder.ReadBuffer(materials.m_hSurfaceData, xiiGALResourceStateFlags::ShaderResource);

      xiiGALTextureCreationDescription targetDescription;
      targetDescription.m_Type        = xiiGALResourceDimension::Texture2D;
      targetDescription.m_Size.width  = description.m_uiWidth;
      targetDescription.m_Size.height = description.m_uiHeight;
      targetDescription.m_uiMipLevels = 1U;
      targetDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      targetDescription.m_Usage       = xiiGALResourceUsage::Default;

      targetDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_hAlbedo             = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, targetDescription, xiiGALResourceStateFlags::RenderTarget);
      targetDescription.m_Format = xiiGALResourceFormat::RG16SNormalized;
      data.m_hNormal             = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferNormal, targetDescription, xiiGALResourceStateFlags::RenderTarget);
      targetDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_hMaterial           = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferMaterial, targetDescription, xiiGALResourceStateFlags::RenderTarget);
      targetDescription.m_Format = xiiGALResourceFormat::RGBA16Float;
      data.m_hEmissive           = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferEmissive, targetDescription, xiiGALResourceStateFlags::RenderTarget);
      targetDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_hNormalRoughness    = builder.WriteTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, targetDescription, xiiGALResourceStateFlags::RenderTarget);

      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged(true);
    },
    [](const GBufferRasterPassData& data, xiiRenderGraphPassContext& context) {
      xiiResourceLock<xiiShaderPermutationResource> permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      if (!permutation.IsValid() || !permutation->IsShaderValid() || context.GetRenderPass() == nullptr)
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

      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hAlbedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hNormal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.5f, 0.0f, 0.0f));
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hMaterial)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.0f, 1.0f, 0.0f));
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hEmissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.5f, 0.5f, 1.0f));
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(data.m_Description.m_uiWidth), static_cast<float>(data.m_Description.m_uiHeight), 0.0f, 1.0f});
      cmd.SetPipelineState(pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Mesh | xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
      cmd.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", context.GetBuffer(data.m_hSurfaceMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_Buffers", xiiGALShaderType::Mesh);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_Textures", xiiGALShaderType::Pixel);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
      cmd.DrawMeshIndirect({context.GetBuffer(data.m_hIndirectCommands), 1U, 0U, xiiGALStateTransitionMode::None, context.GetBuffer(data.m_hIndirectCommandCount)});
    });

  pass.first->m_Description        = description;
  pass.first->m_hShaderPermutation = s_pState->m_hGBufferShaderPermutation;
  return {pass.first->m_hAlbedo, pass.first->m_hNormal, pass.first->m_hMaterial, pass.first->m_hEmissive, pass.first->m_hNormalRoughness, pass.first->m_hVelocity};
}

void xiiGpuSceneRasterManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "GPU scene raster manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiGpuSceneRasterManagerState);
}

void xiiGpuSceneRasterManager::EngineStartup()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted               = true;
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  s_pState->m_bMeshShadersSupported        = pDevice != nullptr && pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled;
  if (!s_pState->m_bMeshShadersSupported)
    return;

  const xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/GpuSceneDepth.xiiShader");
  s_pState->m_hDepthShaderPermutation    = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, {}, true);
  const xiiShaderResourceHandle hGBufferShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/GpuSceneGBuffer.xiiShader");
  s_pState->m_hGBufferShaderPermutation        = xiiShaderPermutationUtilities::PreloadSinglePermutation(hGBufferShader, {}, true);
}

void xiiGpuSceneRasterManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_hDepthShaderPermutation.Invalidate();
  s_pState->m_hGBufferShaderPermutation.Invalidate();
  s_pState->m_bMeshShadersSupported = false;
  s_pState->m_bEngineStarted        = false;
}

void xiiGpuSceneRasterManager::Shutdown()
{
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuSceneRaster);
