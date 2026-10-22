/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Immutable directional mesh shading constants from authored properties and supplied projections.
class XII_GRAPHICSCORE_DLL xiiDirectionalMeshConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalMeshConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_ShadowMap{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineDataInputPin    m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiMat4                          m_ShadowProjection        = xiiMat4::MakeIdentity();
  xiiVec4                          m_SunDirectionIntensity   = xiiVec4(0, 0, -1, 1);
  xiiVec3                          m_AmbientColor            = xiiVec3(0.12f, 0.15f, 0.22f);
  float                            m_fSelectionPulseStrength = 0, m_fShadowWorldUnitsPerTexel = 0;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
