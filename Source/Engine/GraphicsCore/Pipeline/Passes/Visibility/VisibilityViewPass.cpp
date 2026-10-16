/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/VisibilityViewPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisibilityViewPass, 1, xiiRTTIDefaultAllocator<xiiVisibilityViewPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask)->AddAttributes(new xiiDefaultValueAttribute(0xFFFFFFFFU)),
    XII_MEMBER_PROPERTY("RequiredFlags", m_uiRequiredFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(xiiGpuInstanceFlags::Enabled))),
    XII_MEMBER_PROPERTY("ExcludedFlags", m_uiExcludedFlags)->AddAttributes(new xiiDefaultValueAttribute(0U)),
    XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets)->AddAttributes(new xiiDefaultValueAttribute(1024U * 1024U), new xiiClampValueAttribute(1U, 16777216U)),
    XII_MEMBER_PROPERTY("LodHysteresis", m_fLodHysteresis)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0f, 0.49f)),
    XII_MEMBER_PROPERTY("LodScreenScale", m_fLodScreenScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, 1000.0f)),
    XII_MEMBER_PROPERTY("UseViewSelection", m_bUseViewSelection)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVisibilityViewPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (m_uiMaxVisibleMeshlets == 0 || m_uiMaxVisibleMeshlets > 16777216U ||
      !xiiMath::IsFinite(m_fLodHysteresis) || m_fLodHysteresis < 0 || m_fLodHysteresis > 0.49f ||
      !xiiMath::IsFinite(m_fLodScreenScale) || m_fLodScreenScale < 0.001f || m_fLodScreenScale > 1000)
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiResult xiiVisibilityViewPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetCamera() || !context.m_pView->GetGpuInstanceData())
    return context.ReportFailure("Visibility preparation requires a camera and world GPU instances.");
  const auto& view = *context.m_pView;
  xiiFrustum  frustum;
  view.ComputeCullingFrustum(frustum);
  auto snapshot                   = xiiGpuVisibilitySystem::BuildView(view.GetViewProjectionMatrix(xiiCameraEye::Left), frustum,
                                                                      view.GetCamera()->GetCenterPosition(), view.GetRenderResolutionWidth(), view.GetRenderResolutionHeight(), 0,
                                                                      view.GetGpuInstanceData()->GetInstances().GetCount(), m_uiVisibilityMask);
  snapshot.m_uiRequiredFlags      = m_uiRequiredFlags;
  snapshot.m_uiExcludedFlags      = m_uiExcludedFlags;
  snapshot.m_uiGeometryBaseIndex  = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
  snapshot.m_uiMaxVisibleMeshlets = m_uiMaxVisibleMeshlets;
  snapshot.m_ViewportAndHiZ.y *= m_fLodScreenScale;
  const auto token            = static_cast<xiiUInt32>(context.m_Graph.GetFrameIndex() % 65535U);
  snapshot.m_LodParameters    = xiiVec4(m_fLodHysteresis, float(token), float((token + 65534U) % 65535U), 0);
  snapshot.m_uiUseCandidates  = m_bUseViewSelection ? 1 : 0;
  snapshot.m_uiCandidateCount = m_bUseViewSelection ? view.GetGpuInstanceSelection().GetCount() : 0;
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiGpuVisibilityView       m_Snapshot;
    xiiDynamicArray<xiiUInt32> m_Selection;
    xiiRenderGraphBufferHandle m_View, m_Candidates;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Snapshot = snapshot;
      if (m_bUseViewSelection) data.m_Selection = view.GetGpuInstanceSelection();
      if (data.m_Selection.IsEmpty()) data.m_Selection.PushBack(0);
      xiiGALBufferCreationDescription description;
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      description.m_uiSize = sizeof(snapshot);
      description.m_uiElementByteStride = sizeof(snapshot);
      data.m_View = builder.WriteBuffer("Camera Culling Parameters", description, xiiGALResourceStateFlags::CopyDestination);
      description.m_uiSize = data.m_Selection.GetCount() * sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      data.m_Candidates = builder.WriteBuffer("Camera Component Selection", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_View), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_Snapshot), sizeof(data.m_Snapshot)));
      execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Candidates), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Selection.GetData()), data.m_Selection.GetCount() * sizeof(xiiUInt32))); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_View, emitted.first->m_View));
  return context.SetOutput(m_Candidates, emitted.first->m_Candidates);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_VisibilityViewPass);
