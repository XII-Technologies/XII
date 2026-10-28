/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// One occurrence's camera sample. No view, world or pass storage is borrowed.
class XII_GRAPHICSCORE_DLL xiiTemporalCameraSnapshot final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalCameraSnapshot, xiiRenderPipelineFrameData);

public:
  struct Sample
  {
    xiiFrameCameraConstants m_Camera;
    xiiMat4                 m_PreviousViewProjection = xiiMat4::MakeIdentity();
    xiiVec2                 m_Jitter = xiiVec2::MakeZero(), m_PreviousJitter = xiiVec2::MakeZero();
    xiiSizeU32              m_Size;
    xiiUInt64               m_uiFrame = 0, m_uiPreviousFrame = 0;
    xiiUInt32               m_uiSequence = 0, m_uiWorldUpdate = 0;
    bool                    m_bEnabled = false, m_bHistoryValid = false;
  };
  xiiTemporalCameraSnapshot(xiiUInt64 setup, const Sample& sample) : xiiRenderPipelineFrameData(setup), m_Sample(sample) {}
  const Sample m_Sample;
};

/// Authors jitter and previous-camera state once, shared explicitly by its consumers.
class XII_GRAPHICSCORE_DLL xiiTemporalCameraPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalCameraPass, xiiRenderPipelinePass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTemporalCameraPass);

public:
  xiiTemporalCameraPass();
  ~xiiTemporalCameraPass() override;
  xiiRenderPipelineDataOutputPin m_Camera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>()};
  /// Enables projection jitter and temporal reconstruction. Camera motion history is always tracked.
  bool                           m_bEnabled = true;
  xiiResult                      Configure(const xiiVariantDictionary& properties) override;
  xiiResult                      Build(xiiRenderPipelinePassContext& context) override;
  void                           InvalidateTemporalHistory() override;

private:
  struct State;
  xiiSharedPtr<State> m_pState;
};
