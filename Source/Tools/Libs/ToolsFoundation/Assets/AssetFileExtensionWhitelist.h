/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A global whitelist for file extension that may be used as certain asset types
///
/// UI elements etc. may use this whitelist to detect whether a selected file is a valid candidate for an asset slot
class XII_TOOLSFOUNDATION_DLL xiiAssetFileExtensionWhitelist
{
public:
  static void AddAssetFileExtension(xiiStringView sAssetType, xiiStringView sAllowedFileExtension);

  static bool IsFileOnAssetWhitelist(xiiStringView sAssetType, xiiStringView sFile);

  static const xiiSet<xiiString>& GetAssetFileExtensions(xiiStringView sAssetType);

private:
  static xiiMap<xiiString, xiiSet<xiiString>> s_ExtensionWhitelist;
};
