/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkError.h>
#include <Foundation/Strings/StringView.h>

/// A name in the current OS user's local IPC namespace.
class XII_FOUNDATION_DLL xiiNetworkLocalEndpoint
{
public:
  /// Accepts 1 to 48 lowercase ASCII letters, digits, hyphens or underscores.
  [[nodiscard]] static xiiNetworkError Make(xiiStringView sName, xiiNetworkLocalEndpoint& out_endpoint);
  [[nodiscard]] bool                   IsValid() const { return m_szName[0] != '\0'; }
  [[nodiscard]] xiiStringView          GetName() const { return m_szName; }
  bool                                 operator==(const xiiNetworkLocalEndpoint&) const = default;

private:
  char m_szName[49] = {};
};
