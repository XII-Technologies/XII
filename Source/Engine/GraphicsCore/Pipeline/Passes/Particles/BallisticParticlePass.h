/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Foundation/Math/Vec3.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Evaluates immutable birth conditions at their extracted age. This is analytic ballistic motion, not stepped integration.
class XII_GRAPHICSCORE_DLL xiiBallisticParticlePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBallisticParticlePass, xiiRenderPipelinePass);

public:
  xiiBallisticParticlePass();
  ~xiiBallisticParticlePass() override;
  xiiRenderPipelineBufferInputPin             m_Seeds{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Batches{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin            m_Particles{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiVec3                                     m_Acceleration       = xiiVec3(0, 0, -9.81f);
  xiiUInt32                                   m_uiMaximumParticles = 1048576;
  bool                                        m_bAllowAbsentSeeds  = false;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
