/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Foundation/Math/Vec4.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>

/// One selected directional emitter from an extracted view; no borrowed component or world storage.
class XII_GRAPHICSCORE_DLL xiiDirectionalLightFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalLightFrameData, xiiRenderPipelineFrameData);

public:
  xiiDirectionalLightFrameData(xiiUInt64 setup, xiiVec4 directionAndIlluminance, xiiVec4 colorAndRadius, bool castsShadows = false) : xiiRenderPipelineFrameData(setup), m_DirectionAndIlluminance(directionAndIlluminance), m_ColorAndAngularRadius(colorAndRadius), m_bCastsShadows(castsShadows) {}
  const xiiVec4 m_DirectionAndIlluminance, m_ColorAndAngularRadius;
  const bool    m_bCastsShadows;
};
