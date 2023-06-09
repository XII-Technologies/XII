#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALSwapChain, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALWindowSwapChain, xiiGALSwapChain, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiGALSwapChainCreationDescription CreateSwapChainCreationDescription(const xiiRTTI* pType)
{
  xiiGALSwapChainCreationDescription desc;
  desc.m_pSwapChainType = pType;
  return desc;
}

xiiGALSwapChain::xiiGALSwapChain(const xiiRTTI* pSwapChainType) :
  xiiGALObject(CreateSwapChainCreationDescription(pSwapChainType))
{
}

xiiGALSwapChain::~xiiGALSwapChain() = default;

//////////////////////////////////////////////////////////////////////////

xiiGALWindowSwapChain::Functor xiiGALWindowSwapChain::s_Factory;


xiiGALWindowSwapChain::xiiGALWindowSwapChain(const xiiGALWindowSwapChainCreationDescription& Description) :
  xiiGALSwapChain(xiiGetStaticRTTI<xiiGALWindowSwapChain>()), m_WindowDesc(Description)
{
}

void xiiGALWindowSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

xiiGALSwapChainHandle xiiGALWindowSwapChain::Create(const xiiGALWindowSwapChainCreationDescription& desc)
{
  XII_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for xiiGALWindowSwapChain.");
  return s_Factory(desc);
}

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);
