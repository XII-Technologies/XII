#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

xiiIPCObjectMirrorEngine::xiiIPCObjectMirrorEngine() :
  xiiDocumentObjectMirror()
{
}

xiiIPCObjectMirrorEngine::~xiiIPCObjectMirrorEngine() = default;

void xiiIPCObjectMirrorEngine::ApplyOp(xiiObjectChange& inout_change)
{
  if (m_pContext)
  {
    xiiDocumentObjectMirror::ApplyOp(inout_change);
  }
  else
  {
    XII_REPORT_FAILURE("xiiIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
