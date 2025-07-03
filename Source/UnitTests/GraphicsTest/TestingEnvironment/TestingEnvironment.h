#pragma once

#include <Core/System/Window.h>
#include <Foundation/Configuration/Singleton.h>
#include <GraphicsFoundation/Device/Device.h>

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

private:
  xiiUniquePtr<xiiWindow>       m_pWindow;
  xiiSharedPtr<xiiGALDevice>    m_pDevice;
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiSharedPtr<xiiGALTexture>   m_pDepthStencilTexture;
};
