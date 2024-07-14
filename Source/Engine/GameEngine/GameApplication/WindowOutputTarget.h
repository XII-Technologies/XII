#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Foundation/Math/Size.h>
#include <GraphicsFoundation/Device/SwapChain.h>

/// \brief Creates a swapchain and keeps it up to date with the window.
///
/// If the window is resized or xiiGameApplication::cvar_AppVSync changes and onSwapChainChanged is valid, the swapchain is destroyed and recreated. It is up the the application to respond to the OnSwapChainChanged callback and update any references to the swap-chain, e.g. uses in xiiView or uses as render targets in xiiGALRenderTargetSetup.
/// If onSwapChainChanged is not set, the swapchain will not be re-created and it is up to the application to manage the swapchain and react to window changes.
class XII_GAMEENGINE_DLL xiiWindowOutputTargetGAL : public xiiWindowOutputTargetBase
{
public:
  using OnSwapChainChanged = xiiDelegate<void(xiiGALSwapChainHandle hSwapChain, xiiSizeU32 size)>;

  xiiWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged = {});
  ~xiiWindowOutputTargetGAL();

  void CreateSwapchain(const xiiGALSwapChainCreationDescription& desc);

  virtual void      PresentImage(bool bEnableVSync) override;
  virtual void      AcquireImage() override;
  virtual xiiResult CaptureImage(xiiImage& out_image) override;

  OnSwapChainChanged                 m_OnSwapChainChanged;
  xiiSizeU32                         m_Size = xiiSizeU32(0, 0);
  xiiEnum<xiiGALPresentMode>         m_PresentMode;
  xiiGALSwapChainCreationDescription m_CurrentDesc;
  xiiGALSwapChainHandle              m_hSwapChain;
  xiiGALTextureHandle                m_hBackbufferStagingTexture;
};
