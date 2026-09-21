/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/World.h>

struct xiiEventMessage;

/// Base class for components that want to handle 'event messages'
///
/// Event messages are messages that are 'broadcast' to indicate something happened on a component,
/// e.g. a trigger that got activated or an animation that finished playing. These messages are 'bubbled up'
/// the object hierarchy to the closest parent object that holds a xiiEventMessageHandlerComponent.
class XII_CORE_DLL xiiEventMessageHandlerComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiEventMessageHandlerComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiEventMessageHandlerComponent

public:
  /// Keep the constructor private or protected in derived classes, so it cannot be called manually.
  xiiEventMessageHandlerComponent();
  ~xiiEventMessageHandlerComponent();

  /// Sets the debug output object flag. The effect is type specific, most components will not do anything different.
  void SetDebugOutput(bool bEnable);

  /// Gets the debug output object flag.
  bool GetDebugOutput() const;

  /// Registers or de-registers this component as a global event handler.
  void SetGlobalEventHandlerMode(bool bEnable); // [ property ]

  /// Returns whether this component is registered as a global event handler.
  bool GetGlobalEventHandlerMode() const { return m_bIsGlobalEventHandler; } // [ property ]

  /// Sets whether unhandled event messages should be passed to parent objects or not.
  void SetPassThroughUnhandledEvents(bool bPassThrough);                               // [ property ]
  bool GetPassThroughUnhandledEvents() const { return m_bPassThroughUnhandledEvents; } // [ property ]

  /// Returns all global event handler for the given world.
  static xiiArrayPtr<xiiComponentHandle> GetAllGlobalEventHandler(const xiiWorld* pWorld);

  static void ClearGlobalEventHandlersForWorld(const xiiWorld* pWorld);

private:
  bool m_bDebugOutput                = false;
  bool m_bIsGlobalEventHandler       = false;
  bool m_bPassThroughUnhandledEvents = false;
};
