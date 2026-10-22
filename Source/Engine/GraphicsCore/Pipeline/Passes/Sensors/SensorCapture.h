/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiSensorCaptureResult;
/// Common calibration and per-occurrence output lifetime, not an executable sensor algorithm.
class XII_GRAPHICSCORE_DLL xiiSensorCapture : public xiiRenderPipelinePass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSensorCapture);
  XII_ADD_DYNAMIC_REFLECTION(xiiSensorCapture, xiiRenderPipelinePass);

public:
  ~xiiSensorCapture();
  xiiRenderPipelineTextureInputPin            m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin            m_InputVelocity{xiiGALResourceFormat::RG16Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_InputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_InputPerFrameGlobal{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin           m_OutputSensor{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin            m_OutputConstants{false, xiiGALBindFlags::UniformBuffer};
  bool                                        m_bUseViewProfile = false;
  xiiSensorCalibration                        m_Calibration;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  void                                        InvalidateTemporalHistory() override;
  xiiSharedPtr<xiiGALTexture>                 GetViewOutput(xiiSensorProfileHandle profile) const;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

protected:
  explicit xiiSensorCapture(xiiSensorType::Enum type);
  struct Capture
  {
    bool                   m_bActive = false;
    xiiSensorProfile       m_Profile;
    xiiSensorProfileHandle m_ViewProfile;
  };
  struct Inputs
  {
    xiiRenderGraphTextureHandle m_Radiance, m_Normal, m_Albedo;
  };
  xiiResult   Prepare(xiiRenderPipelinePassContext& context, Capture& capture);
  xiiResult   Record(xiiRenderPipelinePassContext& context, const Capture& capture, const Inputs& inputs, xiiStringView shader);
  static bool IsColorInput(const xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle image, xiiUInt32 channels);

private:
  const xiiSensorType::Enum            m_Type;
  xiiSharedPtr<xiiGALTexture>          m_Output;
  xiiSharedPtr<xiiSensorCaptureResult> m_Capture;
};
