#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQueryVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALQueryVulkan::xiiGALQueryVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(pDeviceVulkan, creationDescription)
{
}

xiiGALQueryVulkan::~xiiGALQueryVulkan() = default;

xiiResult xiiGALQueryVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALQueryVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

void xiiGALQueryVulkan::SetDebugNamePlatform(xiiStringView sName)
{
}

bool xiiGALQueryVulkan::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  CheckQueryDataPtr(pData, uiDataSize);

  return false;
}

void xiiGALQueryVulkan::Invalidate()
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_QueryVulkan);
