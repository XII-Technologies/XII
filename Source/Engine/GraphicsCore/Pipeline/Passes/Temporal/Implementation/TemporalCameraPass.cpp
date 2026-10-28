/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalCameraSnapshot, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalCameraPass, 1, xiiRTTIDefaultAllocator<xiiTemporalCameraPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TemporalCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Temporal") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct xiiTemporalCameraPass::State : xiiRefCounted
{
  xiiMutex                          m_Mutex;
  xiiTemporalCameraSnapshot::Sample m_Last;
  xiiUInt64                         m_uiRevision = 0;
  bool                              m_bValid     = false;
};

xiiTemporalCameraPass::xiiTemporalCameraPass() : m_pState(XII_DEFAULT_NEW(State)) {}
xiiTemporalCameraPass::~xiiTemporalCameraPass() = default;

void xiiTemporalCameraPass::InvalidateTemporalHistory()
{
  XII_LOCK(m_pState->m_Mutex);
  m_pState->m_bValid = false;
  ++m_pState->m_uiRevision;
}

xiiResult xiiTemporalCameraPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiTemporalCameraPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetCamera() || context.m_pView->GetCamera()->GetCameraMode() == xiiCameraMode::None)
    return context.ReportFailure("Temporal camera preparation requires a view camera.");
  const auto&                       view = *context.m_pView;
  xiiTemporalCameraSnapshot::Sample sample;
  sample.m_Size = {view.GetRenderResolutionWidth(), view.GetRenderResolutionHeight()};
  if (!sample.m_Size.width || !sample.m_Size.height)
    return context.ReportFailure("Temporal camera preparation requires a nonempty viewport.");
  sample.m_Camera        = xiiSnapshotCameraConstants(view);
  sample.m_uiFrame       = context.m_Graph.GetFrameIndex();
  sample.m_bEnabled      = m_bEnabled;
  sample.m_uiWorldUpdate = view.GetWorld() ? view.GetWorld()->GetUpdateCounter() : static_cast<xiiUInt32>(sample.m_uiFrame);
  XII_LOCK(m_pState->m_Mutex);
  const auto  revision   = m_pState->m_uiRevision;
  const auto& last       = m_pState->m_Last;
  sample.m_bHistoryValid = m_pState->m_bValid && last.m_bEnabled == m_bEnabled && last.m_Size == sample.m_Size && last.m_uiFrame < sample.m_uiFrame && last.m_uiWorldUpdate + 1 == sample.m_uiWorldUpdate;
  sample.m_uiSequence    = m_bEnabled && sample.m_bHistoryValid ? (last.m_uiSequence + 1) % 8 : 0;
  if (m_bEnabled)
  {
    const auto halton = [](xiiUInt32 index, xiiUInt32 base) {
      float value = 0, factor = 1;
      while (index)
      {
        factor /= float(base);
        value += factor * float(index % base);
        index /= base;
      }
      return value;
    };
    sample.m_Jitter                               = xiiVec2((halton(sample.m_uiSequence + 1, 2) - 0.5f) * 2.0f / sample.m_Size.width,
                                                            (halton(sample.m_uiSequence + 1, 3) - 0.5f) * 2.0f / sample.m_Size.height);
    auto jitter                                   = xiiMat4::MakeIdentity();
    jitter.Element(3, 0)                          = sample.m_Jitter.x;
    jitter.Element(3, 1)                          = sample.m_Jitter.y;
    sample.m_Camera.m_ViewProjectionMatrix        = jitter * sample.m_Camera.m_ViewProjectionMatrix;
    sample.m_Camera.m_InverseViewProjectionMatrix = sample.m_Camera.m_ViewProjectionMatrix.GetInverse();
  }
  sample.m_PreviousViewProjection = sample.m_bHistoryValid ? last.m_Camera.m_ViewProjectionMatrix : sample.m_Camera.m_ViewProjectionMatrix;
  sample.m_PreviousJitter         = sample.m_bHistoryValid ? last.m_Jitter : sample.m_Jitter;
  sample.m_uiPreviousFrame        = sample.m_bHistoryValid ? last.m_uiFrame : sample.m_uiFrame;
  // A recording owns its sample and state. Old recordings cannot resurrect a cut or reload.
  context.m_Graph.AddPass<xiiTemporalCameraSnapshot::Sample>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder&) { data = sample; }, [state = m_pState, revision](const auto& data, xiiRenderGraphPassContext&) {
      XII_LOCK(state->m_Mutex);
      if (state->m_uiRevision == revision && (!state->m_bValid || state->m_Last.m_uiFrame < data.m_uiFrame))
      {
        state->m_Last = data;
        state->m_bValid = true;
      } }, true);
  return context.SetOutput(m_Camera, XII_DEFAULT_NEW(xiiTemporalCameraSnapshot, context.m_Graph.GetSetupSerial(), sample));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_Implementation_TemporalCameraPass);
