#pragma once

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/MsgExtractImguiUpdate.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractImguiUpdate);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractImguiUpdate, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_MsgExtractImguiUpdate);
