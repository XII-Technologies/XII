/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalLightFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Packs focused environment illumination. A connected snapshot replaces the authored directional emitter.
class XII_GRAPHICSCORE_DLL xiiEnvironmentLightConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEnvironmentLightConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_Directional{xiiGetStaticRTTI<xiiDirectionalLightFrameData>(), true};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiVec3                          m_Direction = xiiVec3(0, 0, -1), m_Color = xiiVec3(1), m_Ambient = xiiVec3(0.03f, 0.035f, 0.04f);
  float                            m_fIlluminance = 0, m_fAngularRadius = 0;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
