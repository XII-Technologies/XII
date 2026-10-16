/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionsTemporalPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceReflectionsTemporalPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputSSRRawTerm{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputSSRRawTerm{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputSceneDepth{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputGBufferNormal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferNormal{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputGBufferMaterial{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferMaterial{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputGBufferObjectId{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputGBufferObjectId{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineTextureInputPin  m_InputReSTIRDISurfacePrevious{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputReSTIRDISurfacePrevious{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferInputPin   m_InputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_OutputSSRTerm{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputIdentity{xiiGALResourceFormat::RG32UInt, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_OutputConstants;
  xiiRenderPipelineTextureOutputPin m_OutputPreviousReflection{xiiGALResourceFormat::RGBA16Float, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic, true};
  xiiRenderPipelineTextureOutputPin m_OutputPreviousIdentity{xiiGALResourceFormat::RG32UInt, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Automatic, true};
  float                             m_fHistoryWeight   = 0.92f;
  float                             m_fConfidenceDecay = 0.96f;
  float                             m_fDepthThreshold  = 0.02f;
  float                             m_fNormalThreshold = 0.95f;
  xiiScreenSpaceReflectionsTemporalPass();
  ~xiiScreenSpaceReflectionsTemporalPass();
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiResult                                   BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  void                                        InvalidateTemporalHistory() override;

private:
  struct Data;
  struct State;
  void                Setup(Data& data, xiiRenderGraphBuilder& builder);
  void                Execute(const Data& data, xiiRenderGraphPassContext& context);
  xiiUniquePtr<State> m_pState;
};
