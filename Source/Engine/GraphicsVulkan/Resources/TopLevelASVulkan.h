#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTopLevelASVulkan final : public xiiGALTopLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTopLevelASVulkan, xiiGALTopLevelAS);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::AccelerationStructureKHR GetVulkanAccelerationStructure() const { return m_vkAccelerationStructure; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::Buffer GetVulkanBuffer() const { return m_vkBuffer; }
  [[nodiscard]] vk::DeviceAddress            GetVulkanDeviceAddress() const;

  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override final;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::AccelerationStructureKHR                                     m_vkAccelerationStructure = VK_NULL_HANDLE;
  vk::Buffer                                                       m_vkBuffer                = VK_NULL_HANDLE;
  xiiVulkanAllocation                                              m_BufferMemoryAllocation  = {};
  xiiGALTopLevelASBuildDescription                                 m_BuildDescription;
  xiiGALScratchBufferSizeDescription                               m_ScratchBufferSizeDescription;
  xiiHashTable<xiiStringView, xiiGALTopLevelASInstanceDescription> m_NameToInstance;
};
