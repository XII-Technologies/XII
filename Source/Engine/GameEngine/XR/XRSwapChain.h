#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <RendererFoundation/Device/SwapChain.h>

class xiiXRInterface;

class XII_GAMEENGINE_DLL xiiGALXRSwapChain : public xiiGALSwapChain
{
public:
  using Functor = xiiDelegate<xiiGALSwapChainHandle(xiiXRInterface*)>;
  static void SetFactoryMethod(Functor factory);

  static xiiGALSwapChainHandle Create(xiiXRInterface* pXrInterface);

public:
  xiiGALXRSwapChain(xiiXRInterface* pXrInterface);
  virtual xiiResult UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode) override;

protected:
  static Functor s_Factory;

protected:
  xiiXRInterface* m_pXrInterface = nullptr;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiGALXRSwapChain);
