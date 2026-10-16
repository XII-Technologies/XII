/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Integrates an explicitly supplied particle buffer. Its owner retains state and lifetime.
class XII_GRAPHICSCORE_DLL xiiParticleIntegratePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleIntegratePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_InputParticles{false, xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin  m_InputConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_OutputParticles{false, xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiUInt32                        m_uiParticleCount = 0; // Zero dispatches the supplied buffer's full capacity.
  bool                             m_bBypass         = false;
  xiiParticleIntegratePass();
  ~xiiParticleIntegratePass();
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return m_bBypass ? xiiRenderPipelineDeviceFeature::None : xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
