/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the xiiDynamicEnum for a specific type.
class XII_GUIFOUNDATION_DLL xiiDynamicStringEnum
{
public:
  /// Returns a xiiDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  ///
  /// Calls s_RequestUnknownCallback, if the requested enum is not known yet, which will try to load the data.
  static xiiDynamicStringEnum& GetDynamicEnum(xiiStringView sEnumName);

  static xiiDynamicStringEnum& CreateDynamicEnum(xiiStringView sEnumName);

  /// Removes the entire enum with the given name.
  static void RemoveEnum(xiiStringView sEnumName);

  /// Returns all enum values and current names.
  const xiiHybridArray<xiiString, 16>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets the internal data.
  void Clear();

  /// Sets the name for the given enum value.
  void AddValidValue(xiiStringView sValue, bool bSortValues = false);

  /// Removes a certain enum value, if it exists.
  void RemoveValue(xiiStringView sValue);

  /// Returns whether a certain value is known.
  bool IsValueValid(xiiStringView sValue) const;

  /// Sorts existing values alphabetically
  void SortValues();

  /// If set to non-empty, the user can easily edit this enum through a simple dialog and the values will be saved in this file.
  ///
  /// Empty by default, as most dynamic enums need to be set up according to other criteria.
  void SetStorageFile(xiiStringView sFile) { m_sStorageFile = sFile; }

  /// The file where values will be stored.
  xiiStringView GetStorageFile() const { return m_sStorageFile; }

  /// If specified, the widget shows an "edit" option, which will run xiiActionManager::ExecuteAction(sCmd, value).
  ///
  /// This is meant to be used to open existing config dialogs.
  /// There is currently no way to report back a selection, so after making changes, the user has to make another selection.
  void              SetEditCommand(xiiStringView sCmd, const xiiVariant& value);
  xiiStringView     GetEditCommand() const { return m_sEditCommand; }
  const xiiVariant& GetEditCommandValue() const { return m_EditCommandValue; }

  void ReadFromStorage();

  void SaveToStorage();

  /// Invoked by GetDynamicEnum() for enums that are unkonwn at that time.
  ///
  /// Can be used to on-demand load those values, before GetDynamicEnum() returns.
  static xiiDelegate<void(xiiStringView sEnumName, xiiDynamicStringEnum& e)> s_RequestUnknownCallback;

private:
  xiiHybridArray<xiiString, 16> m_ValidValues;
  xiiString                     m_sStorageFile;

  xiiString  m_sEditCommand;
  xiiVariant m_EditCommandValue;

  static xiiMap<xiiString, xiiDynamicStringEnum> s_DynamicEnums;
};
