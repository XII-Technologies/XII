/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Stabilized directional cascades from a setup-time view camera snapshot.
/// Owns neither light components nor shadow atlas residency/raster work.
class XII_GRAPHICSCORE_DLL xiiShadowCascadePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCascadePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineDataOutputPin   m_Snapshot{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>()};
  xiiShadowCascadeSettings         m_Settings;
  xiiVec3                          m_LightDirection = xiiVec3(0, 0, -1);

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
