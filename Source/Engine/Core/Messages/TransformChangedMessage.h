/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Messaging/Message.h>

struct XII_CORE_DLL xiiMsgTransformChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgTransformChanged, xiiMessage);

  xiiTransformReal m_OldGlobalTransform;
  xiiTransformReal m_NewGlobalTransform;
};
