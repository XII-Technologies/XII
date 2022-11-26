#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPluginWindowXR, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

xiiWindowXR::xiiWindowXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowBase> pCompanionWindow) :
  m_pVrInterface(pVrInterface), m_pCompanionWindow(std::move(pCompanionWindow))
{
}

xiiWindowXR::~xiiWindowXR()
{
  XII_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call xiiGALDevice::WaitIdle before destroying a window.");
}

xiiSizeU32 xiiWindowXR::GetClientAreaSize() const
{
  return m_pVrInterface->GetHmdInfo().m_vEyeRenderTargetSize;
}

xiiWindowHandle xiiWindowXR::GetNativeWindowHandle() const
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->GetNativeWindowHandle();
  }
  return xiiWindowHandle();
}

bool xiiWindowXR::IsFullscreenWindow(bool bOnlyProperFullscreenMode) const
{
  return true;
}

void xiiWindowXR::ProcessWindowMessages()
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->ProcessWindowMessages();
  }
}

const xiiWindowBase* xiiWindowXR::GetCompanionWindow() const
{
  return m_pCompanionWindow.Borrow();
}

//////////////////////////////////////////////////////////////////////////

xiiWindowOutputTargetXR::xiiWindowOutputTargetXR(xiiXRInterface* pXrInterface, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutputTarget) :
  m_pXrInterface(pXrInterface), m_pCompanionWindowOutputTarget(std::move(pCompanionWindowOutputTarget))
{
  if (m_pCompanionWindowOutputTarget)
  {
    // Create companion resources.
    m_hCompanionShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/VRCompanionView.xiiShader");
    XII_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiVRCompanionViewConstants>();
  }
}

xiiWindowOutputTargetXR::~xiiWindowOutputTargetXR()
{
  // Delete companion resources.
  xiiRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
  m_hCompanionConstantBuffer.Invalidate();
}

void xiiWindowOutputTargetXR::Present(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the xiiXRInterface now.
}

void xiiWindowOutputTargetXR::RenderCompanionView(bool bThrottleCompanionView)
{
  xiiTime currentTime = xiiTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + xiiTime::Milliseconds(16)))
    return;

  m_LastPresent = currentTime;

  XII_PROFILE_SCOPE("RenderCompanionView");
  xiiGALTextureHandle m_hColorRT = m_pXrInterface->GetCurrentTexture();
  if (m_hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  xiiGALDevice*     pDevice          = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext* m_pRenderContext = xiiRenderContext::GetDefaultInstance();

  {
    pDevice->BeginPipeline("VR CompanionView", m_pCompanionWindowOutputTarget->m_hSwapChain);

    auto pPass = pDevice->BeginPass("Blit CompanionView");

    const xiiGALSwapChain* pSwapChain             = xiiGALDevice::GetDefaultDevice()->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
    xiiGALTextureHandle    hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
    const xiiGALTexture*   tex                    = pDevice->GetTexture(hCompanionRenderTarget);
    auto                   hRenderTargetView      = xiiGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(hCompanionRenderTarget);
    xiiVec2                targetSize             = xiiVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hRenderTargetView);

    m_pRenderContext->BeginRendering(pPass, renderingSetup, xiiRectFloat(targetSize.x, targetSize.y));

    m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
    m_pRenderContext->BindConstantBuffer("xiiVRCompanionViewConstants", m_hCompanionConstantBuffer);
    m_pRenderContext->BindShader(m_hCompanionShader);

    auto* constants       = xiiRenderContext::GetConstantBufferData<xiiVRCompanionViewConstants>(m_hCompanionConstantBuffer);
    constants->TargetSize = targetSize;

    xiiGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
    m_pRenderContext->BindTexture2D("VRTexture", hInputView);
    m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    m_pRenderContext->EndRendering();

    pDevice->EndPass(pPass);

    pDevice->EndPipeline(m_pCompanionWindowOutputTarget->m_hSwapChain);
    m_pRenderContext->ResetContextState();
  }
}

xiiResult xiiWindowOutputTargetXR::CaptureImage(xiiImage& out_Image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    return m_pCompanionWindowOutputTarget->CaptureImage(out_Image);
  }
  return XII_FAILURE;
}

const xiiWindowOutputTargetBase* xiiWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

xiiActorPluginWindowXR::xiiActorPluginWindowXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowBase> companionWindow, xiiUniquePtr<xiiWindowOutputTargetGAL> companionWindowOutput) :
  m_pVrInterface(pVrInterface)
{
  m_pWindow             = XII_DEFAULT_NEW(xiiWindowXR, pVrInterface, std::move(companionWindow));
  m_pWindowOutputTarget = XII_DEFAULT_NEW(xiiWindowOutputTargetXR, pVrInterface, std::move(companionWindowOutput));
}

xiiActorPluginWindowXR::~xiiActorPluginWindowXR()
{
  m_pVrInterface->OnActorDestroyed();
}

void xiiActorPluginWindowXR::Initialize() {}

xiiWindowBase* xiiActorPluginWindowXR::GetWindow() const
{
  return m_pWindow.Borrow();
}

xiiWindowOutputTargetBase* xiiActorPluginWindowXR::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

void xiiActorPluginWindowXR::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRWindow);
