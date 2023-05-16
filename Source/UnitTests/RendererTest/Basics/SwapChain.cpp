#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Basics/SwapChain.h>

xiiResult xiiRendererTestSwapChain::InitializeSubTest(xiiInt32 iIdentifier)
{
  m_iFrame = -1;

  if (xiiGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return XII_FAILURE;

  if (SetupRenderer().Failed())
    return XII_FAILURE;

  m_CurrentWindowSize = xiiSizeU32(320, 240);

  // Window
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = m_CurrentWindowSize.width;
    WindowCreationDesc.m_Resolution.height = m_CurrentWindowSize.height;
    WindowCreationDesc.m_WindowMode        = (iIdentifier == SubTests::ST_ResizeWindow) ? xiiWindowMode::WindowResizable : xiiWindowMode::WindowFixedResolution;
    // xiiGameStateWindow will write any window size changes into the config.
    m_pWindow = XII_DEFAULT_NEW(xiiGameStateWindow, WindowCreationDesc);
  }

  // SwapChain
  {
    xiiGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow            = m_pWindow;
    swapChainDesc.m_SampleCount        = xiiGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots  = true;
    swapChainDesc.m_InitialPresentMode = (iIdentifier == SubTests::ST_NoVSync) ? xiiGALPresentMode::Immediate : xiiGALPresentMode::VSync;
    m_hSwapChain                       = xiiGALWindowSwapChain::Create(swapChainDesc);
  }

  // Depth Texture
  if (iIdentifier != SubTests::ST_ColorOnly)
  {
    xiiGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth  = m_CurrentWindowSize.width;
    texDesc.m_uiHeight = m_CurrentWindowSize.height;
    switch (iIdentifier)
    {
      case SubTests::ST_D16:
        texDesc.m_Format = xiiGALResourceFormat::D16;
        break;
      case SubTests::ST_D24S8:
        texDesc.m_Format = xiiGALResourceFormat::D24S8;
        break;
      default:
      case SubTests::ST_D32:
        texDesc.m_Format = xiiGALResourceFormat::DFloat;
        break;
    }

    texDesc.m_bCreateRenderTarget = true;
    m_hDepthStencilTexture        = m_pDevice->CreateTexture(texDesc);
  }

  return XII_SUCCESS;
}

xiiResult xiiRendererTestSwapChain::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  DestroyWindow();
  ShutdownRenderer();

  if (xiiGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}


void xiiRendererTestSwapChain::ResizeTest(xiiUInt32 uiInvocationCount)
{
  if (uiInvocationCount == 4)
  {
    // Not implemented on all platforms, so we ignore the result here.
    m_pWindow->Resize(xiiSizeU32(640, 480)).IgnoreResult();
  }

  if (m_pWindow->GetClientAreaSize() != m_CurrentWindowSize)
  {
    m_CurrentWindowSize = m_pWindow->GetClientAreaSize();
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    // Swap Chain
    {
      auto presentMode = m_pDevice->GetSwapChain<xiiGALWindowSwapChain>(m_hSwapChain)->GetWindowDescription().m_InitialPresentMode;
      XII_TEST_RESULT(m_pDevice->UpdateSwapChain(m_hSwapChain, presentMode));
    }

    // Depth Texture
    {
      xiiGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth             = m_CurrentWindowSize.width;
      texDesc.m_uiHeight            = m_CurrentWindowSize.height;
      texDesc.m_Format              = xiiGALResourceFormat::DFloat;
      texDesc.m_bCreateRenderTarget = true;
      m_hDepthStencilTexture        = m_pDevice->CreateTexture(texDesc);
    }
  }
}

xiiTestAppRun xiiRendererTestSwapChain::BasicRenderLoop(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
  m_pPass = m_pDevice->BeginPass("SwapChainTest");
  {
    const xiiGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
    renderingSetup.m_ClearColor              = xiiColor::CornflowerBlue;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
      renderingSetup.m_bClearDepth   = true;
      renderingSetup.m_bClearStencil = true;
    }
    xiiRectFloat viewport = xiiRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    xiiRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    m_pWindow->ProcessWindowMessages();

    xiiRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  xiiTaskSystem::FinishFrameTasks();

  return m_iFrame < 120 ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}

static xiiRendererTestSwapChain g_SwapChainTest;
