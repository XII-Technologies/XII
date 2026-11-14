/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>

/// Shared edge-aware signal filtering and fence-protected occurrence-local history.
class XII_GRAPHICSCORE_DLL xiiTemporalSignalDenoisePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalSignalDenoisePass, xiiRenderPipelinePass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTemporalSignalDenoisePass);

public:
  xiiRenderPipelineTextureInputPin  m_Raw;
  xiiRenderPipelineTextureInputPin  m_Depth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Normal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_Velocity{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_Camera{false, xiiGALBindFlags::UniformBuffer}, m_Global{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_Output;
  xiiRenderPipelineTextureOutputPin m_Previous;
  xiiRenderPipelineTextureOutputPin m_Surface;
  xiiRenderPipelineDataInputPin     m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>(), true};
  xiiRenderPipelineTextureInputPin  m_Material{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_ObjectId{xiiGALResourceFormat::R32UInt, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_SampleDomain{xiiGALResourceFormat::R32UInt, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  bool                              m_bEnabled = true;
  /// Spatial-only filtering requires no motion producer or retained images.
  bool  m_bUseHistory      = true;
  float m_fHistoryWeight   = 0.92F;
  float m_fDepthThreshold  = 0.08F;
  float m_fNormalThreshold = 0.82F;
  float m_fSpatialWeight   = 0.35F;
  ~xiiTemporalSignalDenoisePass() override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;

protected:
  enum class SignalMode : xiiUInt8
  {
    Occlusion,
    Irradiance,
    LocalLighting
  };
  explicit xiiTemporalSignalDenoisePass(SignalMode mode);

private:
  const SignalMode                    m_Mode;
  const xiiEnum<xiiGALResourceFormat> m_Format;
  struct State;
  xiiSharedPtr<State> m_pState;
};
