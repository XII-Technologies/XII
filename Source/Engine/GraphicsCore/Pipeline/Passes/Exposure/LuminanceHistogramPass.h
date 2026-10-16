/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiLuminanceHistogramPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLuminanceHistogramPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_InputHDRSceneColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin m_OutputLuminanceHistogram;

  xiiLuminanceHistogramPass();
  ~xiiLuminanceHistogramPass();
  bool      m_bUseViewDefaults     = false;
  float     m_fMinimumLogLuminance = -12.0f;
  float     m_fMaximumLogLuminance = 20.0f;
  xiiResult Configure(const xiiVariantDictionary& properties) override;

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
