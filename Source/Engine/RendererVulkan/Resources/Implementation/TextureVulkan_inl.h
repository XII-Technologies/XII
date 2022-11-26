vk::Image xiiGALTextureVulkan::GetImage() const
{
  return m_image;
}

vk::ImageLayout xiiGALTextureVulkan::GetPreferredLayout() const
{
  return m_preferredLayout;
}

vk::ImageLayout xiiGALTextureVulkan::GetPreferredLayout(vk::ImageLayout targetLayout) const
{
  return targetLayout;
  //#TODO_VULKAN Maintaining UAVs in general layout causes verification failures. For now, switch back and forth between layouts.
  //return m_preferredLayout == vk::ImageLayout::eGeneral ? vk::ImageLayout::eGeneral : targetLayout;
}

vk::PipelineStageFlags xiiGALTextureVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags xiiGALTextureVulkan::GetAccessMask() const
{
  return m_access;
}

xiiVulkanAllocation xiiGALTextureVulkan::GetAllocation() const
{
  return m_alloc;
}

const xiiVulkanAllocationInfo& xiiGALTextureVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

bool xiiGALTextureVulkan::GetFormatOverrideEnabled() const
{
  return m_formatOverride;
}

bool xiiGALTextureVulkan::IsLinearLayout() const
{
  return m_bLinearCPU;
}

xiiGALTextureVulkan::StagingMode xiiGALTextureVulkan::GetStagingMode() const
{
  return m_stagingMode;
}

xiiGALTextureHandle xiiGALTextureVulkan::GetStagingTexture() const
{
  return m_hStagingTexture;
}

xiiGALBufferHandle xiiGALTextureVulkan::GetStagingBuffer() const
{
  return m_hStagingBuffer;
}
