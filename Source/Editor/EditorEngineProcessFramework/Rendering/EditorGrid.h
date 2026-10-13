/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

class xiiView;

/// Reusable CPU geometry for scene and asset-preview grids. Submit GetLines() through xiiDebugRenderer;
/// the grid owns no GPU resources, passes, world objects, or persistent render data.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorGrid
{
public:
  /// Perspective grids span 50 steps either side of the supplied origin/rotation (scale is ignored).
  /// Orthographic grids fill the viewport at a depth behind its scene and align with the camera axes.
  /// Their spacing increases by powers of ten to retain at least ten pixels between lines and bound
  /// work to at most 2050 intervals per axis. Orthographic global grids snap to the world origin; local grids snap
  /// to the supplied origin. Zero spacing or a zero transform scale disables the grid successfully.
  /// Invalid settings clear the output and return failure. Storage capacity is retained across builds.
  xiiResult Build(const xiiView& view, float fSpacing, const xiiTransform& transform = xiiTransform::MakeIdentity(), bool bGlobal = true);

  /// Valid until the next Build() or destruction. Coordinates are in world space.
  xiiArrayPtr<const xiiDebugRendererLine> GetLines() const { return m_Lines; }

private:
  xiiDynamicArray<xiiDebugRendererLine> m_Lines;
};
