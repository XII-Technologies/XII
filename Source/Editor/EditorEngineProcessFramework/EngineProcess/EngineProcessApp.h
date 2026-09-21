/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Configuration/Singleton.h>

/// The mode in which the editor engine process application is running. In primary mode, the application runs in the same process as the editor and does not create a window or view for rendering the editor viewport.
/// In remote mode, the application runs in a separate process and creates a window and view that can be used to render the editor viewport.
enum class xiiEditorEngineProcessMode : xiiUInt8
{
  Primary = 0U, ///< The application runs in the same process as the editor and does not create a window or view for rendering the editor viewport.
  Remote,       ///< The application runs in a separate process and creates a window and view that can be used to render the editor viewport.
};

/// The main application class for the editor engine process. It is responsible for creating a window and view that can be used to render the editor viewport in a remote process.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineProcessApp
{
  XII_DECLARE_SINGLETON(xiiEditorEngineProcessApp);

public:
  xiiEditorEngineProcessApp();

  ~xiiEditorEngineProcessApp();

  /// Switches the application to remote mode, which creates a window and view that can be used to render the editor viewport in a remote process.
  void SetRemoteMode();

  /// Creates a window and view that can be used to render the editor viewport in a remote process. This is called when the application is switched to remote mode.
  virtual xiiRegisteredWindowHandle CreateRemoteWindow();

  /// Destroys the remote window and view, if they exist.
  void DestroyRemoteWindow();

  /// Returns true if the application is in remote mode.
  XII_ALWAYS_INLINE bool IsRemoteMode() const { return m_Mode == xiiEditorEngineProcessMode::Remote; }

protected:
  xiiEditorEngineProcessMode m_Mode = xiiEditorEngineProcessMode::Primary;

  xiiRegisteredWindowHandle m_hWindow;
};
