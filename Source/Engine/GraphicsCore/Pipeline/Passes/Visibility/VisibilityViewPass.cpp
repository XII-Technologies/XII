/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/VisibilityViewPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

struct xiiVisibilityViewPass::State : xiiRefCounted
{
  xiiMutex  m_Mutex;
  xiiUInt32 m_uiLastLodToken = 65534U;
  xiiUInt64 m_uiRevision     = 0;
};

xiiVisibilityViewPass::xiiVisibilityViewPass() : m_pState(XII_DEFAULT_NEW(State)) {}
xiiVisibilityViewPass::~xiiVisibilityViewPass() = default;

void xiiVisibilityViewPass::InvalidateTemporalHistory()
{
  XII_LOCK(m_pState->m_Mutex);
  m_pState->m_uiLastLodToken = 65534U;
  ++m_pState->m_uiRevision;
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisibilityViewPass, 1, xiiRTTIDefaultAllocator<xiiVisibilityViewPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InputCandidates", m_InputCandidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask)->AddAttributes(new xiiDefaultValueAttribute(0xFFFFFFFFU)),
    XII_MEMBER_PROPERTY("RequiredFlags", m_uiRequiredFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiUInt32(xiiGpuInstanceFlags::Enabled))),
    XII_MEMBER_PROPERTY("ExcludedFlags", m_uiExcludedFlags)->AddAttributes(new xiiDefaultValueAttribute(0U)),
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
  if (!xiiMath::IsFinite(m_fLodHysteresis) || m_fLodHysteresis < 0 || m_fLodHysteresis > 0.49f || !xiiMath::IsFinite(m_fLodScreenScale) || m_fLodScreenScale < 0.001f || m_fLodScreenScale > 1000)
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
  xiiGpuVisibilityView snapshot;
  snapshot.m_ViewProjectionMatrix          = view.GetViewProjectionMatrix(xiiCameraEye::Left);
  snapshot.m_OcclusionViewProjectionMatrix = snapshot.m_ViewProjectionMatrix;
  for (xiiUInt8 i = 0; i < 6; ++i)
    snapshot.m_FrustumPlanes[i] = frustum.GetPlane(i).GetAsVec4();
  const auto position            = view.GetCamera()->GetCenterPosition();
  snapshot.m_CameraPosition      = xiiVec4(position.x, position.y, position.z, 1);
  snapshot.m_ViewportAndHiZ      = xiiVec4(float(view.GetRenderResolutionWidth()), float(view.GetRenderResolutionHeight()), 0, 0.0005f);
  snapshot.m_uiInstanceCount     = view.GetGpuInstanceData()->GetInstances().GetCount();
  snapshot.m_uiVisibilityMask    = m_uiVisibilityMask;
  snapshot.m_uiRequiredFlags     = m_uiRequiredFlags;
  snapshot.m_uiExcludedFlags     = m_uiExcludedFlags;
  snapshot.m_uiGeometryBaseIndex = xiiGeometryResidencyManager::GetMetadataBaseIndex(context.m_Graph.GetFrameIndex());
  snapshot.m_ViewportAndHiZ.y *= m_fLodScreenScale;
  // Other worlds and views may advance the device frame clock between executions.
  // LOD validity follows this occurrence's published snapshots instead.
  XII_LOCK(m_pState->m_Mutex);
  const auto revision         = m_pState->m_uiRevision;
  const auto token            = (m_pState->m_uiLastLodToken + 1U) % 65535U;
  snapshot.m_LodParameters    = xiiVec4(m_fLodHysteresis, float(token), float(m_pState->m_uiLastLodToken), 0);
  snapshot.m_uiUseCandidates  = m_bUseViewSelection ? 1 : 0;
  snapshot.m_uiCandidateCount = m_bUseViewSelection ? view.GetGpuInstanceSelection().GetCount() : 0;
  const auto supplied         = context.GetInputBuffer(m_InputCandidates);
  if (supplied.IsValid())
  {
    const auto* description = context.m_Graph.GetBufferDescription(supplied);
    if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiUInt32) || description->m_uiSize < sizeof(xiiUInt32) || description->m_uiSize % sizeof(xiiUInt32) != 0 || description->m_uiSize / sizeof(xiiUInt32) > xiiMath::Max(view.GetGpuInstanceData()->GetInstanceCount(), 1U))
      return context.ReportFailure("Supplied candidates require an exact structured uint stream within the world instance capacity; use an invalid-index sentinel for an empty selection.");
    snapshot.m_uiUseCandidates  = 1;
    snapshot.m_uiCandidateCount = static_cast<xiiUInt32>(description->m_uiSize / sizeof(xiiUInt32));
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiGpuVisibilityView       m_Snapshot;
    xiiDynamicArray<xiiUInt32> m_Selection;
    xiiRenderGraphBufferHandle m_View, m_Candidates;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Snapshot = snapshot;
      if (!supplied.IsValid() && m_bUseViewSelection) data.m_Selection = view.GetGpuInstanceSelection();
      if (!supplied.IsValid() && data.m_Selection.IsEmpty()) data.m_Selection.PushBack(xiiInvalidIndex);
      xiiGALBufferCreationDescription description;
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      description.m_uiSize = sizeof(snapshot);
      description.m_uiElementByteStride = sizeof(snapshot);
      data.m_View = builder.WriteBuffer("Camera Culling Parameters", description, xiiGALResourceStateFlags::CopyDestination);
      description.m_uiSize = data.m_Selection.GetCount() * sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      data.m_Candidates = supplied.IsValid() ? builder.ReadBuffer(supplied, xiiGALResourceStateFlags::ShaderResource) : builder.WriteBuffer("Camera Component Selection", description, xiiGALResourceStateFlags::CopyDestination); }, [state = m_pState, supplied, revision](const Data& data, xiiRenderGraphPassContext& execute) {
      execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_View), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_Snapshot), sizeof(data.m_Snapshot)));
      if (!supplied.IsValid()) execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Candidates), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Selection.GetData()), data.m_Selection.GetCount() * sizeof(xiiUInt32)));
      XII_LOCK(state->m_Mutex);
      if (state->m_uiRevision==revision && state->m_uiLastLodToken==static_cast<xiiUInt32>(data.m_Snapshot.m_LodParameters.z))
        state->m_uiLastLodToken = static_cast<xiiUInt32>(data.m_Snapshot.m_LodParameters.y); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_View, emitted.first->m_View));
  return context.SetOutput(m_Candidates, emitted.first->m_Candidates);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_VisibilityViewPass);
