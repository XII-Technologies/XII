/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

#include <QList>
#include <QRegularExpression>

class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserFilter : public xiiQtAssetFilter
{
  Q_OBJECT
public:
  explicit xiiQtAssetBrowserFilter(QObject* pParent);

  /// Resets all filters to their default state.
  void Reset();
  void UpdateImportExtensions(const xiiSet<xiiString>& extensions);

  enum class Scope
  {
    Content,
    Recent,
    Favorites,
    Attention,
  };

  enum class Status
  {
    Any,
    Ready,
    Attention,
    Pending,
    Errors,
  };

  void                   SetScope(Scope scope);
  void                   SetLocation(const char* szPath, Scope scope);
  Scope                  GetScope() const { return m_Scope; }
  void                   SetStatusFilter(Status status);
  Status                 GetStatusFilter() const { return m_Status; }
  void                   SetFavorites(const xiiSet<xiiUuid>& favorites);
  void                   SetFavorite(xiiUuid guid, bool bFavorite);
  const xiiSet<xiiUuid>& GetFavorites() const { return m_Favorites; }
  void                   RefreshReferences();
  bool                   HasReferenceQuery() const { return !m_References.IsEmpty(); }

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetShowFiles(bool bShow);
  bool GetShowFiles() const { return m_bShowFiles; }

  void SetShowNonImportableFiles(bool bShow);
  bool GetShowNonImportableFiles() const { return m_bShowNonImportableFiles; }

  void SetShowItemsInHiddenFolders(bool bShow);
  bool GetShowItemsInHiddenFolders() const { return m_bShowItemsInHiddenFolders; }

  void         SetSortByRecentUse(bool bSort);
  virtual bool GetSortByRecentUse() const override { return m_bSortByRecentUse || m_Scope == Scope::Recent; }

  void           SetTextFilter(const char* szText);
  const char*    GetTextFilter() const { return m_sQuery; }
  const QString& GetQueryError() const { return m_sQueryError; }

  void          SetPathFilter(const char* szPath);
  xiiStringView GetPathFilter() const;

  void        SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

  void SetFileExtensionFilters(xiiStringView sExtensions);

  void SetRequiredTag(xiiStringView sRequiredTag);

  /// If set, the given item will be visible no matter what until any other filter is changed.
  /// This is used to ensure that newly created assets are always visible, even if they are excluded from the current filter.
  void          SetTemporaryPinnedItem(xiiStringView sDataDirParentRelativePath);
  xiiStringView GetTemporaryPinnedItem() const { return m_sTemporaryPinnedItem; }

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();
  void ScopeChanged();

public:
  virtual bool IsAssetFiltered(xiiStringView sDataDirParentRelativePath, bool bIsFolder, const xiiSubAsset* pInfo) const override;

private:
  struct QueryTerm
  {
    enum Field
    {
      Type,
      Name,
      Path,
      Status,
      Tag,
      Metadata,
    };

    Field              m_Field;
    xiiString          m_sValue;
    xiiString          m_sProperty;
    QRegularExpression m_Glob;
    int                m_iStatus = -1;
  };

  struct ReferenceTerm
  {
    xiiUuid         m_Guid;
    bool            m_bTransitive = false;
    xiiSet<xiiUuid> m_Uses;
  };

  bool MatchesQuery(xiiStringView sPath, const xiiSubAsset* pInfo) const;
  void UpdateReferenceMatches(bool bNotify);

  xiiString                m_sTypeFilter;
  xiiString                m_sRequiredTag = "*"; // show all is the default for the asset browser
  xiiString                m_sPathFilter;
  xiiString                m_sTemporaryPinnedItem;
  xiiSearchPatternFilter   m_SearchFilter;
  xiiString                m_sQuery;
  QString                  m_sQueryError;
  QList<QueryTerm>         m_QueryTerms;
  Scope                    m_Scope                     = Scope::Content;
  Status                   m_Status                    = Status::Any;
  bool                     m_bShowItemsInSubFolders    = true;
  bool                     m_bShowFiles                = true;
  bool                     m_bShowNonImportableFiles   = true;
  bool                     m_bShowItemsInHiddenFolders = false;
  bool                     m_bSortByRecentUse          = false;
  mutable xiiStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  xiiDynamicArray<ReferenceTerm> m_References;
  xiiSet<xiiUuid>                m_Favorites;
  xiiSet<xiiString>              m_ImportExtensions;
  xiiSet<xiiString>              m_FileExtensions;
};
