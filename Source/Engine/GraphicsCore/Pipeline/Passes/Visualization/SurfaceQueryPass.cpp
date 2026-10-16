/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceQueryPass, 1, xiiRTTIDefaultAllocator<xiiSurfaceQueryPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_InputSceneInstances)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("SkinningMatrices", m_InputSkinningMatrices)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_InputVisibleMeshlets)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_InputVisibleMeshletCount)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_InputGeometryMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_InputMeshletMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("SurfaceMaterials", m_InputSurfaceMaterials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("MeshIndirectCommands", m_InputMeshIndirectCommands)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("MeshIndirectCommandCount", m_InputMeshIndirectCommandCount)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Surface", m_OutputSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("OnlyViewWireframe", m_bOnlyViewWireframe)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("FillMode", xiiGALFillMode, m_FillMode)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFillMode::Solid)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSurfaceQueryPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_FillMode == xiiGALFillMode::Solid || m_FillMode == xiiGALFillMode::Wireframe ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiSurfaceQueryPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView)
    return XII_FAILURE;
  if (m_bOnlyViewWireframe && context.m_pView->GetViewRenderMode() != xiiViewRenderMode::Wireframe)
    return XII_SUCCESS;
  xiiRenderPassResourceScope scope(context);
  xiiGpuVisibilityOutputs    visibility;
  visibility.m_hSceneInstances       = context.GetInputBuffer(m_InputSceneInstances);
  visibility.m_hSkinningMatrices     = context.GetInputBuffer(m_InputSkinningMatrices);
  visibility.m_hVisibleMeshlets      = context.GetInputBuffer(m_InputVisibleMeshlets);
  visibility.m_hVisibleMeshletCount  = context.GetInputBuffer(m_InputVisibleMeshletCount);
  visibility.m_hIndirectCommands     = context.GetInputBuffer(m_InputMeshIndirectCommands);
  visibility.m_hIndirectCommandCount = context.GetInputBuffer(m_InputMeshIndirectCommandCount);
  const auto* meshlets               = context.m_Graph.GetBufferDescription(visibility.m_hVisibleMeshlets);
  if (!meshlets || meshlets->m_uiElementByteStride == 0 || meshlets->m_uiSize / meshlets->m_uiElementByteStride > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;
  visibility.m_uiMaxVisibleMeshlets = static_cast<xiiUInt32>(meshlets->m_uiSize / meshlets->m_uiElementByteStride);
  xiiGeometryResidencyManager::UploadHandles geometry;
  geometry.m_hGeometryMetadata   = context.GetInputBuffer(m_InputGeometryMetadata);
  geometry.m_hMeshletMetadata    = context.GetInputBuffer(m_InputMeshletMetadata);
  geometry.m_uiGeometryBaseIndex = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
  if (!xiiMaterialManager::IsInitialized() || !xiiGeometryResidencyManager::IsInitialized())
    return XII_FAILURE;
  xiiMaterialGpuStorage::UploadHandles materials;
  materials.m_hSurfaceData       = context.GetInputBuffer(m_InputSurfaceMaterials);
  materials.m_uiSurfaceBaseIndex = xiiMaterialManager::GetGpuStorage().GetSurfaceBaseIndex(context.m_Graph.GetFrameIndex());
  xiiGpuSceneGBufferRasterDescription description;
  description.m_ViewProjectionMatrix = context.m_pView->GetViewProjectionMatrix(xiiCameraEye::Left);
  description.m_uiWidth              = static_cast<xiiUInt32>(context.m_pView->GetViewport().width);
  description.m_uiHeight             = static_cast<xiiUInt32>(context.m_pView->GetViewport().height);
  description.m_uiVertexStride       = sizeof(xiiMeshPackedVertex);
  description.m_uiNormalOffset       = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vNormal));
  description.m_uiTangentOffset      = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTangent));
  description.m_uiTexCoordOffset     = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0));
  if (m_bUseMeshShaders && xiiGpuSceneRasterManager::IsSupported())
  {
    const auto& properties = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;
    XII_SUCCEED_OR_RETURN(xiiGpuVisibilitySystem::ComputeMeshDispatchShape(properties, visibility.m_uiMaxVisibleMeshlets, description.m_uiMeshDispatchGroupCountX, description.m_uiMeshDispatchGroupCountY));
  }
  xiiRenderGraphTextureHandle surface;
  XII_SUCCEED_OR_RETURN(xiiGpuSceneRasterManager::AddPickingPass(context.m_Graph, context.m_sPassName, visibility, geometry, materials, description, surface, m_bUseMeshShaders, m_FillMode));
  return context.SetOutput(m_OutputSurface, surface);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visualization_SurfaceQueryPass);
