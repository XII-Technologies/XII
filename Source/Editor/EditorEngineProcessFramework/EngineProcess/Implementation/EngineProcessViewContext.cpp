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
      XII_LOCK(pWorld->GetReadMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        pRenderWorldModule->DestroyView(m_hView);

        m_hView.Invalidate();
      }
    }
  }

  xiiWindowManager::GetSingleton()->CloseAll(this);
}

void xiiEngineProcessViewContext::SetViewID(xiiUInt32 uiId)
{
  XII_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once");

  m_uiViewID = uiId;
}

void xiiEngineProcessViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewRedrawMsgToEngine>())
  {
    const xiiViewRedrawMsgToEngine* pMsg2 = static_cast<const xiiViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
      HandleWindowUpdate(reinterpret_cast<xiiWindowHandle>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  else
      xiiWindowHandle windowHandle;
      windowHandle.type                    = xiiWindowHandle::Type::XCB;
      windowHandle.xcbWindow.m_Window      = static_cast<xiiUInt32>(pMsg2->m_uiHWND);
      windowHandle.xcbWindow.m_pConnection = nullptr;
      HandleWindowUpdate(windowHandle, pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  endif
      Redraw(true);
    }
  }
  else if (const xiiViewScreenshotMsgToEngine* msg = xiiDynamicCast<const xiiViewScreenshotMsgToEngine*>(pMsg))
  {
    xiiImage              img;
    xiiActorPluginWindow* pWindow = m_pEditorWndActor->GetPlugin<xiiActorPluginWindow>();
    pWindow->GetOutputTarget()->CaptureImage(img).IgnoreResult();

    img.SaveTo(msg->m_sOutputFile).IgnoreResult();
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

  if (m_pEditorWndActor != nullptr)
  {
    // Update window size
    xiiActorPluginWindow* pWindowPlugin = m_pEditorWndActor->GetPlugin<xiiActorPluginWindow>();

    auto*            pWindow    = static_cast<xiiEditorProcessViewWindow*>(pWindowPlugin->GetWindow());
    const xiiSizeU32 windowSize = pWindow->GetClientAreaSize();

    XII_ASSERT_DEV(pWindow->GetNativeWindowHandle() == hWnd, "Editor view handle must never change. View needs to be destroyed and recreated.");

    if (windowSize.width == uiWidth && windowSize.height == uiHeight)
      return;

    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      xiiLog::Error("Failed to update Editor Process View Window");
    }
    return;
  }

  {
    // Create new actor
    xiiUniquePtr<xiiActor>                  pActor        = XII_DEFAULT_NEW(xiiActor, "EditorView", this);
    xiiUniquePtr<xiiActorPluginWindowOwner> pWindowPlugin = XII_DEFAULT_NEW(xiiActorPluginWindowOwner);

    // create window
    {
      xiiUniquePtr<xiiEditorProcessViewWindow> pWindow = XII_DEFAULT_NEW(xiiEditorProcessViewWindow);
      if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Succeeded())
      {
        pWindowPlugin->m_pWindow = std::move(pWindow);
      }
      else
      {
        xiiLog::Error("Failed to create Editor Process View Window.");
        return;
      }
    }

    // create output target
    {
      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, [this](xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 size) {
        OnSwapChainChanged(pSwapChain, size);
      });

      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = pWindowPlugin->m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
      swapChainDescription.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapChainDescription.m_uiBufferCount         = 2U;
      swapChainDescription.m_fDefaultDepthValue    = 1.0f;
      swapChainDescription.m_uiDefaultStencilValue = 0U;

      pOutput->CreateSwapchain(swapChainDescription);

      if (!pOutput->m_pSwapChain)
      {
        xiiLog::Error("Failed to create swapchain for Editor Process View Window.");
        return;
      }

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // setup render target
    {
      xiiWindowOutputTargetGAL* pOutput = static_cast<xiiWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());

      const xiiSizeU32 windowSize = pWindowPlugin->m_pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_pSwapChain, nullptr, static_cast<xiiUInt16>(windowSize.width), static_cast<xiiUInt16>(windowSize.height));
    }

    pActor->AddPlugin(std::move(pWindowPlugin));
    m_pEditorWndActor = pActor.Borrow();
    xiiActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void xiiEngineProcessViewContext::OnSwapChainChanged(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 size)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
    pView->ForceUpdate();
  }
}

void xiiEngineProcessViewContext::SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, const xiiRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  XII_LOG_BLOCK("xiiEngineProcessViewContext::SetupRenderTarget");
  XII_ASSERT_DEV((pSwapChain != nullptr && pRenderTargets == nullptr) || (pSwapChain == nullptr && pRenderTargets != nullptr), "hSwapChain and pRenderTargets are mutually exclusive.");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      if (pSwapChain != nullptr)
      {
        pView->SetSwapChain(pSwapChain);
      }
      else
      {
        pView->SetRenderTargets(*pRenderTargets);
      }

      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void xiiEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetReadMarker());

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
      XII_LOCK(pWorld->GetReadMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        pView->SetViewRenderMode(pMsg->m_RenderMode);

        if (m_Camera.GetCameraMode() != xiiCameraMode::Stereo)
        {
          if (xiiCameraComponentManager* pCameraManager = pWorld->GetModule<xiiCameraComponentManager>())
          {
            xiiCameraComponent* pCameraComponent = pCameraManager->GetCameraByUsageHint(pView->GetCameraUsageHint());

            // Camera mode should be controlled by a matching camera component if one exists.
            if (pCameraComponent == nullptr || !pCameraComponent->IsActive())
            {
              m_Camera.SetCameraMode(pMsg->m_CameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
            }
          }
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
