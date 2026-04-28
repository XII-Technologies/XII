/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBottomLevelASVulkan final : public xiiGALBottomLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBottomLevelASVulkan, xiiGALBottomLevelAS);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::AccelerationStructureKHR GetVulkanAccelerationStructure() const { return m_vkAccelerationStructure; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::Buffer GetVulkanBuffer() const { return m_vkBuffer; }
  [[nodiscard]] vk::DeviceAddress            GetVulkanDeviceAddress() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::AccelerationStructureKHR m_vkAccelerationStructure = VK_NULL_HANDLE;
  vk::Buffer                   m_vkBuffer                = VK_NULL_HANDLE;
  xiiVulkanAllocation          m_BufferMemoryAllocation  = {};
};
