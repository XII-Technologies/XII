/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// CPU probe scheduling, independently scoped to this occurrence and view.
class XII_GRAPHICSCORE_DLL xiiDDGIProbeSchedulePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIProbeSchedulePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_PreviousIrradiance{xiiGALResourceFormat::RGBA16Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin m_PreviousDistance{xiiGALResourceFormat::RG32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin m_Activation{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineDataOutputPin   m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>(), true};
  bool                             m_bRequireActivation            = false;
  bool                             m_bSkipOnRayHardware            = false;
  float                            m_fTraceDistanceInProbeSpacings = 4.0f;
  float                            m_fForwardOffset                = 0.0f;
  xiiUInt32                        m_CountX = 24, m_CountY = 12, m_CountZ = 24, m_Budget = 128;
  float                            m_fSpacing = 4, m_fHysteresis = 0.97f, m_fRelocation = 1.8f;
  bool                             m_bUseViewCamera = true;
  bool                             m_bDirectional   = false;
  xiiVec3                          m_CameraPosition = xiiVec3::MakeZero();
  xiiDDGIProbeSchedulePass();
  ~xiiDDGIProbeSchedulePass();
  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult ValidateRenderConfiguration(const xiiRenderConfiguration& configuration, xiiStringBuilder& out_error) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

private:
  xiiDDGISettings                          GetProbeSettings() const;
  xiiSharedPtr<xiiDDGIProbeSchedulerState> m_State;
};
