#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief Base class for all components that implement 'non player character' behavior.
/// Ie, game logic for how an AI controlled character should behave (state machines etc)
class XII_GAMEENGINE_DLL xiiNpcComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiNpcComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiNpcComponent

public:
  xiiNpcComponent();
  ~xiiNpcComponent();
};
