/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/SurfaceRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceRasterConstantsPass, 1, xiiRTTIDefaultAllocator<xiiSurfaceRasterConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RasterConstants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VertexStride", m_uiVertexStride)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(sizeof(xiiMeshPackedVertex)))),
    XII_MEMBER_PROPERTY("NormalOffset", m_uiNormalOffset)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(offsetof(xiiMeshPackedVertex, m_vNormal)))),
    XII_MEMBER_PROPERTY("TangentOffset", m_uiTangentOffset)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(offsetof(xiiMeshPackedVertex, m_vTangent)))),
    XII_MEMBER_PROPERTY("TexCoordOffset", m_uiTexCoordOffset)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(offsetof(xiiMeshPackedVertex, m_vTexCoord0)))),
    XII_MEMBER_PROPERTY("MeshGroupCountX", m_uiMeshGroupCountX)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, xiiMath::MaxValue<xiiUInt32>())),
    XII_MEMBER_PROPERTY("MeshGroupCountY", m_uiMeshGroupCountY)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, xiiMath::MaxValue<xiiUInt32>())),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSurfaceRasterConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return (m_uiVertexStride | m_uiNormalOffset | m_uiTangentOffset | m_uiTexCoordOffset) % 4 == 0 &&
      m_uiVertexStride >= 16 && m_uiNormalOffset <= m_uiVertexStride - 12 && m_uiTangentOffset <= m_uiVertexStride - 16 &&
      m_uiTexCoordOffset <= m_uiVertexStride - 8 && m_uiMeshGroupCountX > 0 && m_uiMeshGroupCountY > 0 &&
      xiiUInt64(m_uiMeshGroupCountX) * m_uiMeshGroupCountY <= xiiMath::MaxValue<xiiUInt32>() ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiSurfaceRasterConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetCamera() || context.m_pView->GetCamera()->GetCameraMode() == xiiCameraMode::None ||
      !xiiGeometryResidencyManager::IsInitialized() || !xiiMaterialManager::IsInitialized())
    return context.ReportFailure("Raster constants require a camera and initialized device residency producers.");
  const auto width = context.m_pView->GetRenderResolutionWidth(), height = context.m_pView->GetRenderResolutionHeight();
  if (width == 0 || height == 0)
    return context.ReportFailure("Raster constants require a nonempty render resolution.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle  m_Buffer;
    xiiGpuSceneGBufferConstants m_Snapshot;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Buffer = xiiRenderPassUtils::Constants(builder, "Surface Raster Constants", sizeof(data.m_Snapshot));
      data.m_Snapshot.ViewProjectionMatrix = context.m_pView->GetViewProjectionMatrix(xiiCameraEye::Left);
      data.m_Snapshot.PreviousViewProjectionMatrix = data.m_Snapshot.ViewProjectionMatrix;
      data.m_Snapshot.InvRenderSize = xiiVec2(1.0f / width, 1.0f / height);
      data.m_Snapshot.CurrentJitter = data.m_Snapshot.PreviousJitter = xiiVec2::MakeZero();
      data.m_Snapshot.GeometryBaseIndex = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
      data.m_Snapshot.MaterialBaseIndex = xiiMaterialManager::GetGpuStorage().GetSurfaceBaseIndex(context.m_Graph.GetFrameIndex());
      data.m_Snapshot.VertexStride = m_uiVertexStride;
      data.m_Snapshot.NormalOffset = m_uiNormalOffset;
      data.m_Snapshot.TangentOffset = m_uiTangentOffset;
      data.m_Snapshot.TexCoordOffset = m_uiTexCoordOffset;
      data.m_Snapshot.MeshDispatchGroupCountX = m_uiMeshGroupCountX;
      data.m_Snapshot.MeshDispatchGroupCountY = m_uiMeshGroupCountY; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiGpuSceneGBufferConstants> mapped(commands, execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Snapshot;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execute.GetBuffer(data.m_Buffer);
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); });
  return context.SetOutput(m_Constants, emitted.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_SurfaceRasterConstantsPass);
