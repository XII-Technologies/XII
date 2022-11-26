#pragma once

#include <Foundation/Communication/Message.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct XII_RMLUIPLUGIN_DLL xiiMsgRmlUiReload : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgRmlUiReload, xiiMessage);
};
