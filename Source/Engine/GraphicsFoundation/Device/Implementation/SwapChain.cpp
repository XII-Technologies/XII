#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSwapChain, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALSwapChain::xiiGALSwapChain(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALDeviceObject(pDevice), m_Description(creationDescription)
{
}

xiiGALSwapChain::~xiiGALSwapChain() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_SwapChain);
