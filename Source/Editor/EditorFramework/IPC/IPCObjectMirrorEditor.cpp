#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

xiiIPCObjectMirrorEditor::xiiIPCObjectMirrorEditor() :
  xiiDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

xiiIPCObjectMirrorEditor::~xiiIPCObjectMirrorEditor() = default;

void xiiIPCObjectMirrorEditor::SetIPC(xiiEditorEngineConnection* pIPC)
{
  XII_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

xiiEditorEngineConnection* xiiIPCObjectMirrorEditor::GetIPC()
{
  return m_pIPC;
}

void xiiIPCObjectMirrorEditor::ApplyOp(xiiObjectChange& ref_change)
{
  if (m_pManager)
  {
    SendOp(ref_change);
  }
  else
  {
    XII_REPORT_FAILURE("xiiIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}

void xiiIPCObjectMirrorEditor::SendOp(xiiObjectChange& change)
{
  XII_ASSERT_DEBUG(m_pIPC != nullptr, "Need to call SetIPC before SetReceiver");

  xiiEntityMsgToEngine msg;
  msg.m_change = std::move(change);

  m_pIPC->SendMessage(&msg);
}
