/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/System/Window.h>
#include <Foundation/Configuration/Singleton.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

class xiiGPUTestingEnvironmentInterface
{
public:
  virtual ~xiiGPUTestingEnvironmentInterface() = default;

  virtual xiiResult Initialize() = 0;
  virtual void      Shutdown()   = 0;

  virtual xiiResult CreateWindow(xiiUInt32 uiResolutionX = 960, xiiUInt32 uiResolutionY = 540) = 0;
  virtual void      DestroyWindow()                                                            = 0;

  virtual xiiWindow* GetWindow() const = 0;

  virtual xiiGALDevice* GetDevice() const = 0;

  virtual xiiGALSwapChain* GetSwapChain() const = 0;

  // Convenience helpers for tests
  virtual xiiResult CreateSwapChainForWindow(xiiUInt32 uiResolutionX = 960, xiiUInt32 uiResolutionY = 540) = 0;
  virtual void      DestroySwapChain()                                                                     = 0;

  virtual xiiSharedPtr<xiiGALTexture> GetBackBufferTexture() const   = 0;
  virtual xiiSharedPtr<xiiGALTexture> GetDepthStencilTexture() const = 0;

  virtual void BeginFrame() = 0;
  virtual void EndFrame()   = 0;

  virtual void Present() = 0;

  // Process window/OS messages to keep the environment responsive during tests
  virtual void ProcessWindowMessages() = 0;
};

class xiiGPUTestingEnvironmentVulkan final : public xiiGPUTestingEnvironmentInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiGPUTestingEnvironmentVulkan, xiiGPUTestingEnvironmentInterface);

public:
  xiiGPUTestingEnvironmentVulkan();

  virtual xiiResult Initialize() override final;
  virtual void      Shutdown() override final;

  virtual xiiResult CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY) override final;
  virtual void      DestroyWindow() override final;

  XII_ALWAYS_INLINE virtual xiiWindow* GetWindow() const override final { return m_pWindow.Borrow(); }

  XII_ALWAYS_INLINE virtual xiiGALDevice* GetDevice() const override final { return m_pDevice.Borrow(); }

  XII_ALWAYS_INLINE virtual xiiGALSwapChain* GetSwapChain() const override final { return m_pSwapChain.Borrow(); }

  virtual xiiResult CreateSwapChainForWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY) override final;
  virtual void      DestroySwapChain() override final;

  XII_ALWAYS_INLINE virtual xiiSharedPtr<xiiGALTexture> GetBackBufferTexture() const override final { return m_pSwapChain ? m_pSwapChain->GetBackBufferTexture() : xiiSharedPtr<xiiGALTexture>(); }
  XII_ALWAYS_INLINE virtual xiiSharedPtr<xiiGALTexture> GetDepthStencilTexture() const override final { return m_pDepthStencilTexture; }

  virtual void BeginFrame() override final;
  virtual void EndFrame() override final;

  virtual void Present() override final;

  virtual void ProcessWindowMessages() override final;

private:
  xiiUniquePtr<xiiWindow>       m_pWindow;
  xiiSharedPtr<xiiGALDevice>    m_pDevice;
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiSharedPtr<xiiGALTexture>   m_pDepthStencilTexture;
};
