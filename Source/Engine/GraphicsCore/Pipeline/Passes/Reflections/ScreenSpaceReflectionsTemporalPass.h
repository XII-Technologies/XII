/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Reprojects screen reflections using explicit camera/motion data and private, fence-protected histories.
class XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionsTemporalPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceReflectionsTemporalPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin     m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>()};
  xiiRenderPipelineTextureInputPin  m_InputSSRRawTerm{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferNormal{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferMaterial{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputGBufferObjectId{xiiGALResourceFormat::R32UInt, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin  m_InputDilatedVelocityBuffer{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputSSRTerm{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputIdentity{xiiGALResourceFormat::RG32UInt, true, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_OutputConstants{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_OutputPreviousReflection{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read, true};
  xiiRenderPipelineTextureOutputPin m_OutputPreviousIdentity{xiiGALResourceFormat::RG32UInt, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read, true};
  bool                              m_bEnabled         = true;
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
  struct State;
  xiiSharedPtr<State> m_pState;
};
