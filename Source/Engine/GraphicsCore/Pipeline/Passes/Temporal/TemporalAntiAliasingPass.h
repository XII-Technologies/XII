/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiTemporalAntiAliasingPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalAntiAliasingPass, xiiRenderPipelinePass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTemporalAntiAliasingPass);

public:
  xiiRenderPipelineDataInputPin     m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>(), true};
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  /// Optional transmittance of transparent coverage; untrusted pixels also reject history after disappearance.
  xiiRenderPipelineTextureInputPin  m_HistoryTrust{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputTAAResolved{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputPreviousColor{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic, true};

  xiiTemporalAntiAliasingPass();
  ~xiiTemporalAntiAliasingPass();
  float     m_fBaseBlendAlpha   = 0.08f;
  float     m_fMotionBlendScale = 12.0f;
  float     m_fDepthThreshold   = 0.01f;
  bool      m_bEnabled          = true;
  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct State;
  xiiSharedPtr<State> m_pState;
};
