/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Snapshots integration parameters; owns no particle state or particle lifecycle.
class XII_GRAPHICSCORE_DLL xiiParticleIntegrationConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleIntegrationConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_OutputConstants{false, xiiGALBindFlags::UniformBuffer};
  bool                             m_bUseWorldTime    = false;
  float                            m_fDeltaTimeS      = 1.0f / 60.0f;
  float                            m_fGravityScale    = 1.0f;
  float                            m_fDragCoefficient = 0.05f;
  float                            m_fTurbulenceScale = 0.0f;
  xiiVec3                          m_WindVelocity     = xiiVec3::MakeZero();
  xiiUInt32                        m_uiActiveCount    = 0;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
