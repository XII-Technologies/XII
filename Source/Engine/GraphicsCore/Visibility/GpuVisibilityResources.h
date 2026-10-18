/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

/// Frame-local compiled draw inputs. No scene lifecycle, storage or execution ownership.
struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityOutputs
{
  /// Current/previous pose snapshot shared by all native vertex consumers in this graph.
  xiiRenderGraphBufferHandle m_hSkinningMatrices;
  xiiRenderGraphBufferHandle m_hSceneInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
  xiiRenderGraphBufferHandle m_hVisibleMeshlets;
  xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
  xiiRenderGraphBufferHandle m_hIndirectCommands;
  xiiRenderGraphBufferHandle m_hIndirectCommandCount;
  /// Capacity of the compact visible-meshlet stream, shared by mesh and indexed consumers.
  xiiUInt32 m_uiMaxVisibleMeshlets = 0U;
};
