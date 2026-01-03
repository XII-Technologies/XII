#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Resource.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPluginWindowXR, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

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

    m_pCompanionConstantBuffer = xiiGALDeviceUtilities ::CreateConstantBuffer(xiiGALDevice::GetDefaultDevice(), sizeof(xiiVRCompanionViewConstants), XII_PP_STRINGIFY(xiiVRCompanionViewConstants));
  }
}

xiiWindowOutputTargetXR::~xiiWindowOutputTargetXR()
{
  m_pCompanionConstantBuffer.Clear();
}

void xiiWindowOutputTargetXR::PresentImage(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the xiiXRInterface now.
}

void xiiWindowOutputTargetXR::CompanionViewBeginFrame(bool bThrottleCompanionView)
{
  xiiTime currentTime = xiiTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + xiiTime::MakeFromMilliseconds(16)))
    return;

  m_LastPresent = currentTime;
  m_bRender     = true;
}

void xiiWindowOutputTargetXR::CompanionViewEndFrame()
{
  if (!m_bRender)
    return;

  m_bRender = false;

  XII_PROFILE_SCOPE("RenderCompanionView");
  xiiSharedPtr<xiiGALTexture> pColorRT = m_pXrInterface->GetCurrentTexture();
  if (!pColorRT || !m_pCompanionWindowOutputTarget)
    return;

  xiiRenderContext* pRenderContext = xiiRenderContext::GetDefaultInstance();
  {
    xiiSharedPtr<xiiGALTexture>             pBackBufferTexture = m_pCompanionWindowOutputTarget->m_pSwapChain->GetBackBufferTexture();
    const xiiGALTextureCreationDescription& textureDescription = pBackBufferTexture->GetDescription();
    xiiVec2                                 vTargetSize        = xiiVec2((float)textureDescription.GetWidth(), (float)textureDescription.GetHeight());

    xiiRenderingSetup renderingSetup;
    renderingSetup.AddColorAttachment({m_pCompanionWindowOutputTarget->m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget)});

    pRenderContext->BeginRendering(std::move(renderingSetup), xiiRectFloat(vTargetSize.x, vTargetSize.y), "Blit CompanionView");
    pRenderContext->BindShader(m_hCompanionShader);
    pRenderContext->BindBuffer("xiiVRCompanionViewConstants", m_pCompanionConstantBuffer);
    pRenderContext->BindTexture("VRTexture", pColorRT);
    {
      xiiGALMapHelper<xiiVRCompanionViewConstants> pVRCompanionViewConstants(pRenderContext->GetCommandList(), m_pCompanionConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pVRCompanionViewConstants->TargetSize = vTargetSize;
    }
    pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);
    pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

xiiResult xiiWindowOutputTargetXR::CaptureImage(xiiImage& out_image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    // If we are capturing an image, we need to update the companion view first.
    // If not, CompanionViewEndFrame will be called by the XR implementation.
    CompanionViewEndFrame();

    return m_pCompanionWindowOutputTarget->CaptureImage(out_image);
  }
  return XII_FAILURE;
}

const xiiWindowOutputTargetBase* xiiWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

xiiActorPluginWindowXR::xiiActorPluginWindowXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowBase> pCompanionWindow, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutput) :
  m_pVrInterface(pVrInterface)
{
  m_pWindow             = XII_DEFAULT_NEW(xiiWindowXR, pVrInterface, std::move(pCompanionWindow));
  m_pWindowOutputTarget = XII_DEFAULT_NEW(xiiWindowOutputTargetXR, pVrInterface, std::move(pCompanionWindowOutput));
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
