/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QtWidgets/qtwidgetsglobal.h>

#if QT_CONFIG(systemtrayicon)

#  include <Foundation/Utilities/Progress.h>
#  include <QMap>
#  include <QMenu>
#  include <QObject>
#  include <QPointer>
#  include <QString>
#  include <QSystemTrayIcon>
#  include <QWidget>

class xiiAssetProcessor;
struct xiiAssetProcessorEvent;
class xiiQtEditorApp;

/// Optional Qt tray presentation. Construct/destroy on the GUI thread before the borrowed
/// progress source and asset processor are destroyed. Qt alone owns the native tray entry.
class XII_EDITORFRAMEWORK_DLL xiiQtEditorSystemTray final : public QObject
{
public:
  xiiQtEditorSystemTray(xiiQtEditorApp& editor, QWidget& window, xiiProgress& progress);
  ~xiiQtEditorSystemTray();

private:
  friend struct xiiEditorSystemTrayTest;

  void ProgressEventHandler(const xiiProgressEvent& e);
  void ProcessorEventHandler(const xiiAssetProcessorEvent& e);
  void UpdateToolTip();
  void RefreshMenu();
  void RefreshIcon();
  void ShowEditor();
  bool CanChangeProject() const;
  bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  xiiQtEditorApp&          m_Editor;
  QPointer<QWidget>        m_pWindow;
  xiiProgress&             m_Progress;
  xiiAssetProcessor*       m_pProcessor = nullptr;
  QMenu                    m_Menu;
  QSystemTrayIcon          m_Icon;
  QMenu*                   m_pRecentProjects = nullptr;
  QAction*                 m_pOpenProject    = nullptr;
  QAction*                 m_pExit           = nullptr;
  QMap<QString, xiiUInt32> m_Tools;
  QString                  m_sForeground;
  QString                  m_sBackground;
  bool                     m_bProgressActive = false;
};

#endif
