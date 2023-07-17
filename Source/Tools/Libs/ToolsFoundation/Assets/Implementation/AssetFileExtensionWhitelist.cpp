#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

xiiMap<xiiString, xiiSet<xiiString>> xiiAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void xiiAssetFileExtensionWhitelist::AddAssetFileExtension(xiiStringView sAssetType, xiiStringView sAllowedFileExtension)
{
  xiiStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  xiiStringBuilder sLowerExt = sAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool xiiAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(xiiStringView sAssetType, xiiStringView sFile)
{
  xiiStringBuilder sLowerExt = sFile.GetFileExtension();
  sLowerExt.ToLower();

  xiiStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  xiiHybridArray<xiiString, 16> Types;
  sLowerType.Split(false, Types, ";");

  for (const auto& filter : Types)
  {
    if (s_ExtensionWhitelist[filter].Contains(sLowerExt))
      return true;
  }

  return false;
}

const xiiSet<xiiString>& xiiAssetFileExtensionWhitelist::GetAssetFileExtensions(xiiStringView sAssetType)
{
  return s_ExtensionWhitelist[sAssetType];
}
