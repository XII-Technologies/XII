#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>

xiiEngineProcessViewContext::xiiEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext) :
  m_pDocumentContext(pContext)
{
  m_uiViewID = 0xFFFFFFFF;
}

xiiEngineProcessViewContext::~xiiEngineProcessViewContext()
{
  xiiRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();

  xiiActorManager::GetSingleton()->DestroyAllActors(this);
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

    auto*            pWindow = static_cast<xiiEditorProcessViewWindow*>(pWindowPlugin->GetWindow());
    const xiiSizeU32 wndSize = pWindow->GetClientAreaSize();

    XII_ASSERT_DEV(pWindow->GetNativeWindowHandle() == hWnd, "Editor view handle must never change. View needs to be destroyed and recreated.");

    if (wndSize.width == uiWidth && wndSize.height == uiHeight)
      return;

    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      xiiLog::Error("Failed to update Editor Process View Window");
    }
    return;
  }

  {
    // Create new actor
    xiiUniquePtr<xiiActor> pActor = XII_DEFAULT_NEW(xiiActor, "EditorView", this);
    m_pEditorWndActor             = pActor.Borrow();

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
        xiiLog::Error("Failed to create Editor Process View Window");
      }
    }

    // create output target
    {
      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, [this](xiiGALSwapChainHandle hSwapChain, xiiSizeU32 size) {
        OnSwapChainChanged(hSwapChain, size);
      });

      xiiGALSwapChainCreationDescription swapChainDesc;
      swapChainDesc.m_pWindow               = pWindowPlugin->m_pWindow.Borrow();
      swapChainDesc.m_Resolution            = pWindowPlugin->m_pWindow->GetClientAreaSize();
      swapChainDesc.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDesc.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget;
      swapChainDesc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapChainDesc.m_uiBufferCount         = 2U;
      swapChainDesc.m_fDefaultDepthValue    = 1.0f;
      swapChainDesc.m_uiDefaultStencilValue = 0U;

      pOutput->CreateSwapchain(swapChainDesc);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // setup render target
    {
      xiiWindowOutputTargetGAL* pOutput = static_cast<xiiWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());

      const xiiSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_hSwapChain, nullptr, static_cast<xiiUInt16>(wndSize.width), static_cast<xiiUInt16>(wndSize.height));
    }

    pActor->AddPlugin(std::move(pWindowPlugin));
    xiiActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void xiiEngineProcessViewContext::OnSwapChainChanged(xiiGALSwapChainHandle hSwapChain, xiiSizeU32 size)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
    pView->ForceUpdate();
  }
}

void xiiEngineProcessViewContext::SetupRenderTarget(xiiGALSwapChainHandle hSwapChain, const xiiGALRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  XII_LOG_BLOCK("xiiEngineProcessViewContext::SetupRenderTarget");
  XII_ASSERT_DEV((!hSwapChain.IsInvalidated() && pRenderTargets == nullptr) || (hSwapChain.IsInvalidated() && pRenderTargets != nullptr), "hSwapChain and pRenderTargets are mutually exclusive.");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      if (!hSwapChain.IsInvalidated())
        pView->SetSwapChain(hSwapChain);
      else
        pView->SetRenderTargets(*pRenderTargets);

      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void xiiEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    if (!bRenderEditorGizmos)
    {
      // exclude all editor objects from rendering in proper game views
      pView->m_ExcludeTags.Set(tagEditor);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagEditor);
    }

    xiiRenderWorld::AddMainView(m_hView);
  }
}

bool xiiEngineProcessViewContext::FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  xiiVec3 vDir       = vViewDir;
  bool    bChanged   = false;
  xiiVec3 vCameraPos = inout_camera.GetCenterPosition();
  xiiVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = xiiMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / xiiMath::Sin(xiiAngle::MakeFromDegree(fFov / 2));
  vDir.Normalize();
  xiiVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged   = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    inout_camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    inout_camera.LookAt(vNewCameraPos, vCenterPos, xiiVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void xiiEngineProcessViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  xiiViewRenderMode::Enum renderMode = (xiiViewRenderMode::Enum)pMsg->m_uiRenderMode;

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView) && pView->GetWorld() != nullptr)
  {
    if (renderMode == xiiViewRenderMode::None)
    {
      pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
    }
    else
    {
      pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
    }
  }

  if (m_Camera.GetCameraMode() != xiiCameraMode::Stereo)
  {
    bool bCameraIsActive = false;
    if (pView && pView->GetWorld())
    {
      xiiEnum<xiiCameraUsageHint> usageHint = pView->GetCameraUsageHint();
      xiiCameraComponent*         pComp     = pView->GetWorld()->GetOrCreateComponentManager<xiiCameraComponentManager>()->GetCameraByUsageHint(usageHint);
      bCameraIsActive                       = pComp != nullptr && pComp->IsActive();
    }

    // Camera mode should be controlled by a matching camera component if one exists.
    if (!bCameraIsActive)
    {
      xiiCameraMode::Enum cameraMode = (xiiCameraMode::Enum)pMsg->m_iCameraMode;
      m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
    }

    // prevent too large values
    // sometimes this can happen when imported data is badly scaled and thus way too large
    // then adding dirForwards result in no change and we run into other asserts later
    xiiVec3 pos = pMsg->m_vPosition;
    pos.x       = xiiMath::Clamp(pos.x, -1000000.0f, +1000000.0f);
    pos.y       = xiiMath::Clamp(pos.y, -1000000.0f, +1000000.0f);
    pos.z       = xiiMath::Clamp(pos.z, -1000000.0f, +1000000.0f);

    m_Camera.LookAt(pos, pos + pMsg->m_vDirForwards, pMsg->m_vDirUp);
  }

  if (pView)
  {
    pView->SetViewRenderMode(renderMode);

    bool bUseDepthPrePass = renderMode != xiiViewRenderMode::WireframeColor && renderMode != xiiViewRenderMode::WireframeMonochrome;
    pView->SetRenderPassProperty("DepthPrePass", "Active", bUseDepthPrePass);
    pView->SetRenderPassProperty("AOPass", "Active", bUseDepthPrePass); // Also disable SSAO to save some performance

    // by default this stuff is disabled, derived classes can enable it
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
  }
}

xiiRenderPipelineResourceHandle xiiEngineProcessViewContext::CreateDefaultRenderPipeline()
{
  return xiiEditorEngineProcessApp::GetSingleton()->CreateDefaultMainRenderPipeline();
}

xiiRenderPipelineResourceHandle xiiEngineProcessViewContext::CreateDebugRenderPipeline()
{
  return xiiEditorEngineProcessApp::GetSingleton()->CreateDefaultDebugRenderPipeline();
}

void xiiEngineProcessViewContext::DrawSimpleGrid() const
{
  xiiDynamicArray<xiiDebugRendererLine> lines;
  lines.Reserve(2 * (10 + 1 + 10) + 4);

  const xiiColor xAxisColor = xiiColorScheme::LightUI(xiiColorScheme::Red) * 0.7f;
  const xiiColor yAxisColor = xiiColorScheme::LightUI(xiiColorScheme::Green) * 0.7f;
  const xiiColor gridColor  = xiiColorScheme::LightUI(xiiColorScheme::Gray) * 0.5f;

  // arrows

  const float f = 1.0f;

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_vStart.Set(f, 0.0f, 0.0f);
    l.m_vEnd.Set(f - 0.25f, 0.25f, 0.0f);
    l.m_StartColor = xAxisColor;
    l.m_EndColor   = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_vStart.Set(f, 0.0f, 0.0f);
    l.m_vEnd.Set(f - 0.25f, -0.25f, 0.0f);
    l.m_StartColor = xAxisColor;
    l.m_EndColor   = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_vStart.Set(0.0f, f, 0.0f);
    l.m_vEnd.Set(0.25f, f - 0.25f, 0.0f);
    l.m_StartColor = yAxisColor;
    l.m_EndColor   = yAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_vStart.Set(0.0f, f, 0.0f);
    l.m_vEnd.Set(-0.25f, f - 0.25f, 0.0f);
    l.m_StartColor = yAxisColor;
    l.m_EndColor   = yAxisColor;
  }

  {
    const float x = 10.0f;

    for (xiiInt32 y = -10; y <= +10; ++y)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_vStart.Set((float)-x, (float)y, 0.0f);
      line.m_vEnd.Set((float)+x, (float)y, 0.0f);

      if (y == 0)
      {
        line.m_StartColor = xAxisColor;
      }
      else
      {
        line.m_StartColor = gridColor;
      }

      line.m_EndColor = line.m_StartColor;
    }
  }

  {
    const float y = 10.0f;

    for (xiiInt32 x = -10; x <= +10; ++x)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_vStart.Set((float)x, (float)-y, 0.0f);
      line.m_vEnd.Set((float)x, (float)+y, 0.0f);

      if (x == 0)
      {
        line.m_StartColor = yAxisColor;
      }
      else
      {
        line.m_StartColor = gridColor;
      }

      line.m_EndColor = line.m_StartColor;
    }
  }

  xiiDebugRenderer::DrawLines(m_hView, lines, xiiColor::White);
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Windows/EngineProcessViewContext_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
