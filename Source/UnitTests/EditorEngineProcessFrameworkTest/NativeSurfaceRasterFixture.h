/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/SurfaceRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

/// Explicit fixture inputs only; production operations consume reflected resource pins.
struct xiiTestSurfaceLayout
{
  xiiMat4   m_ViewProjectionMatrix         = xiiMat4::MakeIdentity();
  xiiMat4   m_PreviousViewProjectionMatrix = xiiMat4::MakeIdentity();
  xiiVec2   m_vCurrentJitter = xiiVec2::MakeZero(), m_vPreviousJitter = xiiVec2::MakeZero();
  xiiUInt32 m_uiWidth = 0, m_uiHeight = 0;
  xiiUInt32 m_uiVertexStride = 0, m_uiNormalOffset = 0, m_uiTangentOffset = 0, m_uiTexCoordOffset = 0;
  xiiUInt32 m_uiMeshDispatchGroupCountX = 1, m_uiMeshDispatchGroupCountY = 1;
};

struct xiiTestMaterialTargets
{
  xiiRenderGraphTextureHandle m_hAlbedo, m_hNormal, m_hMaterial, m_hEmissive, m_hNormalRoughness, m_hVelocity, m_hObjectId;
  bool                        IsValid() const { return m_hAlbedo.IsValid() && m_hNormal.IsValid() && m_hMaterial.IsValid() && m_hEmissive.IsValid() && m_hNormalRoughness.IsValid() && m_hVelocity.IsValid() && m_hObjectId.IsValid(); }
};

/// Test-only bindings for isolated geometry fixtures. Schemas come from native member RTTI.
/// Application rendering uses authored assets; this fixture owns no shaders or raster backend.
inline xiiResult xiiTestNativeSurfaceQuery(xiiRenderGraph& graph, xiiView* view, xiiStringView name,
                                           const xiiGpuVisibilityOutputs& visibility, const xiiGeometryResidencyManager::UploadHandles& geometry,
                                           const xiiMaterialGpuStorage::UploadHandles& materials, const xiiTestSurfaceLayout& raster,
                                           xiiRenderGraphTextureHandle& output, bool mesh = true, xiiGALFillMode::Enum fill = xiiGALFillMode::Solid,
                                           xiiRenderGraphBufferHandle* out_commands = nullptr, xiiRenderGraphBufferHandle* out_count = nullptr, bool depthOnly = false, xiiTestMaterialTargets* materialTargets = nullptr, xiiRenderGraphTextureHandle sceneDepth = {})
{
  output = {};
  if (!view || !raster.m_ViewProjectionMatrix.IsValid() || !raster.m_uiWidth || !raster.m_uiHeight ||
      raster.m_uiWidth != view->GetRenderResolutionWidth() || raster.m_uiHeight != view->GetRenderResolutionHeight())
    return XII_FAILURE;
  xiiSurfaceRasterConstantsPass layout;
  layout.m_uiVertexStride    = raster.m_uiVertexStride;
  layout.m_uiNormalOffset    = raster.m_uiNormalOffset;
  layout.m_uiTangentOffset   = raster.m_uiTangentOffset;
  layout.m_uiTexCoordOffset  = raster.m_uiTexCoordOffset;
  layout.m_uiMeshGroupCountX = mesh ? raster.m_uiMeshDispatchGroupCountX : 1;
  layout.m_uiMeshGroupCountY = mesh ? raster.m_uiMeshDispatchGroupCountY : 1;
  if (layout.Configure({}).Failed())
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Fixture '%s': invalid raster layout (%u, %u, %u, %u), mesh grid %u x %u.", xiiString(name).GetData(), layout.m_uiVertexStride, layout.m_uiNormalOffset, layout.m_uiTangentOffset, layout.m_uiTexCoordOffset, layout.m_uiMeshGroupCountX, layout.m_uiMeshGroupCountY);
    return XII_FAILURE;
  }
  struct Inputs
  {
    xiiRenderGraphBufferHandle  m_Constants, m_View;
    xiiGpuSceneGBufferConstants m_Snapshot;
    xiiGpuVisibilityView        m_ViewSnapshot;
  };
  const auto                 inputs = graph.AddPass<Inputs>(xiiStringBuilder(name, " Fixture Inputs"), xiiGALCommandQueueFlags::Transfer, [&](Inputs& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription uniform;
      uniform.m_uiSize = sizeof(data.m_Snapshot);
      uniform.m_BindFlags = xiiGALBindFlags::UniformBuffer;
      uniform.m_Usage = xiiGALResourceUsage::Dynamic;
      uniform.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_Constants = builder.WriteBuffer(xiiStringBuilder(name, " Fixture Constants"), uniform, xiiGALResourceStateFlags::ConstantBuffer);
      xiiMemoryUtils::ZeroFill(reinterpret_cast<xiiUInt8*>(&data.m_Snapshot), sizeof(data.m_Snapshot));
      xiiMemoryUtils::ZeroFill(reinterpret_cast<xiiUInt8*>(&data.m_ViewSnapshot), sizeof(data.m_ViewSnapshot));
      data.m_Snapshot.ViewProjectionMatrix = raster.m_ViewProjectionMatrix;
      data.m_Snapshot.PreviousViewProjectionMatrix = raster.m_PreviousViewProjectionMatrix;
      data.m_Snapshot.InvRenderSize = xiiVec2(1.0f / raster.m_uiWidth, 1.0f / raster.m_uiHeight);
      data.m_Snapshot.CurrentJitter = raster.m_vCurrentJitter;
      data.m_Snapshot.PreviousJitter = raster.m_vPreviousJitter;
      data.m_Snapshot.GeometryBaseIndex = geometry.m_uiGeometryBaseIndex;
      data.m_Snapshot.MaterialBaseIndex = materials.m_uiSurfaceBaseIndex;
      data.m_Snapshot.VertexStride = raster.m_uiVertexStride;
      data.m_Snapshot.NormalOffset = raster.m_uiNormalOffset;
      data.m_Snapshot.TangentOffset = raster.m_uiTangentOffset;
      data.m_Snapshot.TexCoordOffset = raster.m_uiTexCoordOffset;
      data.m_Snapshot.MeshDispatchGroupCountX = raster.m_uiMeshDispatchGroupCountX;
      data.m_Snapshot.MeshDispatchGroupCountY = raster.m_uiMeshDispatchGroupCountY;
      data.m_ViewSnapshot.m_uiGeometryBaseIndex = geometry.m_uiGeometryBaseIndex;
      xiiGALBufferCreationDescription description;
      description.m_uiSize = sizeof(data.m_ViewSnapshot);
      description.m_uiElementByteStride = sizeof(data.m_ViewSnapshot);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_View = builder.WriteBuffer(xiiStringBuilder(name, " Fixture View"), description, xiiGALResourceStateFlags::CopyDestination); }, [](const Inputs& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiGpuSceneGBufferConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Snapshot;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execute.GetBuffer(data.m_Constants);
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      commands.UpdateBuffer(execute.GetBuffer(data.m_View), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_ViewSnapshot), sizeof(data.m_ViewSnapshot))); });
  xiiRenderGraphBlackboard   blackboard;
  xiiVariantDictionary       parameters;
  xiiRenderGraphBufferHandle arguments = visibility.m_hIndirectCommands, argumentCount = visibility.m_hIndirectCommandCount;
  if (!mesh)
  {
    xiiVertexPullDrawCommandsPass         prepare;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prepare.GetPins(pins);
    XII_SUCCEED_OR_RETURN(prepare.InitializePinBindings(pins));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    bindings[prepare.m_Scene.GetBindingIndex()].m_hBuffer    = visibility.m_hSceneInstances;
    bindings[prepare.m_View.GetBindingIndex()].m_hBuffer     = inputs.first->m_View;
    bindings[prepare.m_Geometry.GetBindingIndex()].m_hBuffer = geometry.m_hGeometryMetadata;
    bindings[prepare.m_Meshlets.GetBindingIndex()].m_hBuffer = geometry.m_hMeshletMetadata;
    bindings[prepare.m_Visible.GetBindingIndex()].m_hBuffer  = visibility.m_hVisibleMeshlets;
    bindings[prepare.m_Count.GetBindingIndex()].m_hBuffer    = visibility.m_hVisibleMeshletCount;
    const xiiStringBuilder       label(name, " Draw Preparation");
    xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, label};
    XII_SUCCEED_OR_RETURN(prepare.Build(context));
    arguments     = bindings[prepare.m_Commands.GetBindingIndex()].m_hBuffer;
    argumentCount = bindings[prepare.m_CommandCount.GetBindingIndex()].m_hBuffer;
  }
  if (materialTargets)
  {
    xiiGBufferRasterPass surface;
    surface.m_bUseMeshShaders = mesh;
    surface.m_FillMode        = fill;
    XII_SUCCEED_OR_RETURN(surface.Configure({}));
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    surface.GetPins(pins);
    XII_SUCCEED_OR_RETURN(surface.InitializePinBindings(pins));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    bindings[surface.m_InputSceneInstances.GetBindingIndex()].m_hBuffer      = visibility.m_hSceneInstances;
    bindings[surface.m_InputSkinningMatrices.GetBindingIndex()].m_hBuffer    = visibility.m_hSkinningMatrices;
    bindings[surface.m_InputGeometryMetadata.GetBindingIndex()].m_hBuffer    = geometry.m_hGeometryMetadata;
    bindings[surface.m_InputMeshletMetadata.GetBindingIndex()].m_hBuffer     = geometry.m_hMeshletMetadata;
    bindings[surface.m_InputVisibleMeshlets.GetBindingIndex()].m_hBuffer     = visibility.m_hVisibleMeshlets;
    bindings[surface.m_InputVisibleMeshletCount.GetBindingIndex()].m_hBuffer = visibility.m_hVisibleMeshletCount;
    bindings[surface.m_InputSurfaceMaterials.GetBindingIndex()].m_hBuffer    = materials.m_hSurfaceData;
    bindings[surface.m_InputRasterConstants.GetBindingIndex()].m_hBuffer     = inputs.first->m_Constants;
    bindings[surface.m_InputDrawCommands.GetBindingIndex()].m_hBuffer        = arguments;
    bindings[surface.m_InputDrawCommandCount.GetBindingIndex()].m_hBuffer    = argumentCount;
    bindings[surface.m_InputDepth.GetBindingIndex()].m_hTexture              = sceneDepth;
    xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, name};
    xiiStringBuilder             error;
    context.m_pBuildError = &error;
    if (surface.Build(context).Failed())
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Fixture '%s': %s", xiiString(name).GetData(), error.GetData());
      return XII_FAILURE;
    }
    materialTargets->m_hAlbedo          = bindings[surface.m_OutputAlbedo.GetBindingIndex()].m_hTexture;
    materialTargets->m_hNormal          = bindings[surface.m_OutputNormal.GetBindingIndex()].m_hTexture;
    materialTargets->m_hMaterial        = bindings[surface.m_OutputMaterial.GetBindingIndex()].m_hTexture;
    materialTargets->m_hEmissive        = bindings[surface.m_OutputEmissive.GetBindingIndex()].m_hTexture;
    materialTargets->m_hNormalRoughness = bindings[surface.m_OutputNormalRoughness.GetBindingIndex()].m_hTexture;
    materialTargets->m_hVelocity        = bindings[surface.m_OutputVelocity.GetBindingIndex()].m_hTexture;
    materialTargets->m_hObjectId        = bindings[surface.m_OutputObjectId.GetBindingIndex()].m_hTexture;
  }
  else if (depthOnly)
  {
    xiiDepthRasterPass surface;
    surface.m_bUseMeshShaders = mesh;
    surface.m_FillMode        = fill;
    XII_SUCCEED_OR_RETURN(surface.Configure({}));
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    surface.GetPins(pins);
    XII_SUCCEED_OR_RETURN(surface.InitializePinBindings(pins));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    bindings[surface.m_InputSceneInstances.GetBindingIndex()].m_hBuffer      = visibility.m_hSceneInstances;
    bindings[surface.m_InputSkinningMatrices.GetBindingIndex()].m_hBuffer    = visibility.m_hSkinningMatrices;
    bindings[surface.m_InputGeometryMetadata.GetBindingIndex()].m_hBuffer    = geometry.m_hGeometryMetadata;
    bindings[surface.m_InputMeshletMetadata.GetBindingIndex()].m_hBuffer     = geometry.m_hMeshletMetadata;
    bindings[surface.m_InputVisibleMeshlets.GetBindingIndex()].m_hBuffer     = visibility.m_hVisibleMeshlets;
    bindings[surface.m_InputVisibleMeshletCount.GetBindingIndex()].m_hBuffer = visibility.m_hVisibleMeshletCount;
    bindings[surface.m_InputSurfaceMaterials.GetBindingIndex()].m_hBuffer    = materials.m_hSurfaceData;
    bindings[surface.m_InputRasterConstants.GetBindingIndex()].m_hBuffer     = inputs.first->m_Constants;
    bindings[surface.m_InputDrawCommands.GetBindingIndex()].m_hBuffer        = arguments;
    bindings[surface.m_InputDrawCommandCount.GetBindingIndex()].m_hBuffer    = argumentCount;
    xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, name};
    xiiStringBuilder             error;
    context.m_pBuildError = &error;
    if (surface.Build(context).Failed())
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Fixture '%s': %s", xiiString(name).GetData(), error.GetData());
      return XII_FAILURE;
    }
    output = bindings[surface.m_OutputDepth.GetBindingIndex()].m_hTexture;
  }
  else
  {
    xiiSurfaceQueryPass surface;
    surface.m_bUseMeshShaders = mesh;
    surface.m_FillMode        = fill;
    XII_SUCCEED_OR_RETURN(surface.Configure({}));
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    surface.GetPins(pins);
    XII_SUCCEED_OR_RETURN(surface.InitializePinBindings(pins));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    bindings[surface.m_InputSceneInstances.GetBindingIndex()].m_hBuffer      = visibility.m_hSceneInstances;
    bindings[surface.m_InputSkinningMatrices.GetBindingIndex()].m_hBuffer    = visibility.m_hSkinningMatrices;
    bindings[surface.m_InputGeometryMetadata.GetBindingIndex()].m_hBuffer    = geometry.m_hGeometryMetadata;
    bindings[surface.m_InputMeshletMetadata.GetBindingIndex()].m_hBuffer     = geometry.m_hMeshletMetadata;
    bindings[surface.m_InputVisibleMeshlets.GetBindingIndex()].m_hBuffer     = visibility.m_hVisibleMeshlets;
    bindings[surface.m_InputVisibleMeshletCount.GetBindingIndex()].m_hBuffer = visibility.m_hVisibleMeshletCount;
    bindings[surface.m_InputSurfaceMaterials.GetBindingIndex()].m_hBuffer    = materials.m_hSurfaceData;
    bindings[surface.m_InputRasterConstants.GetBindingIndex()].m_hBuffer     = inputs.first->m_Constants;
    bindings[surface.m_InputDrawCommands.GetBindingIndex()].m_hBuffer        = arguments;
    bindings[surface.m_InputDrawCommandCount.GetBindingIndex()].m_hBuffer    = argumentCount;
    xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, name};
    xiiStringBuilder             error;
    context.m_pBuildError = &error;
    if (surface.Build(context).Failed())
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Fixture '%s': %s", xiiString(name).GetData(), error.GetData());
      return XII_FAILURE;
    }
    output = bindings[surface.m_OutputSurface.GetBindingIndex()].m_hTexture;
  }
  if (out_commands)
    *out_commands = arguments;
  if (out_count)
    *out_count = argumentCount;
  return XII_SUCCESS;
}
