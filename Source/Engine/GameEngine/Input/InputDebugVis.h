/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

class xiiDebugRendererContext;
class xiiVirtualThumbStick;

namespace xiiInputDebugVis
{
  /// \brief Renders a debug visualization of the given thumbstick using the 2D screen space debug render functions.
  XII_GAMEENGINE_DLL void DebugRender(const xiiDebugRendererContext& context, const xiiVec2& vResolution, const xiiVirtualThumbStick& stick);

}; // namespace xiiInputDebugVis
