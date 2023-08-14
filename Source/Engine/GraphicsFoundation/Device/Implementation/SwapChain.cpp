#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/SwapChain.h>

xiiGALSwapChain::xiiGALSwapChain(const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALResource<xiiGALSwapChainCreationDescription>(creationDescription)
{
}

xiiGALSwapChain::~xiiGALSwapChain() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_SwapChain);
