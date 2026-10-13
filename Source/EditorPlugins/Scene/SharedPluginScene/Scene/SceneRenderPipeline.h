/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

/// Editable, serializable Editor scene composition using native Engine stages and shared scene
/// passes. Definition construction does not access GAL. Mutable execution remains per view.
namespace xiiSceneRenderPipeline
{
  XII_SHAREDPLUGINSCENE_DLL xiiResult                       GetDefaultDescriptor(xiiRenderPipelineResourceDescriptor& out_descriptor);
  XII_SHAREDPLUGINSCENE_DLL xiiRenderPipelineResourceHandle GetDefaultResource();
} // namespace xiiSceneRenderPipeline
