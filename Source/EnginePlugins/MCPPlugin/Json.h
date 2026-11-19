/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

namespace xiiMCPJson
{
  constexpr xiiUInt32 DefaultMaximumBytes = 256 * 1024;
  constexpr xiiUInt32 MaximumDepth        = 32;

  /// Parse one UTF-8 JSON value. Duplicate keys and embedded NUL characters are rejected.
  [[nodiscard]] XII_MCPPLUGIN_DLL xiiResult Parse(xiiStringView sJson, xiiVariant& out_value, xiiUInt32 uiMaximumBytes = DefaultMaximumBytes);

  /// Encode JSON-compatible variants into compact UTF-8 text. Output is unchanged on failure.
  [[nodiscard]] XII_MCPPLUGIN_DLL xiiResult Write(const xiiVariant& value, xiiStringBuilder& out_sJson, xiiUInt32 uiMaximumBytes = DefaultMaximumBytes);

  [[nodiscard]] XII_MCPPLUGIN_DLL xiiStringView               GetString(const xiiVariantDictionary& object, xiiStringView sKey);
  [[nodiscard]] XII_MCPPLUGIN_DLL const xiiVariantDictionary* GetObject(const xiiVariantDictionary& object, xiiStringView sKey);
  [[nodiscard]] XII_MCPPLUGIN_DLL bool                        ReadUInt(const xiiVariant& value, xiiUInt64& out_uiValue);
} // namespace xiiMCPJson
