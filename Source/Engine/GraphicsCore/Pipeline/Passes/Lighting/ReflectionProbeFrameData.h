/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/Lighting/EnvironmentMap.h>
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
  using Environments = xiiHybridArray<xiiSharedPtr<const xiiEnvironmentMap>, XII_MAX_REFLECTION_PROBES>;
  xiiReflectionProbeFrameData(xiiUInt64 setup, xiiRenderGraphBufferHandle records, xiiRenderGraphBufferHandle constants, Textures textures, Environments environments = {}, xiiSharedPtr<const xiiEnvironmentMap> sky = {}, xiiRenderGraphBufferHandle skyConstants = {}) :
    xiiRenderPipelineFrameData(setup), m_Records(records), m_Constants(constants), m_Textures(std::move(textures)), m_Environments(std::move(environments)), m_Sky(std::move(sky)), m_SkyConstants(skyConstants)
  {
    XII_ASSERT_DEV(m_Textures.GetCount() == m_Environments.GetCount(), "Probe images must carry their immutable environment owner and readiness.");
  }

  const xiiRenderGraphBufferHandle m_Records, m_Constants;
  const Textures                   m_Textures;
  const Environments               m_Environments;
  const xiiSharedPtr<const xiiEnvironmentMap> m_Sky;
  const xiiRenderGraphBufferHandle            m_SkyConstants;
};
