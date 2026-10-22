/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MatrixRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMatrixRasterConstantsPass, 1, xiiRTTIDefaultAllocator<xiiMatrixRasterConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RasterConstants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ViewProjection", m_ViewProjection),
    XII_MEMBER_PROPERTY("PreviousViewProjection", m_PreviousViewProjection),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, 65536U)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, 65536U)),
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

xiiResult xiiMatrixRasterConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_ViewProjection.IsValid() && m_PreviousViewProjection.IsValid() && m_uiWidth && m_uiWidth <= 65536 && m_uiHeight && m_uiHeight <= 65536 && (m_uiVertexStride | m_uiNormalOffset | m_uiTangentOffset | m_uiTexCoordOffset) % 4 == 0 &&
      m_uiVertexStride >= 16 && m_uiNormalOffset <= m_uiVertexStride - 12 && m_uiTangentOffset <= m_uiVertexStride - 16 &&
      m_uiTexCoordOffset <= m_uiVertexStride - 8 && m_uiMeshGroupCountX > 0 && m_uiMeshGroupCountY > 0 &&
      xiiUInt64(m_uiMeshGroupCountX) * m_uiMeshGroupCountY <= xiiMath::MaxValue<xiiUInt32>() ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiMatrixRasterConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!xiiGeometryResidencyManager::IsInitialized() || !xiiMaterialManager::IsInitialized())
    return context.ReportFailure("Matrix raster constants require initialized device residency producers.");
  auto width = m_uiWidth, height = m_uiHeight, groupsX = m_uiMeshGroupCountX, groupsY = m_uiMeshGroupCountY;
  auto projection = m_ViewProjection, previous = m_PreviousViewProjection;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterViewProjection", projection));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterPreviousViewProjection", previous));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterWidth", width));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterHeight", height));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterMeshGroupCountX", groupsX));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterMeshGroupCountY", groupsY));
  if (!projection.IsValid() || !previous.IsValid() || !width || !height || width > 65536 || height > 65536 || !groupsX || !groupsY || xiiUInt64(groupsX) * groupsY > xiiMath::MaxValue<xiiUInt32>())
    return context.ReportFailure("Invalid matrix raster projection, dimensions or mesh command strides.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle  m_Buffer;
    xiiGpuSceneGBufferConstants m_Snapshot;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Buffer = xiiRenderPassUtils::Constants(builder, "Surface Raster Constants", sizeof(data.m_Snapshot));
      data.m_Snapshot.ViewProjectionMatrix = projection;
      data.m_Snapshot.PreviousViewProjectionMatrix = previous;
      data.m_Snapshot.InvRenderSize = xiiVec2(1.0f / width, 1.0f / height);
      data.m_Snapshot.CurrentJitter = data.m_Snapshot.PreviousJitter = xiiVec2::MakeZero();
      data.m_Snapshot.GeometryBaseIndex = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
      data.m_Snapshot.MaterialBaseIndex = xiiMaterialManager::GetGpuStorage().GetSurfaceBaseIndex(context.m_Graph.GetFrameIndex());
      data.m_Snapshot.VertexStride = m_uiVertexStride;
      data.m_Snapshot.NormalOffset = m_uiNormalOffset;
      data.m_Snapshot.TangentOffset = m_uiTangentOffset;
      data.m_Snapshot.TexCoordOffset = m_uiTexCoordOffset;
      data.m_Snapshot.MeshDispatchGroupCountX = groupsX;
      data.m_Snapshot.MeshDispatchGroupCountY = groupsY; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
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
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_MatrixRasterConstantsPass);
