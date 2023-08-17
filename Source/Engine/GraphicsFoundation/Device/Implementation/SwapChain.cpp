#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/SwapChain.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALSwapChain, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALSwapChain::xiiGALSwapChain(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALObject<xiiGALSwapChainCreationDescription>(creationDescription)
{
}

xiiGALSwapChain::~xiiGALSwapChain() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_SwapChain);
