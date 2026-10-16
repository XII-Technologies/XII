/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiSensorOutputPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSensorOutputPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferNormal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferAlbedo{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameGlobal{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_OutputSensorOutput{xiiGALResourceFormat::RGBA32Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_OutputSensorOutputConstants;
  xiiRenderPipelineTextureOutputPin m_OutputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};

  xiiSensorOutputPass();
  ~xiiSensorOutputPass();
  bool                        m_bUseViewProfile = false;
  xiiSensorProfile            m_Profile;
  xiiSharedPtr<xiiGALTexture> GetViewOutput(xiiSensorProfileHandle profile) const;
  void                        InvalidateTemporalHistory() override;

  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  struct Data;
  void Setup(Data& data, xiiRenderGraphBuilder& builder);
  void Execute(const Data& data, xiiRenderGraphPassContext& context);
  struct State;
  xiiUniquePtr<State> m_pState;
};
