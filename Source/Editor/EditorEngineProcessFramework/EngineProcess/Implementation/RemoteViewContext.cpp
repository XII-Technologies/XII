#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiUInt32                          xiiRemoteEngineProcessViewContext::s_uiActiveViewID           = 0;
xiiRemoteEngineProcessViewContext* xiiRemoteEngineProcessViewContext::s_pActiveRemoteViewContext = nullptr;

xiiRemoteEngineProcessViewContext::xiiRemoteEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
}

xiiRemoteEngineProcessViewContext::~xiiRemoteEngineProcessViewContext()
{
  if (s_pActiveRemoteViewContext == this)
  {
    s_pActiveRemoteViewContext = nullptr;

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetWorld(nullptr);
    }
  }

  // make sure the base class destructor doesn't destroy the view
  m_hView.Invalidate();
}

void xiiRemoteEngineProcessViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = xiiEditorEngineProcessApp::GetSingleton()->CreateRemoteWindowAndView(&m_Camera);
    }

    s_pActiveRemoteViewContext = this;

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewRedrawMsgToEngine>())
  {
    const xiiViewRedrawMsgToEngine* pMsg2 = static_cast<const xiiViewRedrawMsgToEngine*>(pMsg);
    SetCamera(pMsg2);

    // skip the on-message redraw, in remote mode it will just render as fast as it can
    // Redraw(false);
  }
}

xiiViewHandle xiiRemoteEngineProcessViewContext::CreateView()
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiViewHandle();
}
