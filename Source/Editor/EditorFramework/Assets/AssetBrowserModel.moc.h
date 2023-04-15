#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>

struct xiiAssetInfo;
struct xiiAssetCuratorEvent;
struct xiiSubAsset;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit xiiQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(const xiiSubAsset* pInfo) const                  = 0;
  virtual bool Less(const xiiSubAsset* pInfoA, const xiiSubAsset* pInfoB) const = 0;

Q_SIGNALS:
  void FilterChanged();
};

class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0,
    AssetGuid,
    AbsolutePath,
    RelativePath,
    AssetIconPath,
    TransformState,
  };

  xiiQtAssetBrowserModel(QObject* pParent, xiiQtAssetFilter* pFilter);
  ~xiiQtAssetBrowserModel();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

private Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID);

public: // QAbstractItemModel interface
  virtual QVariant      data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant      headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex   index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex   parent(const QModelIndex& index) const override;
  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList   mimeTypes() const override;
  virtual QMimeData*    mimeData(const QModelIndexList& indexes) const override;

private:
  friend struct AssetComparer;
  struct AssetEntry
  {
    xiiUuid           m_Guid;
    mutable xiiUInt32 m_uiThumbnailID;
  };

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };
  void     AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  xiiInt32 FindAssetIndex(const xiiUuid& assetGuid) const;
  void     HandleAsset(const xiiSubAsset* pInfo, AssetOp op);
  void     Init(AssetEntry& ae, const xiiSubAsset* pInfo);

  xiiQtAssetFilter*           m_pFilter;
  xiiDynamicArray<AssetEntry> m_AssetsToDisplay;
  xiiSet<xiiUuid>             m_DisplayedEntries;

  bool m_bIconMode;
};
