/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/System/Window.h>
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

class xiiGPUTestingEnvironment final : public xiiGPUTestingEnvironmentInterface
{
public:
  explicit xiiGPUTestingEnvironment(xiiStringView sImplementationName);

  ~xiiGPUTestingEnvironment();

  virtual xiiResult Initialize() override;

  virtual void Shutdown() override;

  virtual xiiStringView GetName() const override { return m_sImplementationName; }

  virtual xiiGALDevice* GetDevice() const override { return m_pDevice.Borrow(); }

  virtual xiiUniquePtr<xiiWindowBase> CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle) override;

private:
  xiiString                   m_sImplementationName;
  xiiSharedPtr<xiiGALDevice> m_pDevice;
};

/// Returns the number of graphics implementations selected for this run.
xiiUInt32 xiiGetGPUTestingEnvironmentCount();

/// Returns one selected graphics implementation. The pointer remains valid for the complete test run.
xiiGPUTestingEnvironmentInterface* xiiGetGPUTestingEnvironment(xiiUInt32 uiIndex);
