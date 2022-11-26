#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserFilter : public xiiQtAssetFilter
{
  Q_OBJECT
public:
  explicit xiiQtAssetBrowserFilter(QObject* pParent);

  /// Resets all filters to their default state.
  void Reset();

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetShowItemsInHiddenFolders(bool bShow);
  bool GetShowItemsInHiddenFolders() const { return m_bShowItemsInHiddenFolders; }

  void SetSortByRecentUse(bool bSort);
  bool GetSortByRecentUse() const { return m_bSortByRecentUse; }

  void        SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_sTextFilter; }

  void        SetPathFilter(const char* szPath);
  const char* GetPathFilter() const { return m_sPathFilter; }

  void        SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();

public:
  virtual bool IsAssetFiltered(const xiiSubAsset* pInfo) const override;
  virtual bool Less(const xiiSubAsset* pInfoA, const xiiSubAsset* pInfoB) const override;

private:
  xiiString                m_sTextFilter, m_sTypeFilter, m_sPathFilter;
  bool                     m_bShowItemsInSubFolders    = true;
  bool                     m_bShowItemsInHiddenFolders = false;
  bool                     m_bSortByRecentUse          = false;
  mutable xiiStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  // Cache for uses search
  bool            m_bUsesSearchActive = false;
  bool            m_bTransitive       = false;
  xiiSet<xiiUuid> m_Uses;
};
