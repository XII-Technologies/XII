/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadeViewPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowCascadeViewPass, 1, xiiRTTIDefaultAllocator<xiiShadowCascadeViewPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("CascadeSnapshot", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("RasterConstants", m_Raster)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("CascadeIndex", m_uiCascadeIndex)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 3U)),
    } XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiCategoryAttribute("Rendering/Shadows")} XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiShadowCascadeViewPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiCascadeIndex < 4 ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiShadowCascadeViewPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto input = context.GetInputData(m_Cascades);
  if (!input)
    return XII_SUCCESS;
  const auto* cascades = xiiDynamicCast<const xiiShadowCascadeSnapshot*>(input.Borrow());
  if (!cascades)
    return context.ReportFailure("Shadow view requires a cascade snapshot.");
  if (m_uiCascadeIndex >= cascades->m_Cascades.GetCount())
    return XII_SUCCESS;
  if (!context.m_pView || !context.m_pView->GetGpuInstanceData() || !xiiGeometryResidencyManager::IsInitialized() || !xiiMaterialManager::IsInitialized())
    return context.ReportFailure("Shadow view requires authoritative world extraction and device residency.");
  const auto& cascade = cascades->m_Cascades[m_uiCascadeIndex];
  if (!cascades->m_uiResolution || !cascade.m_mViewProjection.IsValid() || !xiiMath::IsFinite(cascade.m_fWorldRadius) || cascade.m_fWorldRadius <= 0)
    return context.ReportFailure("Cascade visibility requires finite projection and positive world extent/resolution.");
  const auto           frustum = xiiFrustum::MakeFromMVP(cascade.m_mViewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);
  xiiGpuVisibilityView view{};
  view.m_ViewProjectionMatrix          = cascade.m_mViewProjection;
  view.m_OcclusionViewProjectionMatrix = cascade.m_mViewProjection;
  for (xiiUInt8 plane = 0; plane < 6; ++plane)
    view.m_FrustumPlanes[plane] = frustum.GetPlane(plane).GetAsVec4();
  view.m_CameraPosition = xiiVec4(0, 0, 0, 1);
  // Orthographic coverage is world radius times pixels per metre, independent of eye distance.
  // The fourth LOD field also disables perspective cone rejection in parallel-ray views.
  view.m_ViewportAndHiZ      = xiiVec4(float(cascades->m_uiResolution), cascades->m_uiResolution / (2.0f * cascade.m_fWorldRadius), 0, 0);
  view.m_LodParameters       = xiiVec4(0, 0, 65534, 1);
  view.m_uiInstanceCount     = context.m_pView->GetGpuInstanceData()->GetInstanceCount();
  view.m_uiVisibilityMask    = 0xFFFFFFFFU;
  view.m_uiRequiredFlags     = (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::CastShadows).GetValue();
  view.m_uiExcludedFlags     = xiiGpuInstanceFlags::Transparent;
  view.m_uiGeometryBaseIndex = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
  view.m_uiUseCandidates     = 1;
  view.m_uiCandidateCount    = context.m_pView->GetGpuInstanceSelection().GetCount();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiGpuVisibilityView        m_View;
    xiiGpuSceneGBufferConstants m_Raster{};
    xiiDynamicArray<xiiUInt32>  m_Candidates;
    xiiRenderGraphBufferHandle  m_ViewBuffer, m_CandidateBuffer, m_RasterBuffer;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_View = view;
    data.m_Candidates = context.m_pView->GetGpuInstanceSelection();
    if (data.m_Candidates.IsEmpty()) data.m_Candidates.PushBack(xiiInvalidIndex);
    data.m_Raster.ViewProjectionMatrix = cascade.m_mViewProjection;
    data.m_Raster.PreviousViewProjectionMatrix = cascade.m_mViewProjection;
    data.m_Raster.InvRenderSize = xiiVec2(1.0f / cascades->m_uiResolution);
    data.m_Raster.GeometryBaseIndex = view.m_uiGeometryBaseIndex;
    data.m_Raster.MaterialBaseIndex = xiiMaterialManager::GetGpuStorage().GetSurfaceBaseIndex(context.m_Graph.GetFrameIndex());
    data.m_Raster.VertexStride = sizeof(xiiMeshPackedVertex);
    data.m_Raster.NormalOffset = offsetof(xiiMeshPackedVertex, m_vNormal);
    data.m_Raster.TangentOffset = offsetof(xiiMeshPackedVertex, m_vTangent);
    data.m_Raster.TexCoordOffset = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
    data.m_Raster.MeshDispatchGroupCountX = data.m_Raster.MeshDispatchGroupCountY = 1;
    xiiGALBufferCreationDescription description;
    description.m_Mode = xiiGALBufferMode::Structured;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource;
    description.m_uiSize = description.m_uiElementByteStride = sizeof(view);
    data.m_ViewBuffer = builder.WriteBuffer("Cascade Visibility", description, xiiGALResourceStateFlags::CopyDestination);
    description.m_uiElementByteStride = sizeof(xiiUInt32);
    description.m_uiSize = data.m_Candidates.GetCount() * sizeof(xiiUInt32);
    data.m_CandidateBuffer = builder.WriteBuffer("Cascade Candidates", description, xiiGALResourceStateFlags::CopyDestination);
    data.m_RasterBuffer = xiiRenderPassUtils::Constants(builder, "Cascade Raster Constants", sizeof(data.m_Raster)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.UpdateBuffer(execute.GetBuffer(data.m_ViewBuffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_View), sizeof(data.m_View)));
    commands.UpdateBuffer(execute.GetBuffer(data.m_CandidateBuffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Candidates.GetData()), data.m_Candidates.GetCount() * sizeof(xiiUInt32)));
    xiiGALMapHelper<xiiGpuSceneGBufferConstants> mapped(commands, execute.GetBuffer(data.m_RasterBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mapped = data.m_Raster; });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_View, emitted.first->m_ViewBuffer));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Candidates, emitted.first->m_CandidateBuffer));
  return context.SetOutput(m_Raster, emitted.first->m_RasterBuffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_ShadowCascadeViewPass);
