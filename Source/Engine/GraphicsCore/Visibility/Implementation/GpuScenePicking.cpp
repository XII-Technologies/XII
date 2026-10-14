/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsCore/Visibility/Implementation/GpuSceneRasterState.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

namespace
{
  struct Constants
  {
    xiiRenderGraphBufferHandle          m_Buffer;
    xiiGpuSceneGBufferRasterDescription m_Description;
    xiiUInt32                           m_uiGeometryBase = 0, m_uiMaterialBase = 0;
  };
  struct Raster
  {
    xiiRenderGraphTextureHandle        m_Target, m_Depth;
    xiiRenderGraphBufferHandle         m_Constants, m_Instances, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Commands, m_CommandCount, m_Materials, m_Skinning;
    xiiUInt32                          m_uiWidth = 0, m_uiHeight = 0;
    xiiUInt32                          m_uiMaxDrawCount = 1U;
    bool                               m_bIndexed       = false;
    xiiShaderPermutationResourceHandle m_hShader;
  };
  struct IndexedCommands
  {
    xiiRenderGraphBufferHandle               m_Constants, m_Instances, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Commands;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiMaxDrawCount = 0U;
  };
} // namespace

xiiResult xiiGpuSceneRasterManager::AddPickingPass(xiiRenderGraph& graph, xiiStringView name,
                                                   const xiiGpuVisibilityOutputs& visibility, const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                   const xiiMaterialGpuStorage::UploadHandles& materials, const xiiGpuSceneGBufferRasterDescription& description, xiiRenderGraphTextureHandle& out_hSurface, bool bUseMeshShaders, xiiGALFillMode::Enum fillMode)
{
  out_hSurface = {};
  if (fillMode != xiiGALFillMode::Solid && fillMode != xiiGALFillMode::Wireframe)
    return XII_FAILURE;
  if (fillMode == xiiGALFillMode::Wireframe &&
      (!xiiGALDevice::HasDefaultDevice() || xiiGALDevice::GetDefaultDevice()->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled))
    return XII_FAILURE;
  const bool indexed = !bUseMeshShaders || !IsSupported();
  if (!IsPickingSupported() || !visibility.m_hSceneInstances.IsValid() || !visibility.m_hSkinningMatrices.IsValid() || !visibility.m_hVisibleMeshlets.IsValid() ||
      !visibility.m_hVisibleMeshletCount.IsValid() ||
      (!indexed && (!visibility.m_hIndirectCommands.IsValid() || !visibility.m_hIndirectCommandCount.IsValid() || description.m_uiMeshDispatchGroupCountX == 0 || description.m_uiMeshDispatchGroupCountY == 0)) ||
      (indexed && (visibility.m_uiMaxVisibleMeshlets == 0U || visibility.m_uiMaxVisibleMeshlets > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiVec4U32))) ||
      !geometry.m_hGeometryMetadata.IsValid() || !geometry.m_hMeshletMetadata.IsValid() || !materials.m_hSurfaceData.IsValid() ||
      description.m_uiWidth == 0 || description.m_uiHeight == 0 || description.m_uiVertexStride < 16 ||
      description.m_uiNormalOffset > description.m_uiVertexStride - 12 ||
      description.m_uiTangentOffset > description.m_uiVertexStride - 16 ||
      description.m_uiTexCoordOffset > description.m_uiVertexStride - 8 || !description.m_ViewProjectionMatrix.IsValid())
    return XII_FAILURE;
  const auto                                    hShader = fillMode == xiiGALFillMode::Wireframe ?
    (indexed ? s_pState->m_hIndexedWireframePickingShaderPermutation : s_pState->m_hWireframePickingShaderPermutation) :
    (indexed ? s_pState->m_hIndexedPickingShaderPermutation : s_pState->m_hPickingShaderPermutation);
  xiiResourceLock<xiiShaderPermutationResource> permutation(hShader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
    return XII_FAILURE;
  xiiSharedPtr<xiiGALComputePipelineState> indexedPipeline;
  if (indexed)
  {
    const auto device = xiiGALDevice::GetDefaultDevice();
    if (!device)
      return XII_FAILURE;
    const auto& properties = device->GetGraphicsDeviceAdapterProperties();
    if (!properties.m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer) ||
        visibility.m_uiMaxVisibleMeshlets > properties.m_DrawCommandProperties.m_uiMaxDrawIndirectCount ||
        (visibility.m_uiMaxVisibleMeshlets + 63U) / 64U > properties.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX)
      return XII_FAILURE;
    xiiResourceLock<xiiShaderPermutationResource> commands(s_pState->m_hIndexedPickingCommandsShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!commands.IsValid() || !commands->IsShaderValid())
      return XII_FAILURE;
    xiiGALComputePipelineStateCreationDescription pipeline;
    pipeline.m_pComputeShader             = commands->GetGALShader(xiiGALShaderType::Compute);
    pipeline.m_pPipelineResourceSignature = commands->GetPipelineResourceSignature();
    indexedPipeline                       = xiiGALPipelineCache::GetPipeline(pipeline);
    if (!indexedPipeline)
      return XII_FAILURE;
  }
  auto constants                             = graph.AddPass<Constants>(xiiStringBuilder(name, " Constants"), xiiGALCommandQueueFlags::Graphics, [resourceName = xiiStringBuilder(name, " Constants buffer")](Constants& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = sizeof(xiiGpuSceneGBufferConstants);
      buffer.m_BindFlags = xiiGALBindFlags::UniformBuffer;
      buffer.m_Usage = xiiGALResourceUsage::Dynamic;
      buffer.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_Buffer = builder.WriteBuffer(resourceName, buffer, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false); }, [](const Constants& data, xiiRenderGraphPassContext& execution) {
      xiiGALMapHelper<xiiGpuSceneGBufferConstants> mapped(execution.GetCommandList(), execution.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      const auto& desc = data.m_Description;
      mapped->ViewProjectionMatrix = desc.m_ViewProjectionMatrix;
      mapped->PreviousViewProjectionMatrix = desc.m_ViewProjectionMatrix;
      mapped->InvRenderSize = xiiVec2(1.0f / desc.m_uiWidth, 1.0f / desc.m_uiHeight);
      mapped->CurrentJitter = xiiVec2::MakeZero();
      mapped->PreviousJitter = xiiVec2::MakeZero();
      mapped->GeometryBaseIndex = data.m_uiGeometryBase;
      mapped->MaterialBaseIndex = data.m_uiMaterialBase;
      mapped->VertexStride = desc.m_uiVertexStride;
      mapped->NormalOffset = desc.m_uiNormalOffset;
      mapped->TangentOffset = desc.m_uiTangentOffset;
      mapped->TexCoordOffset = desc.m_uiTexCoordOffset;
      mapped->MeshDispatchGroupCountX = desc.m_uiMeshDispatchGroupCountX;
      mapped->MeshDispatchGroupCountY = desc.m_uiMeshDispatchGroupCountY; });
  constants.first->m_Description             = description;
  constants.first->m_uiGeometryBase          = geometry.m_uiGeometryBaseIndex;
  constants.first->m_uiMaterialBase          = materials.m_uiSurfaceBaseIndex;
  const auto                 constantsHandle = constants.first->m_Buffer;
  xiiRenderGraphBufferHandle indexedCommands;
  if (indexed)
  {
    auto build                    = graph.AddPass<IndexedCommands>(xiiStringBuilder(name, " Indexed commands"), xiiGALCommandQueueFlags::Graphics, [&](IndexedCommands& data, xiiRenderGraphBuilder& builder) {
        data.m_Constants = builder.ReadBuffer(constantsHandle, xiiGALResourceStateFlags::ConstantBuffer);
        data.m_Instances = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_Geometry = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_Meshlets = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_Visible = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
        data.m_Count = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
        xiiGALBufferCreationDescription buffer;
        builder.SetPassAllowMerge(false);
        buffer.m_uiSize = static_cast<xiiUInt64>(visibility.m_uiMaxVisibleMeshlets) * sizeof(xiiVec4U32);
        buffer.m_uiElementByteStride = sizeof(xiiVec4U32);
        buffer.m_Mode = xiiGALBufferMode::Structured;
        buffer.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
        data.m_Commands = builder.WriteBuffer(xiiStringBuilder(name, " Indexed commands buffer"), buffer, xiiGALResourceStateFlags::UnorderedAccess); }, [](const IndexedCommands& data, xiiRenderGraphPassContext& execution) {
        auto& cmd = execution.GetCommandList();
        cmd.SetPipelineState(data.m_pPipeline.Borrow());
        cmd.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants", execution.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execution.GetBuffer(data.m_Instances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", execution.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", execution.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", execution.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", execution.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetUnorderedAccessBufferView("g_IndexedCommands", execution.GetBuffer(data.m_Commands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed())
        {
          xiiLog::Error("Could not bind native indexed query commands.");
          return;
        }
        cmd.DispatchCompute({(data.m_uiMaxDrawCount + 63U) / 64U, 1U, 1U}); });
    build.first->m_pPipeline      = indexedPipeline;
    build.first->m_uiMaxDrawCount = visibility.m_uiMaxVisibleMeshlets;
    indexedCommands               = build.first->m_Commands;
  }
  auto pass                    = graph.AddPass<Raster>(name, xiiGALCommandQueueFlags::Graphics, [&](Raster& data, xiiRenderGraphBuilder& builder) {
      data.m_Constants = builder.ReadBuffer(constantsHandle, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_Instances = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_Skinning  = builder.ReadBuffer(visibility.m_hSkinningMatrices, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_Visible = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      if (!indexed)
        data.m_Count = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_Commands = builder.ReadBuffer(indexed ? indexedCommands : visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
      data.m_CommandCount = builder.ReadBuffer(indexed ? visibility.m_hVisibleMeshletCount : visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);
      data.m_Materials = builder.ReadBuffer(materials.m_hSurfaceData, xiiGALResourceStateFlags::ShaderResource);
      xiiGALTextureCreationDescription target;
      target.m_Type = xiiGALResourceDimension::Texture2D;
      target.m_Size = {description.m_uiWidth, description.m_uiHeight};
      target.m_Format = xiiGALResourceFormat::RGBA32UInt;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      data.m_Target = builder.WriteTexture(xiiStringBuilder(name, " Surface"), target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::D32Float;
      target.m_BindFlags = xiiGALBindFlags::DepthStencil;
      data.m_Depth = builder.WriteTexture(xiiStringBuilder(name, " Depth"), target, xiiGALResourceStateFlags::DepthWrite);
      data.m_uiWidth = description.m_uiWidth;
      data.m_uiHeight = description.m_uiHeight;
      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged(); }, [](const Raster& data, xiiRenderGraphPassContext& execution) {
      auto& commands = execution.GetCommandList();
      commands.ClearRenderTargetView(execution.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      commands.ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0);
      xiiResourceLock<xiiShaderPermutationResource> shader(data.m_hShader, xiiResourceAcquireMode::BlockTillLoaded);
      if (!shader.IsValid() || !shader->IsShaderValid())
      {
        xiiLog::Error("The native surface query shader is unavailable.");
        return;
      }
      xiiGALGraphicsPipelineStateCreationDescription pipeline;
      pipeline.m_PipelineType = data.m_bIndexed ? xiiGALPipelineType::Graphics : xiiGALPipelineType::Mesh;
      pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
      if (data.m_bIndexed)
        pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex);
      else
        pipeline.m_pMeshShader = shader->GetGALShader(xiiGALShaderType::Mesh);
      pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
      pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState();
      pipeline.m_GraphicsPipeline.m_pRasterizerState = shader->GetRasterizerState();
      pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
      pipeline.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(execution.GetSubpassIndex());
      pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state)
      {
        xiiLog::Error("Could not create the native surface query pipeline.");
        return;
      }
      commands.SetPipelineState(state);
      commands.SetViewport({0, 0, static_cast<float>(data.m_uiWidth), static_cast<float>(data.m_uiHeight), 0, 1});
      commands.ResolveAndSetSampler("g_sAnisotropicWrap", s_pState->m_pMaterialSampler.Borrow());
      const xiiGALShaderType::Enum geometryStage = data.m_bIndexed ? xiiGALShaderType::Vertex : xiiGALShaderType::Mesh;
      commands.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants", execution.GetBuffer(data.m_Constants), geometryStage | xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execution.GetBuffer(data.m_Instances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      commands.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices", execution.GetBuffer(data.m_Skinning)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execution.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execution.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", execution.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      if (!data.m_bIndexed)
        commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", execution.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), geometryStage);
      commands.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", execution.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
      xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_Buffers", geometryStage);
      xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_Textures", xiiGALShaderType::Pixel);
      if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed())
      {
        xiiLog::Error("Could not bind the native surface query resources.");
        return;
      }
      if (data.m_bIndexed)
        commands.DrawIndirect({execution.GetBuffer(data.m_Commands), data.m_uiMaxDrawCount, 0U, sizeof(xiiVec4U32), xiiGALStateTransitionMode::None, execution.GetBuffer(data.m_CommandCount)});
      else
        commands.DrawMeshIndirect({execution.GetBuffer(data.m_Commands), 1U, 0U, xiiGALStateTransitionMode::None, execution.GetBuffer(data.m_CommandCount)}); });
  pass.first->m_bIndexed       = indexed;
  pass.first->m_hShader        = hShader;
  pass.first->m_uiMaxDrawCount = indexed ? visibility.m_uiMaxVisibleMeshlets : 1U;
  out_hSurface                 = pass.first->m_Target;
  return XII_SUCCESS;
}
