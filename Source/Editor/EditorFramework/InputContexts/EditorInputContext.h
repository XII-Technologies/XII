/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Reflection/Reflection.h>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class xiiDocument;
class xiiQtEngineDocumentWindow;
class xiiQtEngineViewWidget;

enum class xiiEditorInput
{
  MayBeHandledByOthers,
  WasExclusivelyHandled,
};

class XII_EDITORFRAMEWORK_DLL xiiEditorInputContext : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorInputContext, xiiReflectedClass);

public:
  xiiEditorInputContext();

  virtual ~xiiEditorInputContext();

  void FocusLost(bool bCancel);

  xiiEditorInput KeyPressEvent(QKeyEvent* e) { return DoKeyPressEvent(e); }
  xiiEditorInput KeyReleaseEvent(QKeyEvent* e) { return DoKeyReleaseEvent(e); }
  xiiEditorInput MousePressEvent(QMouseEvent* e) { return DoMousePressEvent(e); }
  xiiEditorInput MouseReleaseEvent(QMouseEvent* e) { return DoMouseReleaseEvent(e); }
  xiiEditorInput MouseMoveEvent(QMouseEvent* e);
  xiiEditorInput WheelEvent(QWheelEvent* e) { return DoWheelEvent(e); }

  static void SetActiveInputContext(xiiEditorInputContext* pContext);

  void MakeActiveInputContext(bool bActive = true);

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static xiiEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext();

  bool IsActiveInputContext() const;

  void SetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView);

  xiiQtEngineDocumentWindow* GetOwnerWindow() const;

  xiiQtEngineViewWidget* GetOwnerView() const;

  bool GetShortcutsDisabled() const { return m_bDisableShortcuts; }

  /// If set to true, the surrounding window will ensure to block all shortcuts and instead send keypress events to the input context
  void SetShortcutsDisabled(bool bDisabled) { m_bDisableShortcuts = bDisabled; }

  virtual bool IsPickingSelectedAllowed() const { return true; }

  /// How the mouse position is updated when the mouse cursor reaches the screen borders.
  enum class MouseMode
  {
    Normal,                     ///< Nothing happens, the mouse will stop at screen borders as usual
    WrapAtScreenBorders,        ///< The mouse is visibly wrapped at screen borders. When this mode is disabled, the mouse stays where it is.
    HideAndWrapAtScreenBorders, ///< The mouse is wrapped at screen borders, which enables infinite movement, but the cursor is invisible. When this
                                ///< mode is disabled the mouse is restored to the position where it was when it was enabled.
  };

  /// Sets how the mouse will act when it reaches the screen border. UpdateMouseMode() must be called on every mouseMoveEvent to update the
  /// state.
  ///
  /// The return value is the current global mouse position. Can be used to initialize a 'Last Mouse Position' variable.
  xiiVec2I32 SetMouseMode(MouseMode mode);

  /// Updates the mouse position. Can always be called but will only have an effect if SetMouseMode() was called with one of the wrap modes.
  ///
  /// Returns the new global mouse position, which may change drastically if the mouse cursor needed to be wrapped around the screen.
  /// Should be used to update a "Last Mouse Position" variable.
  xiiVec2I32 UpdateMouseMode(QMouseEvent* e);

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) {}

protected:
  virtual void DoFocusLost(bool bCancel) {}

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) = 0;

  virtual void           OnActivated() {}
  virtual void           OnDeactivated() {}
  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e);
  virtual xiiEditorInput DoKeyReleaseEvent(QKeyEvent* e) { return xiiEditorInput::MayBeHandledByOthers; }
  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) { return xiiEditorInput::MayBeHandledByOthers; }
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) { return xiiEditorInput::MayBeHandledByOthers; }
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) { return xiiEditorInput::MayBeHandledByOthers; }
  virtual xiiEditorInput DoWheelEvent(QWheelEvent* e) { return xiiEditorInput::MayBeHandledByOthers; }

private:
  static xiiEditorInputContext* s_pActiveInputContext;

  xiiQtEngineDocumentWindow* m_pOwnerWindow;
  xiiQtEngineViewWidget*     m_pOwnerView;
  bool                       m_bDisableShortcuts;
  bool                       m_bJustWrappedMouse;
  MouseMode                  m_MouseMode;
  xiiVec2I32                 m_vMouseRestorePosition;
  xiiVec2I32                 m_vMousePosBeforeWrap;
  xiiVec2I32                 m_vExpectedMousePosition;
  xiiRectU32                 m_MouseWrapRect;

  virtual void UpdateContext() {}
};
