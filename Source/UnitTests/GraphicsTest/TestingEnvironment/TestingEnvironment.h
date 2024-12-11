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

  virtual xiiGALSwapChainHandle GetSwapChainHandle() const = 0;
};

class xiiGPUTestingEnvironmentD3D11 final : public xiiGPUTestingEnvironmentInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiGPUTestingEnvironmentD3D11, xiiGPUTestingEnvironmentInterface);

public:
  xiiGPUTestingEnvironmentD3D11();

  virtual xiiResult Initialize() override final;
  virtual void      Shutdown() override final;

  virtual xiiResult CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY) override final;
  virtual void      DestroyWindow() override final;

  XII_ALWAYS_INLINE virtual xiiWindow* GetWindow() const override final { return m_pWindow; }

  XII_ALWAYS_INLINE virtual xiiGALDevice* GetDevice() const override final { return m_pDevice; }

  XII_ALWAYS_INLINE virtual xiiGALSwapChainHandle GetSwapChainHandle() const override final { return m_hSwapChain; }

private:
  xiiWindow*    m_pWindow = nullptr;
  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;
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

  XII_ALWAYS_INLINE virtual xiiWindow* GetWindow() const override final { return m_pWindow; }

  XII_ALWAYS_INLINE virtual xiiGALDevice* GetDevice() const override final { return m_pDevice; }

  XII_ALWAYS_INLINE virtual xiiGALSwapChainHandle GetSwapChainHandle() const override final { return m_hSwapChain; }

private:
  xiiWindow*    m_pWindow = nullptr;
  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;
};
