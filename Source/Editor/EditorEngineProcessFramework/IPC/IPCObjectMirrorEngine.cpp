#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

xiiIPCObjectMirrorEngine::xiiIPCObjectMirrorEngine() :
  xiiDocumentObjectMirror()
{
}

xiiIPCObjectMirrorEngine::~xiiIPCObjectMirrorEngine() {}

void xiiIPCObjectMirrorEngine::ApplyOp(xiiObjectChange& change)
{
  if (m_pContext)
  {
    xiiDocumentObjectMirror::ApplyOp(change);
  }
  else
  {
    XII_REPORT_FAILURE("xiiIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
