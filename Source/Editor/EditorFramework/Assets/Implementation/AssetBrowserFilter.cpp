/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>

xiiQtAssetBrowserFilter::xiiQtAssetBrowserFilter(QObject* pParent) :
  xiiQtAssetFilter(pParent)
{
  Reset();
}

void xiiQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowFiles(true);
  SetShowNonImportableFiles(true);
  SetShowItemsInHiddenFolders(false);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
}

void xiiQtAssetBrowserFilter::UpdateImportExtensions(const xiiSet<xiiString>& extensions)
{
  m_ImportExtensions = extensions;
  if (!m_bShowNonImportableFiles)
    Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInSubFolders = bShow;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetShowFiles(bool bShow)
{
  if (m_bShowFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowFiles = bShow;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetShowNonImportableFiles(bool bShow)
{
  if (m_bShowNonImportableFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowNonImportableFiles = bShow;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInHiddenFolders = bShow;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  Q_EMIT FilterChanged();
  Q_EMIT SortByRecentUseChanged();
}


void xiiQtAssetBrowserFilter::SetTextFilter(const char* szText)
{
  xiiStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_SearchFilter.GetSearchText() == sCleanText)
    return;

  m_SearchFilter.SetSearchText(sCleanText);
  // Clear uses search cache
  m_bUsesSearchActive = false;
  m_bTransitive       = false;
  m_Uses.Clear();

  const char* szRefGuid    = xiiStringUtils::FindSubString_NoCase(szText, "ref:");
  const char* szRefAllGuid = xiiStringUtils::FindSubString_NoCase(szText, "ref-all:");
  if (szRefGuid || szRefAllGuid)
  {
    bool        bTransitive = szRefAllGuid != nullptr;
    const char* szGuid      = szRefAllGuid ? szRefAllGuid + strlen("ref-all:") : szRefGuid + strlen("ref:");
    if (xiiConversionUtils::IsStringUuid(szGuid))
    {
      m_bUsesSearchActive = true;
      m_bTransitive       = bTransitive;
      xiiUuid guid        = xiiConversionUtils::ConvertStringToUuid(szGuid);
      xiiAssetCurator::GetSingleton()->FindAllUses(guid, m_Uses, m_bTransitive);
    }
  }

  Q_EMIT FilterChanged();
  Q_EMIT TextFilterChanged();
}

void xiiQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  xiiStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();
  // The assumption is that only full directory names are set as path filters. Thus, we can ensure they end with a / to make it easier to filter items inside the path.
  if (!sCleanText.IsEmpty() && !sCleanText.EndsWith_NoCase("/"))
  {
    sCleanText.Append("/");
  }

  if (m_sPathFilter == sCleanText)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}


xiiStringView xiiQtAssetBrowserFilter::GetPathFilter() const
{
  if (m_sPathFilter.EndsWith_NoCase("/"))
  {
    return m_sPathFilter.GetSubString(0, m_sPathFilter.GetCharacterCount() - 1);
  }
  return m_sPathFilter;
}

void xiiQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

void xiiQtAssetBrowserFilter::SetFileExtensionFilters(xiiStringView sExtensions)
{
  m_FileExtensions.Clear();

  xiiHybridArray<xiiStringView, 8> filters;
  sExtensions.Split(false, filters, ";", "*", ".");

  xiiStringBuilder tmp;
  for (xiiStringView filter : filters)
  {
    tmp = filter;
    tmp.ToLower();
    m_FileExtensions.Insert(tmp);
  }

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetRequiredTag(xiiStringView sRequiredTag)
{
  xiiStringBuilder tag;

  if (sRequiredTag == "*")
  {
    tag = "*";
  }
  else if (!sRequiredTag.IsEmpty())
  {
    tag.Set(";", sRequiredTag, ";");
  }
  // else: tag stays empty

  if (m_sRequiredTag == tag)
    return;

  m_sRequiredTag = tag;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetTemporaryPinnedItem(xiiStringView sDataDirParentRelativePath)
{
  if (m_sTemporaryPinnedItem == sDataDirParentRelativePath)
    return;

  m_sTemporaryPinnedItem = sDataDirParentRelativePath;
  Q_EMIT FilterChanged();
}

bool xiiQtAssetBrowserFilter::IsAssetFiltered(xiiStringView sDataDirParentRelativePath, bool bIsFolder, const xiiSubAsset* pInfo) const
{
  // ignore all paths leading into the AssetCache
  if (sDataDirParentRelativePath.FindSubString("/AssetCache/"))
    return true;

  // also ignore the AssetCache folder directly
  if (bIsFolder && sDataDirParentRelativePath.GetFileNameAndExtension() == "AssetCache")
    return true;

  if (sDataDirParentRelativePath == m_sTemporaryPinnedItem)
    return false;

  if (!m_bShowFiles && !pInfo && !bIsFolder)
    return true;

  xiiStringBuilder sExt;
  if (m_bShowFiles && !m_bShowNonImportableFiles && !pInfo && !bIsFolder)
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();
    if (!m_ImportExtensions.Contains(sExt))
      return true;
  }

  // if the string is not found in the path, ignore this asset
  if (!sDataDirParentRelativePath.StartsWith(m_sPathFilter))
    return true;

  if (bIsFolder)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (xiiStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
    {
      return true;
    }
  }

  if (m_SearchFilter.IsEmpty() && !m_bShowItemsInSubFolders)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (xiiStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
    {
      return true;
    }
  }
  else if (m_sPathFilter.IsEmpty() && !bIsFolder) // <Root> folder
  {
    if (!m_bShowItemsInSubFolders && m_SearchFilter.IsEmpty())
    {
      return true;
    }
  }

  if (!m_bShowItemsInHiddenFolders)
  {
    // treat folders starting with a dot as hidden folders
    if (sDataDirParentRelativePath.FindSubString("/."))
      return true;

    if (!(m_bUsesSearchActive && !m_SearchFilter.IsEmpty()))
    {
      if (xiiStringUtils::FindSubString_NoCase(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount() + 1, "_data/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
        return true;
    }
  }

  if (!m_SearchFilter.IsEmpty())
  {
    if (m_bUsesSearchActive)
    {
      if (pInfo == nullptr)
        return true;

      if (!m_Uses.Contains(pInfo->m_Data.m_Guid))
        return true;
    }
    else
    {
      // if the string is not found in the path, ignore this asset
      if (m_SearchFilter.PassesFilters(sDataDirParentRelativePath) == false)
      {
        if (pInfo == nullptr)
          return true;

        if (m_SearchFilter.PassesFilters(pInfo->GetName()) == false)
        {
          xiiConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
          if (m_SearchFilter.PassesFilters(m_sTemp) == false)
            return true;

          // we could actually (partially) match the GUID
        }
      }
    }
  }

  // Always show folders on the right
  if (bIsFolder)
  {
    // unless we have a type filter active
    return !m_sTypeFilter.IsEmpty();
  }

  if (!m_FileExtensions.IsEmpty())
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();

    return !m_FileExtensions.Contains(sExt);
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    if (pInfo == nullptr) // if this is no asset, but we have an asset type filter active, always hide this thing
      return true;

    m_sTemp.Set(";", pInfo->m_Data.m_sSubAssetsDocumentTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return true;
  }

  if (pInfo && m_sRequiredTag != "*") // '*' means everything is allowed
  {
    const auto& tags = pInfo->m_pAssetInfo->m_Info->GetAssetsDocumentTags();

    if (m_sRequiredTag.IsEmpty())
    {
      // if the required tag is empty, we only display assets without any tags
      // so the "default tag" (nothing at all) is already a tag for not-tagged items
      // if you really want to see all assets, use * as the required tag
      return !tags.IsEmpty();
    }
    else
    {
      // otherwise search for ";required;" in the tags string (note the semicolons at the start and end as delimiters
      if (tags.FindSubString(m_sRequiredTag) == nullptr)
        return true;
    }
  }

  return false;
}
