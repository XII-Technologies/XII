/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics/Platform/Windows/MinWindows.h>
#  include <Foundation/Types/ArrayPtr.h>
#  include <Foundation/Utilities/Progress.h>
#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <QAbstractNativeEventFilter>
#  include <QObject>
#  include <QPointer>
#  include <QTimer>
#  include <QWidget>

struct ITaskbarList3;
class xiiQtEditorApp;

/// Editor-owned taskbar presentation. Construct and destroy on the GUI thread, before the progress source is destroyed.
/// The window and progress source are borrowed; Qt alone owns and creates the native window.
class XII_EDITORFRAMEWORK_DLL xiiWindowsTaskbar final : public QObject, public QAbstractNativeEventFilter
{
public:
  xiiWindowsTaskbar(xiiQtEditorApp& editor, QWidget& window, xiiProgress& progress);
  ~xiiWindowsTaskbar();

private:
  friend struct xiiWindowsTaskbarTest;

  enum class ProgressState : xiiUInt8
  {
    Inactive,
    Normal,
    Indeterminate,
    Paused,
    Error
  };

  enum class Overlay : xiiUInt8
  {
    None,
    Warning,
    Error
  };

  struct State
  {
    ProgressState m_Progress    = ProgressState::Inactive;
    Overlay       m_Overlay     = Overlay::None;
    double        m_fCompletion = 0.0;
    bool          m_bHasValue   = false;

    bool operator==(const State&) const = default;
  };

  // Equal weight for each measured operation; unknown work stays indeterminate. Error > paused > unknown > normal.
  static State Combine(xiiArrayPtr<const State> states);
  void         ProgressEventHandler(const xiiProgressEvent& e);
  void         UiServicesEventHandler(const xiiQtUiServices::Event& e);
  void         Update();
  void         Apply();
  void         ReleaseTaskbar();
  void         UpdateWindow();
  bool         eventFilter(QObject* pObject, QEvent* pEvent) override;
  bool         nativeEventFilter(const QByteArray& eventType, void* pMessage, qintptr* pResult) override;

  QPointer<QWidget> m_pWindow;
  xiiProgress&      m_Progress;
  QTimer            m_UpdateTimer;
  State             m_Foreground;
  State             m_State;
  State             m_AppliedState;
  Overlay           m_StatusOverlay = Overlay::None;
  xiiUInt32         m_uiToolCount   = 0;
  bool              m_bToolError    = false;
  bool              m_bGlobalError  = false;

  ITaskbarList3*      m_pTaskbar               = nullptr;
  xiiMinWindows::HWND m_hWindow                = nullptr;
  xiiUInt32           m_uiTaskbarButtonCreated = 0;
  xiiUInt32           m_uiTaskbarCreated       = 0;
  bool                m_bComUsable             = false;
  bool                m_bUninitializeCom       = false;
  bool                m_bTaskbarReady          = false;
  bool                m_bApplied               = false;
  bool                m_bApplying              = false;
  bool                m_bResetTaskbar          = false;
};

#endif
