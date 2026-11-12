/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Strings/TranslationLookup.h>

#include <QRegularExpression>

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
  SetScope(Scope::Content);
  SetStatusFilter(Status::Any);
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


namespace
{
  int ParseTransformState(const QString& sValue)
  {
    // Query keywords are stable across editor languages.
    static constexpr const char* states[] = {
      "Unknown", "UpToDate", "NeedsImport", "NeedsTransform", "NeedsThumbnail", "TransformError",
      "MissingTransformDependency", "MissingThumbnailDependency", "MissingPackageDependency", "CircularDependency", "Processing"};
    static_assert(std::size(states) == xiiAssetInfo::COUNT + 1);
    for (int i = 0; i < std::size(states); ++i)
    {
      if (sValue.compare(QLatin1String(states[i]), Qt::CaseInsensitive) == 0)
        return i;
    }

    if (sValue.compare(QLatin1String("Ready"), Qt::CaseInsensitive) == 0)
      return xiiAssetInfo::UpToDate;
    if (sValue.compare(QLatin1String("Failed"), Qt::CaseInsensitive) == 0)
      return xiiAssetInfo::TransformError;
    return -1;
  }
} // namespace

void xiiQtAssetBrowserFilter::SetScope(Scope scope)
{
  SetLocation("", scope);
}

void xiiQtAssetBrowserFilter::SetStatusFilter(Status status)
{
  if (m_Status == status)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_Status = status;
  Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetFavorites(const xiiSet<xiiUuid>& favorites)
{
  if (m_Favorites == favorites)
    return;

  m_Favorites = favorites;
  if (m_Scope == Scope::Favorites)
    Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetFavorite(xiiUuid guid, bool bFavorite)
{
  if (!guid.IsValid() || m_Favorites.Contains(guid) == bFavorite)
    return;

  if (bFavorite)
    m_Favorites.Insert(guid);
  else
    m_Favorites.Remove(guid);

  if (m_Scope == Scope::Favorites)
    Q_EMIT FilterChanged();
}

void xiiQtAssetBrowserFilter::SetTextFilter(const char* szText)
{
  if (m_sQuery == szText)
    return;

  m_sQuery = szText;
  m_sTemporaryPinnedItem.Clear();
  m_QueryTerms.clear();
  m_References.Clear();
  m_sQueryError.clear();

  static const QRegularExpression fields(
    QStringLiteral(R"query((?:^|\s)(type|name|path|status|tag|ref-all|ref|meta\.[a-zA-Z_][a-zA-Z_0-9]*):("[^"]*"|\{[^}]*\}|[^\s]*))query"),
    QRegularExpression::CaseInsensitiveOption);
  const QString sQuery = xiiMakeQString(m_sQuery);
  QString       sPlain;
  qsizetype     iPlainStart = 0;

  for (auto matches = fields.globalMatch(sQuery); matches.hasNext();)
  {
    const auto match = matches.next();
    sPlain += sQuery.sliced(iPlainStart, match.capturedStart() - iPlainStart);
    sPlain += QLatin1Char(' ');
    iPlainStart = match.capturedEnd();

    const QString sField = match.captured(1).toLower();
    QString       sValue = match.captured(2);
    if (sValue.startsWith(QLatin1Char('"')) && sValue.endsWith(QLatin1Char('"')) && sValue.size() > 1)
      sValue = sValue.sliced(1, sValue.size() - 2).trimmed();

    if (sValue.isEmpty() || sValue.contains(QLatin1Char('"')))
    {
      m_sQueryError = tr("Enter a value for %1. Enclose values containing spaces in double quotes.").arg(sField);
      break;
    }

    if (sField == QLatin1String("ref") || sField == QLatin1String("ref-all"))
    {
      if (sValue.startsWith(QLatin1Char('{')) && sValue.endsWith(QLatin1Char('}')))
        sValue = sValue.sliced(1, sValue.size() - 2).trimmed();

      const xiiString sGuid = qtToXIIString(QStringLiteral("{ %1 }").arg(sValue));
      if (!xiiConversionUtils::IsStringUuid(sGuid))
      {
        m_sQueryError = tr("%1 requires an asset GUID.").arg(sField);
        break;
      }
      auto& reference         = m_References.ExpandAndGetRef();
      reference.m_Guid        = xiiConversionUtils::ConvertStringToUuid(sGuid);
      reference.m_bTransitive = sField == QLatin1String("ref-all");
      continue;
    }

    QueryTerm term;
    term.m_sValue = qtToXIIString(sValue);
    if (sField == QLatin1String("type"))
      term.m_Field = QueryTerm::Type;
    else if (sField == QLatin1String("name"))
      term.m_Field = QueryTerm::Name;
    else if (sField == QLatin1String("tag"))
      term.m_Field = QueryTerm::Tag;
    else if (sField == QLatin1String("path"))
    {
      term.m_Field = QueryTerm::Path;
      sValue.replace(QLatin1Char('\\'), QLatin1Char('/'));
      term.m_sValue = qtToXIIString(sValue);
      if (sValue.contains(QLatin1Char('*')) || sValue.contains(QLatin1Char('?')))
        term.m_Glob = QRegularExpression(QRegularExpression::wildcardToRegularExpression(sValue, QRegularExpression::NonPathWildcardConversion), QRegularExpression::CaseInsensitiveOption);
    }
    else if (sField == QLatin1String("status"))
    {
      term.m_Field   = QueryTerm::Status;
      term.m_iStatus = ParseTransformState(sValue);
      if (term.m_iStatus < 0)
      {
        m_sQueryError = tr("Unknown processing status “%1”. Try Ready, Failed, NeedsTransform or NeedsThumbnail.").arg(sValue);
        break;
      }
    }
    else
    {
      term.m_Field     = QueryTerm::Metadata;
      term.m_sProperty = qtToXIIString(sField.sliced(5));
    }
    m_QueryTerms.push_back(std::move(term));
  }

  sPlain += sQuery.sliced(iPlainStart);
  xiiStringBuilder sCleanText = qtToXIIString(sPlain.trimmed());
  sCleanText.MakeCleanPath();
  m_SearchFilter.SetSearchText(sCleanText);

  if (m_sQueryError.isEmpty())
    UpdateReferenceMatches(false);

  Q_EMIT FilterChanged();
  Q_EMIT TextFilterChanged();
}

void xiiQtAssetBrowserFilter::RefreshReferences()
{
  UpdateReferenceMatches(true);
}

void xiiQtAssetBrowserFilter::UpdateReferenceMatches(bool bNotify)
{
  if (!m_sQueryError.isEmpty())
    return;

  bool bChanged = false;
  for (auto& reference : m_References)
  {
    xiiSet<xiiUuid> uses;
    xiiAssetCurator::GetSingleton()->FindAllUses(reference.m_Guid, uses, reference.m_bTransitive);
    if (!(uses == reference.m_Uses))
    {
      reference.m_Uses.Swap(uses);
      bChanged = true;
    }
  }

  if (bNotify && bChanged)
    Q_EMIT FilterChanged();
}

bool xiiQtAssetBrowserFilter::MatchesQuery(xiiStringView sPath, const xiiSubAsset* pInfo) const
{
  for (const auto& reference : m_References)
  {
    if (!pInfo || !reference.m_Uses.Contains(pInfo->m_Data.m_Guid))
      return false;
  }

  for (const auto& term : m_QueryTerms)
  {
    switch (term.m_Field)
    {
      case QueryTerm::Type:
        if (!pInfo || (!pInfo->m_Data.m_sSubAssetsDocumentTypeName.GetView().IsEqual_NoCase(term.m_sValue) && !xiiTranslate(pInfo->m_Data.m_sSubAssetsDocumentTypeName.GetView()).IsEqual_NoCase(term.m_sValue)))
          return false;
        break;
      case QueryTerm::Name:
        if (!(pInfo ? pInfo->GetName() : sPath.GetFileNameAndExtension()).FindSubString_NoCase(term.m_sValue))
          return false;
        break;
      case QueryTerm::Path:
        if (term.m_Glob.pattern().isEmpty() ? !sPath.FindSubString_NoCase(term.m_sValue) : !term.m_Glob.match(xiiMakeQString(sPath)).hasMatch())
          return false;
        break;
      case QueryTerm::Status:
      {
        if (!pInfo)
          return false;
        if (term.m_iStatus == xiiAssetInfo::COUNT)
        {
          if (!xiiAssetCurator::GetSingleton()->IsAssetBeingTransformed(pInfo->m_pAssetInfo->m_Info->m_DocumentID))
            return false;
        }
        else if (pInfo->m_pAssetInfo->m_TransformState != term.m_iStatus)
          return false;
        break;
      }
      case QueryTerm::Tag:
        m_sTemp.Set(";", term.m_sValue, ";");
        if (!pInfo || !pInfo->m_pAssetInfo->m_Info->GetAssetsDocumentTags().FindSubString_NoCase(m_sTemp))
          return false;
        break;
      case QueryTerm::Metadata:
      {
        if (!pInfo)
          return false;
        bool bMatches = false;
        pInfo->m_pAssetInfo->m_Info->VisitMetadataProperties(term.m_sProperty, [&term, &bMatches](xiiStringView, xiiStringView sValue) {
          bMatches = sValue.FindSubString_NoCase(term.m_sValue) != nullptr;
          return !bMatches;
        });
        if (!bMatches)
          return false;
        break;
      }
    }
  }

  if (!m_SearchFilter.IsEmpty() && !m_SearchFilter.PassesFilters(sPath))
  {
    if (!pInfo)
      return false;
    if (!m_SearchFilter.PassesFilters(pInfo->GetName()))
    {
      xiiConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
      return m_SearchFilter.PassesFilters(m_sTemp);
    }
  }
  return true;
}

void xiiQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  SetLocation(szPath, Scope::Content);
}

void xiiQtAssetBrowserFilter::SetLocation(const char* szPath, Scope scope)
{
  xiiStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();
  // The assumption is that only full directory names are set as path filters. Thus, we can ensure they end with a / to make it easier to filter items inside the path.
  if (!sCleanText.IsEmpty() && !sCleanText.EndsWith_NoCase("/"))
  {
    sCleanText.Append("/");
  }

  const bool bPathChanged  = m_sPathFilter != sCleanText;
  const bool bScopeChanged = m_Scope != scope;
  if (!bPathChanged && !bScopeChanged)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sPathFilter = sCleanText;
  m_Scope       = scope;

  Q_EMIT FilterChanged();
  if (bPathChanged)
    Q_EMIT PathFilterChanged();
  if (bScopeChanged)
    Q_EMIT ScopeChanged();
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

  if (m_sQuery.IsEmpty() && !m_bShowItemsInSubFolders && m_Scope == Scope::Content && m_Status == Status::Any)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (xiiStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
    {
      return true;
    }
  }
  if (!m_bShowItemsInHiddenFolders)
  {
    // treat folders starting with a dot as hidden folders
    if (sDataDirParentRelativePath.FindSubString("/."))
      return true;

    if (m_References.IsEmpty())
    {
      if (xiiStringUtils::FindSubString_NoCase(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount() + 1, "_data/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
        return true;
    }
  }

  if (m_Scope != Scope::Content || m_Status != Status::Any)
  {
    if (!pInfo)
      return true;

    const auto state = pInfo->m_pAssetInfo->m_TransformState;
    if (m_Scope == Scope::Recent && !pInfo->m_LastAccess.IsPositive())
      return true;
    if (m_Scope == Scope::Favorites && !m_Favorites.Contains(pInfo->m_Data.m_Guid))
      return true;
    if ((m_Scope == Scope::Attention || m_Status == Status::Attention) && state == xiiAssetInfo::UpToDate)
      return true;
    if (m_Status == Status::Ready && state != xiiAssetInfo::UpToDate)
      return true;
    if (m_Status == Status::Pending && (state == xiiAssetInfo::UpToDate || state >= xiiAssetInfo::TransformError))
      return true;
    if (m_Status == Status::Errors && state < xiiAssetInfo::TransformError)
      return true;
  }

  if (!m_sQueryError.isEmpty() || !MatchesQuery(sDataDirParentRelativePath, pInfo))
    return true;

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
