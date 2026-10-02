/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/System/Window.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

class xiiGPUTestingEnvironment
{
public:
  xiiGPUTestingEnvironment();

  ~xiiGPUTestingEnvironment();

  xiiResult Initialize();

  void Shutdown();

  xiiStringView GetName() const { return m_sImplementationName; }

  xiiStringView GetShaderModel() const { return m_sShaderModel; }

  xiiGALDevice* GetDevice() const { return m_pDevice.Borrow(); }

  xiiSharedPtr<xiiGALDevice> GetDeviceShared() const { return m_pDevice; }

  xiiUniquePtr<xiiWindowBase> CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle);

private:
  xiiString                  m_sImplementationName;
  xiiString                  m_sShaderModel;
  xiiSharedPtr<xiiGALDevice> m_pDevice;
};
