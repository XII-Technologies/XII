/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Uploads world-authored initial conditions for the explicitly authored ballistic billboard workload.
class XII_GRAPHICSCORE_DLL xiiParticleBirthUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBirthUploadPass, xiiRenderPipelinePass);

public:
  xiiParticleBirthUploadPass();
  ~xiiParticleBirthUploadPass() override;
  xiiRenderPipelineBufferOutputPin         m_Seeds{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Batches{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Materials{true, xiiGALBindFlags::ShaderResource};
  bool                                     m_bEnabled           = true;
  xiiUInt32                                m_uiMaximumParticles = 1048576;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled ? xiiRenderExtractionCategory::Particles : xiiRenderExtractionCategory::None; }

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
