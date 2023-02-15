#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRSwapChain.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALXRSwapChain, xiiGALSwapChain, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiGALXRSwapChain::Functor xiiGALXRSwapChain::s_Factory;

xiiGALXRSwapChain::xiiGALXRSwapChain(xiiXRInterface* pXrInterface) :
  xiiGALSwapChain(xiiGetStaticRTTI<xiiGALXRSwapChain>()), m_pXrInterface(pXrInterface)
{
}

xiiResult xiiGALXRSwapChain::UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  return XII_FAILURE;
}

void xiiGALXRSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

xiiGALSwapChainHandle xiiGALXRSwapChain::Create(xiiXRInterface* pXrInterface)
{
  XII_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for xiiGALXRSwapChain.");
  return s_Factory(pXrInterface);
}


XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRSwapChain);
