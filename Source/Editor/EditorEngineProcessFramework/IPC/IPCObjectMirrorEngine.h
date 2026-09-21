/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another one on the engine side as receiver.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiIPCObjectMirrorEngine : public xiiDocumentObjectMirror
{
public:
  xiiIPCObjectMirrorEngine();
  ~xiiIPCObjectMirrorEngine();

  virtual void ApplyOp(xiiObjectChange& inout_change) override;
};
