/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <GraphicsFoundation/Device/SwapChain.h>

/// Creates a swapchain and keeps it up to date with the window.
///
/// If the window is resized or xiiGameApplication::cvar_AppVSync changes and onSwapChainChanged is valid, the swapchain is destroyed and recreated. It is up the the application to respond to the OnSwapChainChanged callback and update any references to the swap-chain, e.g. uses in xiiView or uses as render targets in xiiGALRenderTargetSetup.
///
/// If onSwapChainChanged is not set, the swapchain will not be re-created and it is up to the application to manage the swapchain and react to window changes.
class XII_GAMEENGINE_DLL xiiWindowOutputTargetGAL : public xiiWindowOutputTargetBase
{
public:
  /// Returns the current swapchain. This can be used by the application to access the back buffer texture for rendering, e.g. by using it as a render target.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALSwapChain* GetSwapChain() const { return m_pSwapChain; }

public:
  using OnSwapChainChanged = xiiDelegate<void(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 vSize)>;

  xiiWindowOutputTargetGAL(const xiiGALSwapChainCreationDescription& description, OnSwapChainChanged onSwapChainChanged = {});

  ~xiiWindowOutputTargetGAL();

  virtual bool GetVSyncEnabled() const override;

  virtual void SetVSyncEnabled(bool bEnableVSync) override;

  virtual void PresentImage() override;

  virtual void Resize(const xiiSizeU32& newSize) override;

  virtual xiiResult CaptureImage(xiiImage& out_image) override;

public:
  OnSwapChainChanged            m_OnSwapChainChanged; ///< This is called whenever the swapchain is created or resized. The application can use this to react to changes in the swapchain, e.g. by updating any references to it in xiiView.
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;         ///< The current swapchain. This is created and managed by xiiWindowOutputTargetGAL. The application can use this to access the back buffer texture for rendering, e.g. by using it as a render target.

  xiiUniquePtr<xiiGALImageCapture> m_pImageCapture;       ///< This is used to capture the back buffer for screenshots or similar purposes. It is created by xiiWindowOutputTargetGAL and can be used by the application to capture images from the swapchain.
  xiiUInt32                        m_uiCurrentFrame = 0U; ///< This is incremented every time PresentImage is called. It can be used by the application to track the current frame index for synchronization purposes, e.g. when using multiple frames in flight.
};
