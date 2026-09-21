/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QItemDelegate>
#include <QTreeWidget>
#include <QValidator>

class xiiQtAssetBrowserFilter;

/// Basic file name validator. Makes sure that under a given parent folder, the new file name is valid and not already in use by a different file.
class xiiFileNameValidator : public QValidator
{
public:
  /// Constructor. Validator requires the current location and name of the file.
  /// \param sParentFolder Absolute path to the location of the file.
  /// \param sCurrentName Current filename. If set, this name is marked as valid, even though it is already in use.
  xiiFileNameValidator(QObject* pParent, xiiStringView sParentFolder, xiiStringView sCurrentName);
  virtual QValidator::State validate(QString& ref_sInput, int& ref_iPos) const override;

private:
  xiiString m_sParentFolder;
  xiiString m_sCurrentName;
};

/// Custom delegate for the eqQtAssetBrowserFolderView to enable renaming folders. Does not do any model modifications. Instead, it fires editingFinished when the delegate editor closes.
class xiiFolderNameDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  xiiFolderNameDelegate(QObject* pParent = nullptr);

  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void     setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName) const;
};

/// Folder tree of the asset browser to allow filtering by folder.
///
/// This class keeps up to date with the folder structure in xiiFileSystemModel. Events from xiiFileSystemModel are cached ans flushed via OnFlushFileSystemEvents.
/// Folder movement, creation and deletion is supported and handled by this class. The context menu is implemented in xiiQtAssetBrowserWidget as it requires a global context of the asset browser instance.
class eqQtAssetBrowserFolderView : public QTreeWidget
{
  Q_OBJECT
public:
  eqQtAssetBrowserFolderView(QWidget* pParent);
  ~eqQtAssetBrowserFolderView();

  /// Required to be set right after the ctor. This class will call xiiQtAssetBrowserFilter::SetPathFilter whenever the current selected item changes.
  void SetFilter(xiiQtAssetBrowserFilter* pFilter);
  /// In dialog mode, any modifications (folder movement, creation and deletion) are disabled.
  void SetDialogMode(bool bDialogMode);

  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

public Q_SLOTS:
  /// Creates a new folder under the current selected item and enters edit mode to allow the user to rename it.
  void NewFolder();
  /// Opens the current selected item in the windows explorer or OS equivalent.
  void TreeOpenExplorer();
  /// Deletes the currently selected folder after confirmation.
  void DeleteFolder();

private Q_SLOTS:
  void OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName);
  void OnFlushFileSystemEvents();
  void OnItemSelectionChanged();
  void OnPathFilterChanged();

protected:
  virtual void            dragMoveEvent(QDragMoveEvent* e) override;
  virtual void            mouseMoveEvent(QMouseEvent* e) override;
  virtual void            dropEvent(QDropEvent* event) override;
  virtual Qt::DropActions supportedDropActions() const override;
  xiiStatus               canDrop(QDropEvent* e, xiiDynamicArray<xiiString>& out_files, xiiString& out_sTargetFolder);
  virtual QStringList     mimeTypes() const override;
  virtual QMimeData*      mimeData(const QList<QTreeWidgetItem*>& items) const override;
  virtual void            keyPressEvent(QKeyEvent* e) override;

private:
  bool             SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void             UpdateDirectoryTree();
  void             ClearDirectoryTree();
  void             BuildDirectoryTree(const xiiDataDirPath& path, xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem, bool bIsHidden);
  void             RemoveDirectoryTreeItem(xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem);
  QTreeWidgetItem* FindDirectoryTreeItem(xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem);
  void             FileSystemModelFolderEventHandler(const xiiFolderChangedEvent& e);
  void             ProjectEventHandler(const xiiToolsProjectEvent& e);

private:
  bool      m_bDialogMode                    = false;
  xiiUInt32 m_uiKnownAssetFolderCount        = 0;
  bool      m_bTreeSelectionChangeInProgress = false;

  xiiQtAssetBrowserFilter* m_pFilter = nullptr;

  xiiMutex                                 m_FolderStructureMutex;
  xiiHybridArray<xiiFolderChangedEvent, 2> m_QueuedFolderEvents;
};
