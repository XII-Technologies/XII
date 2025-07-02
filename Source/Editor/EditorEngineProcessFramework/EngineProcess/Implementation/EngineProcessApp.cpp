#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
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
  // EditorRenderPipeline.xiiRenderPipelineAsset
  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }");
}

xiiRenderPipelineResourceHandle xiiEditorEngineProcessApp::CreateDefaultDebugRenderPipeline()
{
  // DebugRenderPipeline.xiiRenderPipelineAsset
  return xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }");
}

xiiViewHandle xiiEditorEngineProcessApp::CreateRemoteWindowAndView(xiiCamera* pCamera)
{
  XII_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  if (m_hRemoteView.IsInvalidated())
  {
    xiiActorPluginWindowOwner* pWindowPlugin = m_pActor->GetPlugin<xiiActorPluginWindowOwner>();

    // create output target
    {
      xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL);

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

    // get swapchain
    xiiGALSwapChainHandle hSwapChain;
    {
      xiiWindowOutputTargetGAL* pOutputTarget = static_cast<xiiWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());
      hSwapChain                              = pOutputTarget->m_hSwapChain;
    }

    // setup view
    {
      xiiView* pView = nullptr;
      m_hRemoteView  = xiiRenderWorld::CreateView("Remote Process", pView);

      // EditorRenderPipeline.xiiRenderPipelineAsset
      pView->SetRenderPipelineResource(xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

      const xiiSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();

      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)wndSize.width, (float)wndSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
