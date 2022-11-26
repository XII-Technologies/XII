
vk::Buffer xiiGALBufferVulkan::GetVkBuffer() const
{
  m_currentBuffer.m_currentFrame = m_pDeviceVulkan->GetCurrentFrame();
  return m_currentBuffer.m_buffer;
}

vk::IndexType xiiGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

xiiVulkanAllocation xiiGALBufferVulkan::GetAllocation() const
{
  return m_currentBuffer.m_alloc;
}

const xiiVulkanAllocationInfo& xiiGALBufferVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

vk::PipelineStageFlags xiiGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags xiiGALBufferVulkan::GetAccessMask() const
{
  return m_access;
}
