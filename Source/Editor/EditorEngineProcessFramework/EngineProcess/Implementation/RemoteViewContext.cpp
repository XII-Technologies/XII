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
#include <Texture/Image/Image.h>

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
  if (xiiDynamicCast<const xiiActivateRemoteViewMsgToEngine*>(pMsg))
  {
    auto*          pManager = xiiWindowManager::GetSingleton();
    const auto     hWindow  = xiiEditorEngineProcessApp::GetSingleton()->CreateRemoteWindow();
    xiiWindowBase* pWindow  = pManager->GetWindow(hWindow);
    if (!pWindow)
    {
      xiiLog::Error("Failed to create remote view window.");
      return;
    }

    if (!pWindow->GetOutputTarget())
    {
      xiiGALSwapChainCreationDescription description;
      description.m_pWindow                          = pWindow;
      description.m_ColorBufferFormat                = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      description.m_UsageFlags                       = xiiGALSwapChainUsageFlags::RenderTarget;
      description.m_PreTransform                     = xiiGALSurfaceTransform::Optimal;
      description.m_uiBufferCount                    = 2;
      description.m_fDefaultDepthValue               = 0.0f;
      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, description);
      if (!pOutput->GetSwapChain())
        return;
      pWindow->SetOutputTarget(std::move(pOutput));
    }

    // A remote window is shared between documents; only its active view may render into it.
    if (s_pActiveRemoteViewContext && s_pActiveRemoteViewContext != this)
    {
      auto* pPreviousWorld = s_pActiveRemoteViewContext->GetDocumentContext()->GetWorld();
      XII_LOCK(pPreviousWorld->GetWriteMarker());
      if (auto* pPreviousView = s_pActiveRemoteViewContext->GetView())
      {
        pPreviousView->SetRenderEnabled(false);
        pPreviousView->SetSwapChain(nullptr);
      }
    }

    auto* pWorld = GetDocumentContext()->GetWorld();
    if (!pWorld)
      return;
    XII_LOCK(pWorld->GetWriteMarker());
    if (m_hView.IsInvalidated())
      m_hView = CreateView();
    if (auto* pView = GetView())
    {
      auto*      pOutput = static_cast<xiiWindowOutputTargetGAL*>(pWindow->GetOutputTarget());
      const auto size    = pWindow->GetClientAreaSize();
      pView->SetSwapChain(pOutput->GetSwapChain());
      pView->SetCamera(&m_Camera);
      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height)));
      s_pActiveRemoteViewContext = this;
      s_uiActiveViewID           = pMsg->m_uiViewID;
      m_hEditorWindow            = hWindow;
      pView->SetRenderEnabled(true);
    }
    return;
  }

  // View IDs are document-local, so both the context and the ID must match.
  if (s_pActiveRemoteViewContext != this || pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (const auto* pRedraw = xiiDynamicCast<const xiiViewRedrawMsgToEngine*>(pMsg))
  {
    SetCamera(pRedraw);
    Redraw(false);
  }
  else if (const auto* pScreenshot = xiiDynamicCast<const xiiViewScreenshotMsgToEngine*>(pMsg))
  {
    const auto hWindow = xiiEditorEngineProcessApp::GetSingleton()->CreateRemoteWindow();
    auto*      pWindow = xiiWindowManager::GetSingleton()->GetWindow(hWindow);
    xiiImage   image;
    if (pWindow && pWindow->GetOutputTarget() && pWindow->GetOutputTarget()->CaptureImage(image).Succeeded())
    {
      if (image.SaveTo(pScreenshot->m_sOutputFile).Failed())
        xiiLog::Error("Failed to save remote view screenshot to '{0}'.", pScreenshot->m_sOutputFile);
    }
  }
}

void xiiRemoteEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  if (s_pActiveRemoteViewContext == this)
    xiiEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

xiiViewHandle xiiRemoteEngineProcessViewContext::CreateView()
{
  auto* pWorld = GetDocumentContext()->GetWorld();
  if (!pWorld)
    return {};
  XII_LOCK(pWorld->GetWriteMarker());
  xiiView*   pView  = nullptr;
  const auto handle = pWorld->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Remote View", pView);
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  return handle;
}
