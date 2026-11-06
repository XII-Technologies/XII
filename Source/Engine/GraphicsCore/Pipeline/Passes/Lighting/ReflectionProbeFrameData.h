/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Shaders/Pipeline/ReflectionProbeData.h>
#include <utility>

/// Immutable extracted probe bindings. Native images remain alive across resource reload and delayed execution.
class XII_GRAPHICSCORE_DLL xiiReflectionProbeFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeFrameData, xiiRenderPipelineFrameData);

public:
  using Textures = xiiHybridArray<xiiSharedPtr<xiiGALTexture>, XII_MAX_REFLECTION_PROBES>;
  xiiReflectionProbeFrameData(xiiUInt64 setup, xiiRenderGraphBufferHandle records, xiiRenderGraphBufferHandle constants, Textures textures) :
    xiiRenderPipelineFrameData(setup), m_Records(records), m_Constants(constants), m_Textures(std::move(textures)) {}

  const xiiRenderGraphBufferHandle m_Records, m_Constants;
  const Textures                   m_Textures;
};
