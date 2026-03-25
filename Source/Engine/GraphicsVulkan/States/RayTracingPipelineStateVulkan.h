#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace vk
{
  class Pipeline;
}

class XII_GRAPHICSVULKAN_DLL xiiGALRayTracingPipelineStateVulkan final : public xiiGALRayTracingPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRayTracingPipelineStateVulkan, xiiGALRayTracingPipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::Pipeline GetVulkanPipeline() const { return m_vkPipeline; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineCache GetVulkanPipelineCache() const { return m_vkPipelineCache; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineLayout GetVulkanPipelineLayout() const { return m_vkPipelineLayout; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineBindPoint GetVulkanPipelineBindPoint() const { return m_vkPipelineBindPoint; }

  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt8> GetShaderGroupHandles() const { return m_ShaderGroupHandles; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetRayGenerationGroupIndices() const { return m_RayGenerationGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetMissGroupIndices() const { return m_MissGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetHitGroupIndices() const { return m_HitGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetCallableGroupIndices() const { return m_CallableGroupIndices; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRayTracingPipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALRayTracingPipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Pipeline          m_vkPipeline;
  vk::PipelineCache     m_vkPipelineCache;
  vk::PipelineLayout    m_vkPipelineLayout;
  vk::PipelineBindPoint m_vkPipelineBindPoint;

  xiiDynamicArray<xiiUInt8>  m_ShaderGroupHandles;
  xiiDynamicArray<xiiUInt32> m_RayGenerationGroupIndices;
  xiiDynamicArray<xiiUInt32> m_MissGroupIndices;
  xiiDynamicArray<xiiUInt32> m_HitGroupIndices;
  xiiDynamicArray<xiiUInt32> m_CallableGroupIndices;
};
