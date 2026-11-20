/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

class xiiWorld;

namespace xiiMCPRuntimeProviders
{
  [[nodiscard]] xiiWorld* FindWorld(const xiiVariantDictionary& arguments);

  template <typename Handle>
  [[nodiscard]] xiiString FormatHandle(Handle handle)
  {
    xiiStringBuilder sId;
    sId.SetFormat("{}", handle.GetInternalID().m_Data);
    return xiiString(sId);
  }
} // namespace xiiMCPRuntimeProviders
