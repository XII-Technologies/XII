#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALSwapChainVulkan::xiiGALSwapChainVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceVulkan, creationDescription)
{
}

xiiGALSwapChainVulkan::~xiiGALSwapChainVulkan() = default;

xiiResult xiiGALSwapChainVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  return CreateBackBufferInternal(pDeviceVulkan);
}

xiiResult xiiGALSwapChainVulkan::DeInitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainVulkan::CreateBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
  return XII_FAILURE;
}

void xiiGALSwapChainVulkan::DestroyBackBufferInternal(xiiGALDeviceVulkan* pDeviceVulkan)
{
}

void xiiGALSwapChainVulkan::AcquireNextRenderTarget()
{
}

void xiiGALSwapChainVulkan::Present()
{
}

xiiResult xiiGALSwapChainVulkan::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_SwapChainVulkan);
