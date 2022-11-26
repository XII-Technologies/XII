
#pragma once

#include <RendererFoundation/State/State.h>

#include <vulkan/vulkan.hpp>

class XII_RENDERERVULKAN_DLL xiiGALBlendStateVulkan : public xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& Description);

  ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::PipelineColorBlendStateCreateInfo m_blendState              = {};
  vk::PipelineColorBlendAttachmentState m_blendAttachmentState[8] = {};
};

class XII_RENDERERVULKAN_DLL xiiGALDepthStencilStateVulkan : public xiiGALDepthStencilState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& Description);

  ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::PipelineDepthStencilStateCreateInfo m_depthStencilState = {};
};

class XII_RENDERERVULKAN_DLL xiiGALRasterizerStateVulkan : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& Description);

  ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::PipelineRasterizationStateCreateInfo m_rasterizerState = {};
};

class XII_RENDERERVULKAN_DLL xiiGALSamplerStateVulkan : public xiiGALSamplerState
{
public:
  XII_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSamplerStateVulkan(const xiiGALSamplerStateCreationDescription& Description);
  ~xiiGALSamplerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::DescriptorImageInfo m_resourceImageInfo;
};


#include <RendererVulkan/State/Implementation/StateVulkan_inl.h>
