/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Fileserve/Fileserve.h>

#ifdef XII_USE_QT

#  include <QMainWindow>

class xiiApplication;
class xiiQtFileserveWidget;

class xiiQtFileserveMainWnd : public QMainWindow
{
  Q_OBJECT
public:
  xiiQtFileserveMainWnd(xiiApplication* pApp, QWidget* pParent = nullptr);

private Q_SLOTS:
  void UpdateNetworkSlot();
  void OnServerStarted(const QString& ip, xiiUInt16 uiPort);
  void OnServerStopped();

private:
  xiiApplication*       m_pApp;
  xiiQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(xiiApplication* pApp);

#endif
