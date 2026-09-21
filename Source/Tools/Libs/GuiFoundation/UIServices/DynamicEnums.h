/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

/// Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the xiiDynamicEnum for a specific type.
class XII_GUIFOUNDATION_DLL xiiDynamicEnum
{
public:
  /// Returns a xiiDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static xiiDynamicEnum& GetDynamicEnum(xiiStringView sEnumName);

  /// Returns all enum values and current names.
  const xiiMap<xiiInt32, xiiString>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets the internal data.
  void Clear();

  /// Sets the name for the given enum value.
  void SetValueAndName(xiiInt32 iValue, xiiStringView sNewName);

  /// Removes a certain enum value, if it exists.
  void RemoveValue(xiiInt32 iValue);

  /// Returns whether a certain value is known.
  bool IsValueValid(xiiInt32 iValue) const;

  /// Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  xiiStringView GetValueName(xiiInt32 iValue) const;

  /// If specified, the widget shows an "edit" option, which will run xiiActionManager::ExecuteAction(sCmd, value).
  ///
  /// This is meant to be used to open existing config dialogs.
  /// There is currently no way to report back a selection, so after making changes, the user has to make another selection.
  void              SetEditCommand(xiiStringView sCmd, const xiiVariant& value);
  xiiStringView     GetEditCommand() const { return m_sEditCommand; }
  const xiiVariant& GetEditCommandValue() const { return m_EditCommandValue; }

private:
  xiiMap<xiiInt32, xiiString> m_ValidValues;

  xiiString  m_sEditCommand;
  xiiVariant m_EditCommandValue;

  static xiiMap<xiiString, xiiDynamicEnum> s_DynamicEnums;
};
