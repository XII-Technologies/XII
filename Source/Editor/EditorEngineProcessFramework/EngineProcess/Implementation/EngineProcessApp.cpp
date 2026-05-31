/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

XII_IMPLEMENT_SINGLETON(xiiEditorEngineProcessApp);

xiiEditorEngineProcessApp::xiiEditorEngineProcessApp() :
  m_SingletonRegistrar(this)
{
}

xiiEditorEngineProcessApp::~xiiEditorEngineProcessApp()
{
  DestroyRemoteWindow();
}

void xiiEditorEngineProcessApp::SetRemoteMode()
{
  m_Mode = xiiEditorEngineProcessMode::Remote;

  CreateRemoteWindow();
}

void xiiEditorEngineProcessApp::CreateRemoteWindow()
{
  XII_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  if (m_pActor != nullptr)
    return;

  xiiUniquePtr<xiiActor> pActor = XII_DEFAULT_NEW(xiiActor, "Engine View", this);
  m_pActor                      = pActor.Borrow();

  // create window
  {
    xiiUniquePtr<xiiRemoteProcessWindow> pWindow = XII_DEFAULT_NEW(xiiRemoteProcessWindow);

    xiiWindowCreationDescription desc;
    desc.m_uiWindowNumber   = 0;
    desc.m_bClipMouseCursor = false;
    desc.m_bShowMouseCursor = true;
    desc.m_Resolution       = xiiSizeU32(1024, 768);
    desc.m_WindowMode       = xiiWindowMode::WindowFixedResolution;
    desc.m_Title            = "Engine View";

    pWindow->Initialize(desc).IgnoreResult();

    xiiUniquePtr<xiiActorPluginWindowOwner> pWindowPlugin = XII_DEFAULT_NEW(xiiActorPluginWindowOwner);
    pWindowPlugin->m_pWindow                              = std::move(pWindow);

    m_pActor->AddPlugin(std::move(pWindowPlugin));
  }

  xiiActorManager::GetSingleton()->AddActor(std::move(pActor));
}

void xiiEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (!m_hRemoteView.IsInvalidated())
  {
    xiiRenderWorld::DeleteView(m_hRemoteView);
    m_hRemoteView.Invalidate();
  }

  if (xiiActorManager::GetSingleton())
  {
    xiiActorManager::GetSingleton()->DestroyAllActors(this);
  }

  m_pActor = nullptr;
}

xiiRenderPipelineResourceHandle xiiEditorEngineProcessApp::CreateDefaultMainRenderPipeline()
{
  const auto* pConfig = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();

  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sEditorRenderPipeline);
}

xiiRenderPipelineResourceHandle xiiEditorEngineProcessApp::CreateDefaultDebugRenderPipeline()
{
  const auto* pConfig = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();

  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sDebugRenderPipeline);
}

xiiViewHandle xiiEditorEngineProcessApp::CreateRemoteWindowAndView(xiiCamera* pCamera)
{
  XII_ASSERT_DEV(IsRemoteMode(), "Incorrect application mode.");

  CreateRemoteWindow();

  if (m_hRemoteView.IsInvalidated())
  {
    xiiActorPluginWindowOwner* pWindowPlugin = m_pActor->GetPlugin<xiiActorPluginWindowOwner>();

    // Create output target.
    {
      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL);

      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = pWindowPlugin->m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
      swapChainDescription.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapChainDescription.m_uiBufferCount         = 2U;
      swapChainDescription.m_fDefaultDepthValue    = 1.0f;
      swapChainDescription.m_uiDefaultStencilValue = 0U;

      pOutput->CreateSwapchain(swapChainDescription);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // Retrieve swap chain.
    xiiSharedPtr<xiiGALSwapChain> pSwapChain;
    {
      xiiWindowOutputTargetGAL* pOutputTarget = static_cast<xiiWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());
      pSwapChain                              = pOutputTarget->m_pSwapChain;
    }

    // Setup view.
    {
      xiiView* pView = nullptr;
      m_hRemoteView  = xiiRenderWorld::CreateView("Remote Process", pView);

      const auto* pConfig         = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();
      auto        hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sEditorRenderPipeline);
      pView->SetRenderPipelineResource(hRenderPipeline);

      const xiiSizeU32 windowSize = pWindowPlugin->m_pWindow->GetClientAreaSize();

      pView->SetSwapChain(pSwapChain);
      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)windowSize.width, (float)windowSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
