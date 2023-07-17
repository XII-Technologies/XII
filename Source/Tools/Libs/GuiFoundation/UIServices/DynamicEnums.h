#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the xiiDynamicEnum for a specific type.
class XII_GUIFOUNDATION_DLL xiiDynamicEnum
{
public:
  /// \brief Returns a xiiDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static xiiDynamicEnum& GetDynamicEnum(xiiStringView sEnumName);

  /// \brief Returns all enum values and current names.
  const xiiMap<xiiInt32, xiiString>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void SetValueAndName(xiiInt32 iValue, xiiStringView sNewName);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(xiiInt32 iValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(xiiInt32 iValue) const;

  /// \brief Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  const char* GetValueName(xiiInt32 iValue) const;

private:
  xiiMap<xiiInt32, xiiString> m_ValidValues;

  static xiiMap<xiiString, xiiDynamicEnum> s_DynamicEnums;
};
