#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

xiiMap<xiiString, xiiSet<xiiString>> xiiAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void xiiAssetFileExtensionWhitelist::AddAssetFileExtension(const char* szAssetType, const char* szAllowedFileExtension)
{
  xiiStringBuilder sLowerType = szAssetType;
  sLowerType.ToLower();

  xiiStringBuilder sLowerExt = szAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool xiiAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(const char* szAssetType, const char* szFile)
{
  xiiStringBuilder sLowerExt = xiiPathUtils::GetFileExtension(szFile);
  sLowerExt.ToLower();

  xiiStringBuilder sLowerType = szAssetType;
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

const xiiSet<xiiString>& xiiAssetFileExtensionWhitelist::GetAssetFileExtensions(const char* szAssetType)
{
  return s_ExtensionWhitelist[szAssetType];
}
