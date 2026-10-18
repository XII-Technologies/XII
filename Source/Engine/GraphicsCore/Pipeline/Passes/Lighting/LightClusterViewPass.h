/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Captures a view or explicit projection and bounded cluster layout without scene extraction.
class XII_GRAPHICSCORE_DLL xiiLightClusterViewPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightClusterViewPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataOutputPin m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  bool                           m_bUseViewCamera = true, m_bLogarithmicDepth = true;
  xiiMat4                        m_InverseViewProjection = xiiMat4::MakeIdentity();
  xiiVec3                        m_CameraPosition = xiiVec3::MakeZero(), m_CameraForward = xiiVec3(0, 0, 1);
  float                          m_fNearDistance = 0.1f, m_fFarDistance = 1000;
  xiiUInt32                      m_uiWidth = 0, m_uiHeight = 0, m_uiTileSize = 16, m_uiDepthSlices = 24, m_uiMaxLightsPerCluster = 128;
  xiiUInt32                      m_uiMaximumClusters = 262144, m_uiMaximumIndices = 4194304;
  xiiResult                      Configure(const xiiVariantDictionary& properties) override;
  xiiResult                      Build(xiiRenderPipelinePassContext& context) override;
};
