/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

inline xiiUuid xiiScreenEffectsDefinitionId()
{
  return xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenEffects");
}

/// Versioned conversion of the former composite node to an ordinary authored resource.
/// This does not construct its replacement topology; the resource owns that definition.
inline xiiUuid xiiDebugVisualizationDefinitionId()
{
  return xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.DebugVisualization");
}

inline xiiUuid xiiOpaqueCompositionDefinitionId()
{
  return xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.OpaqueComposition");
}

inline xiiUuid xiiTransparencyResolveDefinitionId()
{
  return xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.TransparencyResolve");
}

void xiiMigrateLegacyRenderPipelineDefinitions(xiiRenderPipelineResourceDescriptor& descriptor);
