/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderGraph.h>

namespace xiiDebugRendererInternal
{
  /// Reuses matching scene depth, or resamples it to the display target for native-resolution debug drawing.
  xiiRenderGraphTextureHandle PrepareDepth(xiiRenderGraph& graph, xiiRenderGraphTextureHandle depth, xiiRenderGraphTextureHandle color);
} // namespace xiiDebugRendererInternal
