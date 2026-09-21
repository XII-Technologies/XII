/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// Common message for components that can be toggled between playing and paused states
struct XII_CORE_DLL xiiMsgSetPlaying : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetPlaying, xiiMessage);

  bool m_bPlay = true;
};

/// Common message for components that can or need to be canceled immediately
struct XII_CORE_DLL xiiMsgInterruptPlaying : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgInterruptPlaying, xiiMessage);
};

/// Basic message to set some generic parameter to a float value.
struct XII_CORE_DLL xiiMsgSetFloatParameter : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetFloatParameter, xiiMessage);

  xiiString m_sParameterName;
  float     m_fValue = 0.0f;
};

/// Basic message to set some generic parameter to a double value.
struct XII_CORE_DLL xiiMsgSetDoubleParameter : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetDoubleParameter, xiiMessage);

  xiiString m_sParameterName;
  double    m_fValue = 0.0;
};

/// Basic message to set some generic parameter to a xiiReal value.
struct XII_CORE_DLL xiiMsgSetRealParameter : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetRealParameter, xiiMessage);

  xiiString m_sParameterName;
  xiiReal   m_fValue = static_cast<xiiReal>(0);
};

/// For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom messages for more elaborate cases where a string is not sufficient
/// information.
struct XII_CORE_DLL xiiMsgGenericEvent : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgGenericEvent, xiiEventMessage);

  /// A custom string to identify the intent.
  xiiHashedString m_sMessage;
  xiiVariant      m_Value;
};

/// Sent when an animation reached its end (either forwards or backwards playing)
///
/// This is sent regardless of whether the animation is played once, looped or back and forth,
/// ie. it should be sent at each 'end' point, even when it then starts another cycle.
struct XII_CORE_DLL xiiMsgAnimationReachedEnd : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationReachedEnd, xiiEventMessage);
};
