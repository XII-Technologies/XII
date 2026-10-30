/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_AssetProfilesPage.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>

#include <QWidget>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiAssetProfilesDocument;
class xiiPlatformProfile;
class xiiQtDocumentTreeView;
class xiiDocument;
class xiiQtAssetConfigAdapter;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetProfilesPage : public QWidget, public Ui_xiiQtAssetProfilesPage
{
public:
  Q_OBJECT

public:
  explicit xiiQtAssetProfilesPage(QWidget* pParent, bool bManageLifecycle = true);
  ~xiiQtAssetProfilesPage();

  xiiResult ApplyChanges();
  xiiStatus ApplyAllChanges();
  bool      HasChanges() const;

private Q_SLOTS:
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  friend class xiiQtAssetConfigAdapter;
  bool DetermineNewProfileName(QWidget* parent, xiiString& result, xiiUuid exclude = {});
  bool CheckProfileNameUniqueness(const char* szName, xiiUuid exclude);
  void AllAssetProfilesToObject();

  xiiUuid NativeToObject(xiiPlatformProfile* pProfile);
  void    ObjectToNative(xiiUuid objectGuid, xiiPlatformProfile* pProfile);
  void    SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void    ReleaseObjects();

  xiiAssetProfilesDocument*                                     m_pDocument;
  xiiUuid                                                       m_ActiveProfile;
  xiiUuid                                                       m_OriginalActiveProfile;
  xiiCopyOnBroadcastEvent<const xiiPluginEvent&>::Unsubscriber  m_PluginSubscription;
  xiiEvent<const xiiToolsProjectEvent&, xiiMutex>::Unsubscriber m_ProjectSubscription;
};
