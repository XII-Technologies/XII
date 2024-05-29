#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALTextureVulkan::xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceVulkan, creationDescription)
{
}

xiiGALTextureVulkan::~xiiGALTextureVulkan() = default;

xiiResult xiiGALTextureVulkan::InitPlatform(const xiiGALTextureData* pInitialData)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALTextureVulkan::DeInitPlatform()
{
  // Prevent releasing native objects.
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
