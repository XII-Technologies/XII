#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

xiiGALBufferVulkan::xiiGALBufferVulkan(const xiiGALBufferCreationDescription& Description, bool bCPU) :
  xiiGALBuffer(Description), m_bCPU(bCPU)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan() {}

xiiResult xiiGALBufferVulkan::InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  m_pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);
  m_device        = m_pDeviceVulkan->GetVulkanDevice();

  m_stages = vk::PipelineStageFlagBits::eTransfer;

  switch (m_Description.m_BufferType)
  {
    case xiiGALBufferType::ConstantBuffer:
      m_usage = vk::BufferUsageFlagBits::eUniformBuffer;
      m_stages |= m_pDeviceVulkan->GetSupportedStages();
      m_access |= vk::AccessFlagBits::eUniformRead;
      break;
    case xiiGALBufferType::IndexBuffer:
      m_usage = vk::BufferUsageFlagBits::eIndexBuffer;
      m_stages |= vk::PipelineStageFlagBits::eVertexInput;
      m_access |= vk::AccessFlagBits::eIndexRead;
      m_indexType = m_Description.m_uiStructSize == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

      break;
    case xiiGALBufferType::VertexBuffer:
      m_usage = vk::BufferUsageFlagBits::eVertexBuffer;
      m_stages |= vk::PipelineStageFlagBits::eVertexInput;
      m_access |= vk::AccessFlagBits::eVertexAttributeRead;
      break;
    case xiiGALBufferType::Generic:
      m_usage = m_Description.m_bUseAsStructuredBuffer ? vk::BufferUsageFlagBits::eStorageBuffer : vk::BufferUsageFlagBits::eUniformTexelBuffer;
      m_stages |= m_pDeviceVulkan->GetSupportedStages();
      break;
    default:
      xiiLog::Error("Unknown buffer type supplied to CreateBuffer()!");
      return XII_FAILURE;
  }

  if (m_Description.m_bAllowShaderResourceView)
  {
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
  }

  if (m_Description.m_bAllowUAV)
  {
    m_stages |= m_pDeviceVulkan->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
  }

  if (m_Description.m_bStreamOutputTarget)
  {
    m_usage |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
    //#TODO_VULKAN will need to create a counter buffer.
    m_stages |= vk::PipelineStageFlagBits::eTransformFeedbackEXT;
    m_access |= vk::AccessFlagBits::eTransformFeedbackWriteEXT;
  }

  if (m_Description.m_bUseForIndirectArguments)
  {
    m_usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    m_stages |= vk::PipelineStageFlagBits::eDrawIndirect;
    m_access |= vk::AccessFlagBits::eIndirectCommandRead;
  }

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    m_usage |= vk::BufferUsageFlagBits::eTransferSrc;
    m_access |= vk::AccessFlagBits::eTransferRead;
  }

  m_usage |= vk::BufferUsageFlagBits::eTransferDst;
  m_access |= vk::AccessFlagBits::eTransferWrite;

  XII_ASSERT_DEBUG(pInitialData.GetCount() <= m_Description.m_uiTotalSize, "Initial data is bigger than target buffer.");
  vk::DeviceSize alignment = GetAlignment(m_pDeviceVulkan, m_usage);
  m_size                   = xiiMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  if (m_Description.m_bAllowRawViews)
  {
    // TODO Vulkan?
  }

  CreateBuffer();

  m_resourceBufferInfo.offset = 0;
  m_resourceBufferInfo.range  = m_size;

  if (!pInitialData.IsEmpty())
  {
    void* pData = nullptr;
    VK_ASSERT_DEV(xiiMemoryAllocatorVulkan::MapMemory(m_currentBuffer.m_alloc, &pData));
    XII_ASSERT_DEV(pData, "Implementation error");
    xiiMemoryUtils::Copy((xiiUInt8*)pData, pInitialData.GetPtr(), pInitialData.GetCount());
    xiiMemoryAllocatorVulkan::UnmapMemory(m_currentBuffer.m_alloc);
  }
  return XII_SUCCESS;
}

xiiResult xiiGALBufferVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  if (m_currentBuffer.m_buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_currentBuffer.m_buffer, m_currentBuffer.m_alloc);
    m_allocInfo = {};
  }
  for (auto& bufferVulkan : m_usedBuffers)
  {
    m_pDeviceVulkan->DeleteLater(bufferVulkan.m_buffer, bufferVulkan.m_alloc);
  }
  m_usedBuffers.Clear();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();

  m_stages    = {};
  m_access    = {};
  m_indexType = vk::IndexType::eUint16;
  m_usage     = {};
  m_size      = 0;

  m_pDeviceVulkan = nullptr;
  m_device        = nullptr;

  return XII_SUCCESS;
}

void xiiGALBufferVulkan::DiscardBuffer() const
{
  m_usedBuffers.PushBack(m_currentBuffer);
  m_currentBuffer = {};

  xiiUInt64 uiSafeFrame = m_pDeviceVulkan->GetSafeFrame();
  if (m_usedBuffers.PeekFront().m_currentFrame <= uiSafeFrame)
  {
    m_currentBuffer = m_usedBuffers.PeekFront();
    m_usedBuffers.PopFront();
    m_allocInfo = xiiMemoryAllocatorVulkan::GetAllocationInfo(m_currentBuffer.m_alloc);
  }
  else
  {
    CreateBuffer();
    SetDebugNamePlatform(m_sDebugName);
  }
}

const vk::DescriptorBufferInfo& xiiGALBufferVulkan::GetBufferInfo() const
{
  m_currentBuffer.m_currentFrame = m_pDeviceVulkan->GetCurrentFrame();
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = m_currentBuffer.m_buffer;
  return m_resourceBufferInfo;
}

void xiiGALBufferVulkan::CreateBuffer() const
{
  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage                 = m_usage;
  bufferCreateInfo.pQueueFamilyIndices   = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
  bufferCreateInfo.size                  = m_size;

  xiiVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = xiiVulkanMemoryUsage::Auto;
  if (m_bCPU)
  {
    allocCreateInfo.m_flags = xiiVulkanAllocationCreateFlags::HostAccessRandom;
  }
  else
  {
    allocCreateInfo.m_flags = xiiVulkanAllocationCreateFlags::HostAccessSequentialWrite;
  }
  VK_ASSERT_DEV(xiiMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_currentBuffer.m_buffer, m_currentBuffer.m_alloc, &m_allocInfo));
}

void xiiGALBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_sDebugName = szName;
  m_pDeviceVulkan->SetDebugName(szName, m_currentBuffer.m_buffer, m_currentBuffer.m_alloc);
}

vk::DeviceSize xiiGALBufferVulkan::GetAlignment(const xiiGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage)
{
  const vk::PhysicalDeviceProperties& properties = pDevice->GetPhysicalDeviceProperties();

  vk::DeviceSize alignment = xiiMath::Max<vk::DeviceSize>(4, properties.limits.nonCoherentAtomSize);

  if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
    alignment = xiiMath::Max(alignment, properties.limits.minUniformBufferOffsetAlignment);

  if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
    alignment = xiiMath::Max(alignment, properties.limits.minStorageBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer))
    alignment = xiiMath::Max(alignment, properties.limits.minTexelBufferOffsetAlignment);

  if (usage & (vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndirectBuffer))
    alignment = xiiMath::Max(alignment, VkDeviceSize(16));

  if (usage & (vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst))
    alignment = xiiMath::Max(alignment, properties.limits.optimalBufferCopyOffsetAlignment);

  return alignment;
}

XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_BufferVulkan);
