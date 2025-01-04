#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
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
  DiscardQueries();

  xiiGALQuery::Invalidate();
}

bool xiiGALQueryVulkan::OnBeginQuery(xiiGALCommandListVulkan* pCommandListVulkan)
{
  xiiGALQuery::OnBeginQuery(pCommandListVulkan);

  return AllocateQueries();
}

bool xiiGALQueryVulkan::OnEndQuery(xiiGALCommandListVulkan* pCommandListVulkan)
{
  xiiGALQuery::OnEndQuery(pCommandListVulkan);

  return false;
}

bool xiiGALQueryVulkan::AllocateQueries()
{
  return false;
}

bool xiiGALQueryVulkan::DiscardQueries()
{
  return false;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_QueryVulkan);
