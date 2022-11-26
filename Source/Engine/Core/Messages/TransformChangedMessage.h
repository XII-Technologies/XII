#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiMsgTransformChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgTransformChanged, xiiMessage);

  xiiTransform m_OldGlobalTransform;
  xiiTransform m_NewGlobalTransform;
};
