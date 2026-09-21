/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

#include <QMainWindow>
#include <ads/DockManager.h>

class xiiQtContainerWindow;
class xiiDocument;
class xiiQtDocumentWindow;
class QLabel;
class QToolButton;

struct xiiQtDocumentWindowEvent
{
  enum Type
  {
    WindowClosing,           ///< Sent shortly before the window is being deleted
    WindowClosed,            ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
    WindowDecorationChanged, ///< Window title or icon has changed
    BeforeRedraw,            ///< Sent shortly before the content of the window is being redrawn
  };

  Type                 m_Type;
  xiiQtDocumentWindow* m_pWindow;
};

/// Base class for all document windows. Handles the most basic document window management.
class XII_GUIFOUNDATION_DLL xiiQtDocumentWindow : public QMainWindow
{
  Q_OBJECT

public:
  static xiiEvent<const xiiQtDocumentWindowEvent&> s_Events;

public:
  xiiQtDocumentWindow(xiiDocument* pDocument);
  xiiQtDocumentWindow(xiiStringView sUniqueName);
  virtual ~xiiQtDocumentWindow();

  ads::CDockManager* m_pDockManager = nullptr;

  void EnsureVisible();

  virtual xiiString GetWindowIcon() const;
  virtual xiiString GetDisplayName() const { return GetUniqueName(); }
  virtual xiiString GetDisplayNameShort() const;

  xiiStringView GetUniqueName() const { return m_sUniqueName; }

  /// The 'GroupName' is used for serializing window layouts. It should be unique among different window types.
  virtual xiiStringView GetWindowLayoutGroupName() const = 0;

  xiiDocument* GetDocument() const { return m_pDocument; }

  xiiStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  void ScheduleRestoreWindowLayout();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFrameRate(xiiUInt16 uiTargetFPS);
  void SetTargetFrameRateUnfocused(xiiUInt16 uiTargetFPS);

  void TriggerRedraw();

  virtual void RequestWindowTabContextMenu(const QPoint& globalPos);

  static const xiiDynamicArray<xiiQtDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  static xiiQtDocumentWindow* FindWindowByDocument(const xiiDocument* pDocument);
  xiiQtContainerWindow*       GetContainerWindow() const;

  /// Shows the given message for the given duration in the statusbar, then shows the permanent message again.
  void ShowTemporaryStatusBarMsg(const xiiFormatString& text, xiiTime duration = xiiTime::MakeFromSeconds(5));

  /// Sets which text to show permanently in the statusbar. Set an empty string to clear the message.
  void SetPermanentStatusBarMsg(const xiiFormatString& text);

  /// For unit tests to take a screenshot of the window (may include multiple views) to do image comparisons.
  virtual void CreateImageCapture(xiiStringView sOutputPath);

  /// In 'safe' mode we want to prevent the documents from using the stored window layout state
  static bool s_bAllowRestoreWindowLayout;

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void hideEvent(QHideEvent* event) override;
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  void FinishWindowCreation();

private Q_SLOTS:
  void SlotRestoreLayout();
  void SlotRedraw();
  void SlotQueuedDelete();
  void OnPermanentGlobalStatusClicked(bool);
  void OnStatusBarMessageChanged(const QString& sNewText);

private:
  void SaveWindowLayout();
  void RestoreWindowLayout(bool bForce);
  void DisableWindowLayoutSaving();

  void ShutdownDocumentWindow();

private:
  friend class xiiQtContainerWindow;

  void SetVisibleInContainer(bool bVisible);

  bool                  m_bWindowRestored              = false;
  bool                  m_bIsVisibleInContainer        = false;
  bool                  m_bRedrawIsTriggered           = false;
  bool                  m_bIsDrawingATM                = false;
  bool                  m_bTriggerRedrawQueued         = false;
  bool                  m_bAllowSaveWindowLayout       = true;
  xiiUInt16             m_uiTargetFrameRate            = 0U;
  xiiUInt16             m_uiTargetFrameRateUnfocused   = 0U;
  xiiDocument*          m_pDocument                    = nullptr;
  xiiQtContainerWindow* m_pContainerWindow             = nullptr;
  QLabel*               m_pPermanentDocumentStatusText = nullptr;
  QToolButton*          m_pPermanentGlobalStatusButton = nullptr;

private:
  void Constructor();
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& e);
  void DocumentEventHandler(const xiiDocumentEvent& e);
  void UIServicesEventHandler(const xiiQtUiServices::Event& e);
  void UIServicesTickEventHandler(const xiiQtUiServices::TickEvent& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) {}
  virtual void InternalRedraw() {}

  xiiString m_sUniqueName;

  static xiiDynamicArray<xiiQtDocumentWindow*> s_AllDocumentWindows;
};
