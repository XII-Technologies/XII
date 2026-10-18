/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <Shaders/Pipeline/Passes/Lighting/LightClusterConstants.h>

/// Immutable camera/layout policy for this graph setup; contains no scene objects or light registry.
class XII_GRAPHICSCORE_DLL xiiLightClusterFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightClusterFrameData, xiiRenderPipelineFrameData);

public:
  xiiLightClusterFrameData(xiiUInt64 setup, const xiiLightClusterConstants& constants) : xiiRenderPipelineFrameData(setup), m_Constants(constants) {}
  const xiiLightClusterConstants m_Constants;
};
