/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// Stores the valid values and names for 'dynamic' bitflags.
///
/// The names and valid values for dynamic bitflags may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicBitflags() to create or get the xiiDynamicBitflags for a specific type.
class XII_GUIFOUNDATION_DLL xiiDynamicBitflags
{
public:
  /// Returns a xiiDynamicBitflags under the given name. Creates a new one, if the name has not been used before.
  static xiiDynamicBitflags& GetDynamicBitflags(xiiStringView sName);

  /// Returns all bitflag values and current names.
  const xiiMap<xiiUInt64, xiiString>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets stored values.
  void Clear();

  /// Sets the name for the given bit position.
  void SetValueAndName(xiiUInt32 uiBitPos, xiiStringView sName);

  /// Removes a value, if it exists.
  void RemoveValue(xiiUInt32 uiBitPos);

  /// Returns whether a certain value is known.
  bool IsValueValid(xiiUInt32 uiBitPos) const;

  /// Returns the name for the given value
  bool TryGetValueName(xiiUInt32 uiBitPos, xiiStringView& out_sName) const;

private:
  xiiMap<xiiUInt64, xiiString> m_ValidValues;

  static xiiMap<xiiString, xiiDynamicBitflags> s_DynamicBitflags;
};
