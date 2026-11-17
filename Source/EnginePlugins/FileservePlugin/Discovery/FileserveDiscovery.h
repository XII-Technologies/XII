/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/FileservePluginDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Time.h>

namespace xiiFileserveDiscovery
{
  /// Sends one authenticated address advertisement to a waiting device and requires its acknowledgement.
  xiiResult Send(xiiStringView sDevice, xiiUInt16 uiServerPort, xiiArrayPtr<const xiiStringBuilder> addresses, xiiTime timeout);

  /// Blocks only at the discovery boundary. The returned addresses have been validated as numeric IP literals.
  xiiResult Receive(xiiUInt16& out_uiServerPort, xiiDynamicArray<xiiString>& out_addresses, xiiTime timeout);
} // namespace xiiFileserveDiscovery
