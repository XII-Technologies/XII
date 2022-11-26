#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


xiiQtAssetBrowserFilter::xiiQtAssetBrowserFilter(QObject* pParent) :
  xiiQtAssetFilter(pParent)
{
}


void xiiQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowItemsInHiddenFolders(false);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
}

void xiiQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_bShowItemsInSubFolders = bShow;

  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

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

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;
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

  if (m_sPathFilter == sCleanText)
    return;

  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}

void xiiQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

bool xiiQtAssetBrowserFilter::IsAssetFiltered(const xiiSubAsset* pInfo) const
{
  if (!m_sPathFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (!pInfo->m_pAssetInfo->m_sDataDirParentRelativePath.StartsWith_NoCase(m_sPathFilter))
      return true;

    if (!m_bShowItemsInSubFolders)
    {
      // do we find another path separator after the prefix path?
      // if so, there is a sub-folder, and thus we ignore it
      if (xiiStringUtils::FindSubString(pInfo->m_pAssetInfo->m_sDataDirParentRelativePath + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
        return true;
    }
  }

  if (!m_bShowItemsInHiddenFolders)
  {
    if (xiiStringUtils::FindSubString_NoCase(pInfo->m_pAssetInfo->m_sDataDirParentRelativePath + m_sPathFilter.GetElementCount() + 1, "_data/") !=
        nullptr)
      return true;
  }

  if (!m_sTextFilter.IsEmpty())
  {
    if (m_bUsesSearchActive)
    {
      if (!m_Uses.Contains(pInfo->m_Data.m_Guid))
        return true;
    }
    else
    {
      // if the string is not found in the path, ignore this asset
      if (pInfo->m_pAssetInfo->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
      {
        if (pInfo->GetName().FindSubString_NoCase(m_sTextFilter) == nullptr)
        {
          xiiConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
          if (m_sTemp.FindSubString_NoCase(m_sTextFilter) == nullptr)
            return true;

          // we could actually (partially) match the GUID
        }
      }
    }
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    m_sTemp.Set(";", pInfo->m_Data.m_sSubAssetsDocumentTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return true;
  }
  return false;
}

bool xiiQtAssetBrowserFilter::Less(const xiiSubAsset* pInfoA, const xiiSubAsset* pInfoB) const
{
  if (m_bSortByRecentUse && pInfoA->m_LastAccess.GetSeconds() != pInfoB->m_LastAccess.GetSeconds())
  {
    return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
  }

  xiiStringView sSortA = pInfoA->GetName();
  xiiStringView sSortB = pInfoB->GetName();

  xiiInt32 iValue = xiiStringUtils::Compare_NoCase(sSortA.GetStartPointer(), sSortB.GetStartPointer(), sSortA.GetEndPointer(), sSortB.GetEndPointer());
  if (iValue == 0)
  {
    return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
  }
  return iValue < 0;
}
