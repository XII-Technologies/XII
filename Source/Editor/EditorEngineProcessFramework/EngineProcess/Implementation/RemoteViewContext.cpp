/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

xiiUInt32                          xiiRemoteEngineProcessViewContext::s_uiActiveViewID           = 0U;
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
  }
}

void xiiRemoteEngineProcessViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiActivateRemoteViewMsgToEngine>())
  {
    xiiRegisteredWindowHandle hWindow = xiiEditorEngineProcessApp::GetSingleton()->CreateRemoteWindow();
    XII_ASSERT_DEV(!hWindow.IsInvalidated(), "Failed to create remote window.");

    xiiWindowBase* pWindow = xiiWindowManager::GetSingleton()->GetWindow(hWindow);
    if (!pWindow->GetOutputTarget())
    {
      xiiGALSwapChainCreationDescription swapchainDescription;
      swapchainDescription.m_pWindow               = pWindow;
      swapchainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapchainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget;
      swapchainDescription.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapchainDescription.m_uiBufferCount         = 2U;
      swapchainDescription.m_fDefaultDepthValue    = 1.0f;
      swapchainDescription.m_uiDefaultStencilValue = 0U;

      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, swapchainDescription);
      pWindow->SetOutputTarget(std::move(pOutput));
    }

    s_pActiveRemoteViewContext = this;

    if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
    {
      if (xiiWorld* pWorld = pDocumentContext->GetWorld())
      {
        XII_LOCK(pWorld->GetWriteMarker());

        xiiRenderWorldModule* pRenderWorldModule = pWorld->GetOrCreateModule<xiiRenderWorldModule>();

        xiiView* pView = nullptr;
        if (!m_hView.IsInvalidated())
        {
          m_hView                                 = pRenderWorldModule->CreateView("Remote View", pView);
          xiiWindowOutputTargetGAL* pOutputTarget = static_cast<xiiWindowOutputTargetGAL*>(pWindow->GetOutputTarget());

          pView->SetSwapChain(pOutputTarget->GetSwapChain());
          pView->SetCamera(&m_Camera);

          s_uiActiveViewID = pMsg->m_uiViewID;
        }
        else if (pRenderWorldModule->TryGetView(m_hView, pView))
        {
          pView->SetCamera(&m_Camera);

          s_uiActiveViewID = pMsg->m_uiViewID;
        }
      }
    }

    // ignore all messages for views that are currently not activated
    if (pMsg->m_uiViewID != s_uiActiveViewID)
      return;

    if (const xiiViewRedrawMsgToEngine* pViewRedrawMsg = xiiDynamicCast<const xiiViewRedrawMsgToEngine*>(pMsg))
    {
      SetCamera(pViewRedrawMsg);

      // skip the on-message redraw, in remote mode it will just render as fast as it can
      // Redraw(false);
    }
  }
}

xiiViewHandle xiiRemoteEngineProcessViewContext::CreateView()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiViewHandle();
}
