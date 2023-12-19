#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetProfilesDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <QDialog>

class xiiAssetProfilesDocument;
class xiiPlatformProfile;
class xiiQtDocumentTreeView;
class xiiDocument;
struct xiiDocumentObjectPropertyEvent;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetProfilesDlg : public QDialog, public Ui_xiiQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  xiiQtAssetProfilesDlg(QWidget* pParent);
  ~xiiQtAssetProfilesDlg();

  xiiUInt32 m_uiActiveConfig = 0;

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  struct Binding
  {
    enum class State
    {
      None,
      Added,
      Deleted
    };

    State               m_State    = State::None;
    xiiPlatformProfile* m_pProfile = nullptr;
  };

  bool    DetermineNewProfileName(QWidget* parent, xiiString& result);
  bool    CheckProfileNameUniqueness(const char* szName);
  void    AllAssetProfilesToObject();
  void    PropertyChangedEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void    ApplyAllChanges();
  xiiUuid NativeToObject(xiiPlatformProfile* pProfile);
  void    ObjectToNative(xiiUuid objectGuid, xiiPlatformProfile* pProfile);
  void    SelectionEventHandler(const xiiSelectionManagerEvent& e);

  xiiAssetProfilesDocument* m_pDocument;
  xiiMap<xiiUuid, Binding>  m_ProfileBindings;
};
