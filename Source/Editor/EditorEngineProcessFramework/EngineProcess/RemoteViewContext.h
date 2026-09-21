/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GraphicsCore/Pipeline/Declarations.h>

/// A view context used for rendering views in a remote process. It creates a window and view that can be used to render the editor viewport in a remote process.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiRemoteEngineProcessViewContext : public xiiEngineProcessViewContext
{
public:
  /// Creates a view context for a remote process. This creates a window and view that can be used to render the editor viewport in a remote process.
  xiiRemoteEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext);

  /// Destroys the view context and the associated window and view.
  ~xiiRemoteEngineProcessViewContext();

protected:
  /// Handles view messages from the editor process. This is used to update the camera and render target of the view based on the messages received from the editor process.
  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg) override;

  /// Sets up the render target for the view. This is called when the view is created and whenever the render target needs to be updated, e.g. when the window is resized.
  virtual xiiViewHandle CreateView() override;

protected:
  static xiiUInt32                          s_uiActiveViewID;
  static xiiRemoteEngineProcessViewContext* s_pActiveRemoteViewContext;
};
