/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>

class xiiDebugRendererContext;

/// Screen-space geometry shared by the editor's input context and engine overlay.
/// Coordinates and sizes are logical pixels; the renderer applies the device pixel ratio.
struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewportNavigationGizmo
{
  struct Marker
  {
    xiiVec2   m_vPosition;
    float     m_fDepth;
    float     m_fRadius;
    xiiUInt32 m_uiAxis; ///< +X, -X, +Y, -Y, +Z, -Z.
  };

  xiiRectFloat m_Bounds;
  xiiVec2      m_vOrigin;
  Marker       m_Markers[6]; ///< Back to front, also used in reverse order for hit testing.

  xiiViewportNavigationGizmo(const xiiCamera& camera, const xiiVec2& vViewportSize);
  bool     Contains(const xiiVec2& vPosition) const;
  xiiInt32 HitTest(const xiiVec2& vPosition) const;
  void     Draw(const xiiDebugRendererContext& context, float fDevicePixelRatio, xiiInt32 iHoveredAxis, bool bHovered) const;

  static xiiVec3 GetAxisDirection(xiiUInt32 uiAxis);
};
