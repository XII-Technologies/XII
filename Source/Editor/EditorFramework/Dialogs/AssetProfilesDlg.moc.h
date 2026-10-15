/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_AssetProfilesDlg.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>

#include <QDialog>

class xiiAssetProfilesDocument;
class xiiPlatformProfile;
class xiiQtDocumentTreeView;
class xiiDocument;
class xiiQtAssetConfigAdapter;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetProfilesDlg : public QDialog, public Ui_xiiQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  xiiQtAssetProfilesDlg(QWidget* pParent);
  ~xiiQtAssetProfilesDlg();

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  friend class xiiQtAssetConfigAdapter;
  bool      DetermineNewProfileName(QWidget* parent, xiiString& result, xiiUuid exclude = {});
  bool      CheckProfileNameUniqueness(const char* szName, xiiUuid exclude);
  void      AllAssetProfilesToObject();
  xiiStatus ApplyAllChanges();
  xiiUuid   NativeToObject(xiiPlatformProfile* pProfile);
  void      ObjectToNative(xiiUuid objectGuid, xiiPlatformProfile* pProfile);
  void      SelectionEventHandler(const xiiSelectionManagerEvent& e);

  xiiAssetProfilesDocument* m_pDocument;
  xiiUuid                   m_ActiveProfile;
  xiiUuid                   m_OriginalActiveProfile;
};
