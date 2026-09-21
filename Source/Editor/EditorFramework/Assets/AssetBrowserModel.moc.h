/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <QAbstractItemModel>
#include <QFileIconProvider>

struct xiiAssetInfo;
struct xiiAssetCuratorEvent;
struct xiiSubAsset;
class xiiQtAssetFilter;

/// Interface class of the asset filter used to decide which items are shown in the asset browser.
class XII_EDITORFRAMEWORK_DLL xiiQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit xiiQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(xiiStringView sDataDirParentRelativePath, bool bIsFolder, const xiiSubAsset* pInfo) const = 0;
  virtual bool GetSortByRecentUse() const { return false; }

Q_SIGNALS:
  void FilterChanged();
};

/// Each item in the asset browser can be multiple things at the same time as described by these flags.
/// Retrieved via user role xiiQtAssetBrowserModel::UserRoles::ItemFlags.
struct XII_EDITORFRAMEWORK_DLL xiiAssetBrowserItemFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Folder        = XII_BIT(0), // Any folder inside a data directory
    DataDirectory = XII_BIT(1), // mutually exclusive with Folder
    File          = XII_BIT(2), // any file, could also be an Asset
    Asset         = XII_BIT(3), // main asset: mutually exclusive with SubAsset
    SubAsset      = XII_BIT(4), // sub-asset (imaginary, not a File or Asset)
    Default       = 0
  };

  struct Bits
  {
    StorageType Folder : 1;
    StorageType DataDirectory : 1;
    StorageType File : 1;
    StorageType Asset : 1;
    StorageType SubAsset : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiAssetBrowserItemFlags);

/// Model of the item view in the asset browser.
class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserModel : public QAbstractItemModel, public QEnableSharedFromThis<xiiQtAssetBrowserModel>
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0, // xiiUuid
    AssetGuid,                       // xiiUuid
    AbsolutePath,                    // QString
    RelativePath,                    // QString
    AssetIcon,                       // QIcon
    TransformState,                  // QString
    Importable,                      // bool
    ItemFlags,                       // xiiAssetBrowserItemFlags as int
  };

  xiiQtAssetBrowserModel(QObject* pParent, xiiQtAssetFilter* pFilter);
  ~xiiQtAssetBrowserModel();

  void Initialize();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  xiiInt32 FindAssetIndex(const xiiUuid& assetGuid) const;
  xiiInt32 FindIndex(xiiStringView sAbsPath) const;

public Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2);
  void ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID);
  void OnFileSystemUpdate();

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset) const;

public: // QAbstractItemModel interface
  virtual QVariant        data(const QModelIndex& index, int iRole) const override;
  virtual bool            setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags   flags(const QModelIndex& index) const override;
  virtual QVariant        headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex     index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex     parent(const QModelIndex& index) const override;
  virtual int             rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int             columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList     mimeTypes() const override;
  virtual QMimeData*      mimeData(const QModelIndexList& indexes) const override;
  virtual Qt::DropActions supportedDropActions() const override;

private:
  friend struct FileComparer;

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };

  struct VisibleEntry
  {
    xiiDataDirPath                        m_sAbsFilePath;
    xiiUuid                               m_Guid;
    xiiBitflags<xiiAssetBrowserItemFlags> m_Flags;
    mutable xiiUInt32                     m_uiThumbnailID;
  };

  struct FsEvent
  {
    xiiFileChangedEvent   m_FileEvent;
    xiiFolderChangedEvent m_FolderEvent;
  };

private:
  void AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  void HandleEntry(const VisibleEntry& entry, AssetOp op);
  void FileSystemFileEventHandler(const xiiFileChangedEvent& e);
  void FileSystemFolderEventHandler(const xiiFolderChangedEvent& e);
  void HandleFile(const xiiFileChangedEvent& e);
  void HandleFolder(const xiiFolderChangedEvent& e);

private:
  xiiQtAssetFilter* m_pFilter   = nullptr;
  bool              m_bIconMode = true;
  xiiSet<xiiString> m_ImportExtensions;

  xiiEventSubscriptionID m_FileChangedSubscription   = 0;
  xiiEventSubscriptionID m_FolderChangedSubscription = 0;

  xiiMutex                 m_Mutex;
  xiiDynamicArray<FsEvent> m_QueuedFileSystemEvents;

  xiiDynamicArray<VisibleEntry> m_EntriesToDisplay;
  xiiSet<xiiUuid>               m_DisplayedEntries;

  QFileIconProvider m_IconProvider;
};
