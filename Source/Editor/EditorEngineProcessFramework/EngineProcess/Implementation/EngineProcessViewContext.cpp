/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>

xiiEngineProcessViewContext::xiiEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext) :
  m_pDocumentContext(pContext), m_uiViewID(0xFFFFFFFFU)
{
}

xiiEngineProcessViewContext::~xiiEngineProcessViewContext()
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        pRenderWorldModule->DestroyView(m_hView);

        m_hView.Invalidate();
      }
    }
  }

  if (auto* manager = xiiWindowManager::GetSingleton())
    manager->CloseAll(this);
}

void xiiEngineProcessViewContext::SetViewID(xiiUInt32 uiId)
{
  XII_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once.");

  m_uiViewID = uiId;
}

void xiiEngineProcessViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)
  if (const xiiViewRedrawMsgToEngine* pViewRedrawMsg = xiiDynamicCast<const xiiViewRedrawMsgToEngine*>(pMsg))
  {
    if (pViewRedrawMsg->m_uiWindowWidth > 0 && pViewRedrawMsg->m_uiWindowHeight > 0)
    {
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
      HandleWindowUpdate(reinterpret_cast<xiiWindowHandle>(pViewRedrawMsg->m_uiHWND), pViewRedrawMsg->m_uiWindowWidth, pViewRedrawMsg->m_uiWindowHeight);
#  else
      xiiWindowHandle windowHandle;
      windowHandle.type                    = xiiWindowHandle::Type::XCB;
      windowHandle.xcbWindow.m_Window      = static_cast<xiiUInt32>(pViewRedrawMsg->m_uiHWND);
      windowHandle.xcbWindow.m_pConnection = nullptr;
      HandleWindowUpdate(windowHandle, pViewRedrawMsg->m_uiWindowWidth, pViewRedrawMsg->m_uiWindowHeight);
#  endif
    }
    // The first redraw creates the view; apply the message camera to that new view as well.
    SetCamera(pViewRedrawMsg);
    if (pViewRedrawMsg->m_uiWindowWidth > 0 && pViewRedrawMsg->m_uiWindowHeight > 0)
      Redraw(true);
  }
  else if (const xiiViewScreenshotMsgToEngine* pViewScreenshotMsg = xiiDynamicCast<const xiiViewScreenshotMsgToEngine*>(pMsg))
  {
    xiiImage img;
    auto*    window = xiiWindowManager::GetSingleton()->GetWindow(m_hEditorWindow);
    if (window && window->GetOutputTarget() && window->GetOutputTarget()->CaptureImage(img).Succeeded())
    {
      if (img.SaveTo(pViewScreenshotMsg->m_sOutputFile).Failed())
        xiiLog::Error("Failed to save view screenshot to '{0}'.", pViewScreenshotMsg->m_sOutputFile);
    }
  }
#else
#  error "Unsupported platform."
#endif
}

void xiiEngineProcessViewContext::SendViewMessage(xiiEditorEngineViewMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();
  pViewMsg->m_uiViewID     = m_uiViewID;

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void xiiEngineProcessViewContext::HandleWindowUpdate(xiiWindowHandle hWnd, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  XII_LOG_BLOCK("xiiEngineProcessViewContext::HandleWindowUpdate");

  auto* manager = xiiWindowManager::GetSingleton();
  if (auto* window = static_cast<xiiEditorProcessViewWindow*>(manager->GetWindow(m_hEditorWindow)))
  {
    if (window->GetClientAreaSize() == xiiSizeU32(uiWidth, uiHeight))
      return;
    if (window->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      xiiLog::Error("Failed to resize Editor Process View Window.");
      return;
    }
    window->GetOutputTarget()->Resize(window->GetClientAreaSize());
    return;
  }

  xiiUniquePtr<xiiEditorProcessViewWindow> window = XII_DEFAULT_NEW(xiiEditorProcessViewWindow);
  if (window->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
  {
    xiiLog::Error("Failed to create Editor Process View Window.");
    return;
  }

  xiiGALSwapChainCreationDescription description;
  description.m_pWindow                         = window.Borrow();
  description.m_ColorBufferFormat               = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  description.m_UsageFlags                      = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
  description.m_PreTransform                    = xiiGALSurfaceTransform::Optimal;
  description.m_uiBufferCount                   = 2U;
  description.m_fDefaultDepthValue              = 0.0f;
  description.m_uiDefaultStencilValue           = 0U;
  xiiUniquePtr<xiiWindowOutputTargetGAL> output = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, description,
                                                                  [this](xiiSharedPtr<xiiGALSwapChain> swapChain, xiiSizeU32 size) { OnSwapChainChanged(swapChain, size); });
  if (!output->GetSwapChain())
  {
    xiiLog::Error("Failed to create swapchain for Editor Process View Window.");
    return;
  }
  SetupRenderTarget(output->m_pSwapChain, nullptr, uiWidth, uiHeight);
  window->SetOutputTarget(std::move(output));
  m_hEditorWindow = manager->Register("Editor View", this, std::move(window));
}

void xiiEngineProcessViewContext::OnSwapChainChanged(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 size)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        pView->SetSwapChain(pSwapChain.Borrow());
        pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
      }
    }
  }
}

xiiView* xiiEngineProcessViewContext::GetView() const
{
  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return nullptr;
  const auto* module = static_cast<const xiiWorld*>(world)->GetModule<xiiRenderWorldModule>();
  xiiView*    view   = nullptr;
  if (module)
    module->TryGetView(m_hView, view);
  return view;
}

void xiiEngineProcessViewContext::SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiGALTextureView* pRenderTargetView, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  XII_ASSERT_DEV((pSwapChain != nullptr) != (pRenderTargetView != nullptr), "Supply exactly one swapchain or offscreen target.");
  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return;
  XII_LOCK(world->GetWriteMarker());
  if (m_hView.IsInvalidated())
    m_hView = CreateView();
  if (auto* view = GetView())
  {
    view->SetSwapChain(pSwapChain.Borrow());
    view->SetRenderTargetView(pRenderTargetView);
    view->SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight)));
  }
}

void xiiEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        const xiiTag& editorTag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

        if (bRenderEditorGizmos)
        {
          pView->m_ExcludeTags.Remove(editorTag);
        }
        else
        {
          pView->m_ExcludeTags.Set(editorTag);
        }
      }
    }
  }
}

bool xiiEngineProcessViewContext::FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  bool    bChanged        = false;
  xiiVec3 vCameraPosition = inout_camera.GetCenterPosition();
  xiiVec3 vCenterPosition = objectBounds.GetSphere().m_vCenter;

  const float fDistance     = xiiMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / xiiMath::Sin(xiiAngle::MakeFromDegree(fFov / 2));
  xiiVec3     vNewCameraPos = vCenterPosition - vViewDir.GetNormalized() * fDistance;
  if (!vNewCameraPos.IsEqual(vCameraPosition, 0.01f))
  {
    vCameraPosition = vNewCameraPos;
    bChanged        = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    inout_camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    inout_camera.LookAt(vNewCameraPos, vCenterPosition, xiiVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void xiiEngineProcessViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        pView->SetViewRenderMode(pMsg->m_RenderMode);

        if (m_Camera.GetCameraMode() != xiiCameraMode::Stereo)
        {
          xiiCameraComponentManager* pCameraManager   = pWorld->GetModule<xiiCameraComponentManager>();
          xiiCameraComponent*        pCameraComponent = pCameraManager ? pCameraManager->GetCameraByUsageHint(pView->GetCameraUsageHint()) : nullptr;

          // Camera mode should be controlled by a matching camera component if one exists.
          if (pCameraComponent == nullptr || !pCameraComponent->IsActive())
          {
            m_Camera.SetCameraMode(pMsg->m_CameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
          }
          if (pMsg->m_bUseCameraTransformOnDevice)
            m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
        }
      }
    }
  }
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Windows/EngineProcessViewContext_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
