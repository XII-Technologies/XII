/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <EditorPluginFileserve/ui_FileserveWidget.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Time.h>
#include <QWidget>

struct xiiFileserverEvent;
class xiiQtFileserveActivityModel;
class xiiQtFileserveAllFilesModel;
enum class xiiFileserveActivityType;

/// A GUI for the xiiFileServer
///
/// By default the file server does run at startup. Using the command line option "-fs_nostart" prevents that.
class XII_EDITORPLUGINFILESERVE_DLL xiiQtFileserveWidget : public QWidget, public Ui_xiiQtFileserveWidget
{
  Q_OBJECT

public:
  xiiQtFileserveWidget(QWidget* pParent = nullptr);

  void FindOwnIP(xiiStringBuilder& out_sDisplay, xiiHybridArray<xiiStringBuilder, 4>* out_pAllIPs = nullptr);

  ~xiiQtFileserveWidget();

Q_SIGNALS:
  void ServerStarted(const QString& sIp, xiiUInt16 uiPort);
  void ServerStopped();

public Q_SLOTS:
  void on_StartServerButton_clicked();
  void on_ClearActivityButton_clicked();
  void on_ClearAllFilesButton_clicked();
  void on_ReloadResourcesButton_clicked();
  void on_ConnectClient_clicked();

private:
  void FileserverEventHandler(const xiiFileserverEvent& e);
  void LogActivity(const xiiFormatString& text, xiiFileserveActivityType type);
  void UpdateSpecialDirectoryUI();

  xiiQtFileserveActivityModel* m_pActivityModel;
  xiiQtFileserveAllFilesModel* m_pAllFilesModel;
  xiiTime                      m_LastProgressUpdate;

  struct DataDirInfo
  {
    xiiString m_sName;
    xiiString m_sPath;
    xiiString m_sRedirectedPath;
  };

  struct ClientData
  {
    bool                           m_bConnected = false;
    xiiHybridArray<DataDirInfo, 8> m_DataDirs;
  };

  struct SpecialDir
  {
    xiiString m_sName;
    xiiString m_sPath;
  };

  xiiHybridArray<SpecialDir, 4> m_SpecialDirectories;

  xiiHashTable<xiiUInt32, ClientData> m_Clients;
  void                                UpdateClientList();
  void                                ConfigureSpecialDirectories();
};
