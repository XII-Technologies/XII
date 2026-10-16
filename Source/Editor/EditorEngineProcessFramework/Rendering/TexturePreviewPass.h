/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

class xiiGALInputLayout;

/// Resource-driven, unlit asset inspection. Each view owns a pass instance; geometry and compiled
/// definitions are shared through ResourceManager. Source textures are acquired on each Build so
/// reloads replace GPU bindings at frame boundaries. The render graph owns frame resources/bindings.
/// View parameters: Texture (resource ID), Channel (0 RGBA, 1 RGB, 2 R, 3 G, 4 B, 5 A,
/// 6 red coverage, 7 alpha coverage), Mip (-1 = automatic, 0..31 clamped to available levels),
/// AlphaThreshold (0..1). Invalid settings return XII_FAILURE before emitting graph work.
/// Supports ordinary 2D/cube textures and single-sample output; arrays/volumes require a slice UI.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiTexturePreviewPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTexturePreviewPass, xiiRenderPipelinePass);

public:
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;

  bool m_bCube = false;

private:
  xiiResult                       EnsureGeometry();
  xiiMeshBufferResourceHandle     m_hGeometry;
  xiiSharedPtr<xiiGALInputLayout> m_pInputLayout;
};
