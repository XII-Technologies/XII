/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class XII_EDITORFRAMEWORK_DLL xiiIPCObjectMirrorEditor : public xiiDocumentObjectMirror
{
public:
  xiiIPCObjectMirrorEditor();
  ~xiiIPCObjectMirrorEditor();

  void                       SetIPC(xiiEditorEngineConnection* pIPC);
  xiiEditorEngineConnection* GetIPC();
  virtual void               ApplyOp(xiiObjectChange& ref_change) override;

private:
  void SendOp(xiiObjectChange& change);

  xiiEditorEngineConnection* m_pIPC;
};
