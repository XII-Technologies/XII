#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiMsgOnlyApplyToObject : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgOnlyApplyToObject, xiiMessage);

  xiiGameObjectHandle m_hObject;
};
