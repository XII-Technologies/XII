#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectEditTool, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGameObjectEditTool::xiiGameObjectEditTool() {}

void xiiGameObjectEditTool::ConfigureTool(
  xiiGameObjectDocument*         pDocument,
  xiiQtGameObjectDocumentWindow* pWindow,
  xiiGameObjectGizmoInterface*   pInterface)
{
  m_pDocument  = pDocument;
  m_pWindow    = pWindow;
  m_pInterface = pInterface;

  OnConfigured();
}

void xiiGameObjectEditTool::SetActive(bool active)
{
  if (m_bIsActive == active)
    return;

  m_bIsActive = active;
  OnActiveChanged(m_bIsActive);

  if (!m_bIsActive)
  {
    m_pWindow->SetPermanentStatusBarMsg("");
  }
}
