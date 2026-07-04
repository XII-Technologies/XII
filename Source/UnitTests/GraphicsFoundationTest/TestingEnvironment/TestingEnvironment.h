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

  virtual xiiStringView GetName() const = 0;

  virtual xiiGALDevice* GetDevice() const = 0;

  virtual xiiResult Initialize() = 0;

  virtual void Shutdown() = 0;

  virtual xiiUniquePtr<xiiWindowBase> CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle = {}) = 0;
};

class xiiGPUTestingEnvironmentVulkan final : public xiiGPUTestingEnvironmentInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiGPUTestingEnvironmentVulkan, xiiGPUTestingEnvironmentInterface);

public:
  xiiGPUTestingEnvironmentVulkan();

  ~xiiGPUTestingEnvironmentVulkan();

  virtual xiiResult Initialize() override;

  virtual void Shutdown() override;

  virtual xiiStringView GetName() const override { return "Vulkan"; }

  virtual xiiGALDevice* GetDevice() const override { return m_pDevice.Borrow(); }

  virtual xiiUniquePtr<xiiWindowBase> CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle) override;

private:
  xiiSharedPtr<xiiGALDevice> m_pDevice;
};

class xiiGPUTestingEnvironmentD3D12 final : public xiiGPUTestingEnvironmentInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiGPUTestingEnvironmentD3D12, xiiGPUTestingEnvironmentInterface);

public:
  xiiGPUTestingEnvironmentD3D12();

  ~xiiGPUTestingEnvironmentD3D12();

  virtual xiiStringView GetName() const override { return "Direct3D 12"; }

  virtual xiiGALDevice* GetDevice() const override { return m_pDevice.Borrow(); }

  virtual xiiResult Initialize() override;

  virtual void Shutdown() override;

  virtual xiiUniquePtr<xiiWindowBase> CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle) override;

private:
  xiiSharedPtr<xiiGALDevice> m_pDevice;
};
