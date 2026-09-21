/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <QSet>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiDocumentManager;
class xiiDocument;
class xiiQtApplicationPanel;
struct xiiDocumentTypeDescriptor;
class QLabel;

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

  void SaveWindowLayout();
  void SaveDocumentLayouts();
  void RestoreWindowLayout();

  void ScheduleRestoreWindowLayout();

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

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotRestoreLayout();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);
  void SlotDockWidgetFloatingChanged(bool bFloating);

private:
  void UpdateWindowTitle();

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
  xiiDynamicArray<xiiQtDocumentWindow*> m_DocumentWindows;
  xiiDynamicArray<ads::CDockWidget*>    m_DocumentDocks;

  xiiDynamicArray<xiiQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString>                           m_DockNames;

  static xiiQtContainerWindow* s_pContainerWindow;
  static bool                  s_bForceClose;
};
