#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using xiiVisualizeHandComponentManager = class xiiVisualizeHandComponent;

class XII_GAMEENGINE_DLL xiiVisualizeHandComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVisualizeHandComponent, xiiComponent, xiiVisualizeHandComponentManager);

public:
  xiiVisualizeHandComponent();
  ~xiiVisualizeHandComponent();

protected:
  void Update();
};
