/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/AtmosphereLookup.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <utility>

/// Frame-specific binding for immutable, physically matching lookup images.
class XII_GRAPHICSCORE_DLL xiiAtmosphereFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereFrameData, xiiRenderPipelineFrameData);

public:
  xiiAtmosphereFrameData(xiiUInt64 setup, xiiSharedPtr<const xiiAtmosphereLookup> lookup, xiiRenderGraphBufferHandle constants) :
    xiiRenderPipelineFrameData(setup), m_Lookup(std::move(lookup)), m_Constants(constants)
  {
    XII_ASSERT_DEV(m_Lookup != nullptr, "Atmosphere frame bindings require an immutable lookup owner.");
  }

  const xiiSharedPtr<const xiiAtmosphereLookup> m_Lookup;
  const xiiRenderGraphBufferHandle              m_Constants;
};
