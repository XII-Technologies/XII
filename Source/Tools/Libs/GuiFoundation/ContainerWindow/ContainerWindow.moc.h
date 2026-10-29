/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiDocumentManager;
class xiiDocument;
class xiiQtApplicationPanel;
struct xiiDocumentTypeDescriptor;
class QLabel;
class QMenuBar;

namespace ads
{
  class CDockManager;
  class CFloatingDockContainer;
  class CDockWidget;
} // namespace ads

/// Container window that hosts documents and applications panels.
class XII_GUIFOUNDATION_DLL xiiQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// Constructor.
  xiiQtContainerWindow();
  ~xiiQtContainerWindow();

  static xiiQtContainerWindow* GetContainerWindow() { return s_pContainerWindow; }

  void AddDocumentWindow(xiiQtDocumentWindow* pDocWindow);
  void DocumentWindowRenamed(xiiQtDocumentWindow* pDocWindow);
  void AddApplicationPanel(xiiQtApplicationPanel* pPanel);

  ads::CDockManager* GetDockManager() { return m_pDockManager; }

  static xiiResult EnsureVisibleAnyContainer(xiiDocument* pDocument);

  void GetDocumentWindows(xiiHybridArray<xiiQtDocumentWindow*, 16>& ref_windows);

  /// Resolves the document context for application commands such as the settings dialog.
  xiiQtDocumentWindow* GetFocusedDocumentWindow() const;

  void SaveWindowLayout();
  void SaveDocumentLayouts();
  void RestoreWindowLayout();
  void RestoreDocumentLayouts();

  void ScheduleRestoreWindowLayout();

  /// Named project-local layouts retain the existing ADS format and document layout groups.
  QStringList GetWorkspaceNames() const;
  xiiResult   SaveWorkspace(const QString& sName);
  xiiResult   RestoreWorkspace(const QString& sName);
  xiiResult   RemoveWorkspace(const QString& sName);

  /// Startup may construct docks and restore layouts while presentation is disabled.
  /// Enabling presentation does not show the window; the application owns that transition.
  void SetWindowPresentationEnabled(bool bEnabled) { m_bWindowPresentationEnabled = bEnabled; }
  bool IsWindowPresentationEnabled() const { return m_bWindowPresentationEnabled; }
  void setVisible(bool bVisible) override;

public Q_SLOTS:
  void ShowCommandPalette();

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

private:
  friend class xiiQtDocumentWindow;
  friend class xiiQtApplicationPanel;

  xiiResult EnsureVisible(xiiQtDocumentWindow* pDocWindow);
  xiiResult EnsureVisible(xiiDocument* pDocument);
  xiiResult EnsureVisible(xiiQtApplicationPanel* pPanel);

  bool     m_bWindowLayoutRestored;
  xiiInt32 m_iWindowLayoutRestoreScheduled;
  bool     m_bWindowPresentationEnabled = true;

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotRestoreLayout();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);
  void SlotDockWidgetFloatingChanged(bool bFloating);

private:
  void                 UpdateWindowTitle();
  void                 UpdateDocumentMenu();

  void RemoveDocumentWindow(xiiQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanel(xiiQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(xiiQtDocumentWindow* pDocWindow);

  void DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e);
  void ProjectEventHandler(const xiiToolsProjectEvent& e);
  void UIServicesEventHandler(const xiiQtUiServices::Event& e);

  virtual void closeEvent(QCloseEvent* e) override;

private:
  ads::CDockManager*                    m_pDockManager = nullptr;
  QLabel*                               m_pStatusBarLabel;
  QLabel*                               m_pProjectLabel = nullptr;
  QMenuBar*                             m_pDocumentMenu = nullptr;
  QPointer<QMenuBar>                    m_pMenuSource;
  xiiDynamicArray<xiiQtDocumentWindow*> m_DocumentWindows;
  xiiDynamicArray<ads::CDockWidget*>    m_DocumentDocks;

  xiiDynamicArray<xiiQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString>                           m_DockNames;

  static xiiQtContainerWindow* s_pContainerWindow;
  static bool                  s_bForceClose;
};
