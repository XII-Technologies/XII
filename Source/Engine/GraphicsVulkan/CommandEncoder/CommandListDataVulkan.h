/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <GraphicsVulkan/Pools/DynamicBufferPoolVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>

namespace vk
{
  class CommandBuffer;
  class RenderPass;
  class Framebuffer;
  class Pipeline;
  class Buffer;
  class Image;
  class DescriptorSet;
  class QueryPool;
  struct DescriptorBufferInfo;
} // namespace vk

struct XII_GRAPHICSVULKAN_DLL xiiGALCommandListDataVulkan
{
  struct ResourceSetBindings
  {
    XII_ALWAYS_INLINE ResourceSetBindings()  = default;
    XII_ALWAYS_INLINE ~ResourceSetBindings() = default;
    XII_ALWAYS_INLINE ResourceSetBindings(ResourceSetBindings&& other) noexcept
    {
      m_pBoundConstantBuffers                     = std::move(other.m_pBoundConstantBuffers);
      m_pBoundBufferResourceViews                 = std::move(other.m_pBoundBufferResourceViews);
      m_pBoundTextureResourceViews                = std::move(other.m_pBoundTextureResourceViews);
      m_pBoundAccelerationStructures              = std::move(other.m_pBoundAccelerationStructures);
      m_pBoundUnorderedAccessBufferResourceViews  = std::move(other.m_pBoundUnorderedAccessBufferResourceViews);
      m_pBoundUnorderedAccessTextureResourceViews = std::move(other.m_pBoundUnorderedAccessTextureResourceViews);
      m_pBoundSamplerStates                       = std::move(other.m_pBoundSamplerStates);
      m_pBoundBufferResourceViewArrays                 = std::move(other.m_pBoundBufferResourceViewArrays);
      m_pBoundTextureResourceViewArrays                = std::move(other.m_pBoundTextureResourceViewArrays);
      m_pBoundUnorderedAccessBufferResourceViewArrays  = std::move(other.m_pBoundUnorderedAccessBufferResourceViewArrays);
      m_pBoundUnorderedAccessTextureResourceViewArrays = std::move(other.m_pBoundUnorderedAccessTextureResourceViewArrays);
      m_pBoundSamplerStateArrays                       = std::move(other.m_pBoundSamplerStateArrays);
    }

    xiiDynamicArray<xiiGALBufferVulkan*>      m_pBoundConstantBuffers;
    xiiDynamicArray<xiiGALBufferViewVulkan*>  m_pBoundBufferResourceViews;
    xiiDynamicArray<xiiGALTextureViewVulkan*> m_pBoundTextureResourceViews;
    xiiDynamicArray<xiiGALTopLevelASVulkan*>  m_pBoundAccelerationStructures;
    xiiDynamicArray<xiiGALBufferViewVulkan*>  m_pBoundUnorderedAccessBufferResourceViews;
    xiiDynamicArray<xiiGALTextureViewVulkan*> m_pBoundUnorderedAccessTextureResourceViews;
    xiiDynamicArray<xiiGALSamplerVulkan*>     m_pBoundSamplerStates;
    xiiDynamicArray<xiiDynamicArray<xiiGALBufferViewVulkan*>>  m_pBoundBufferResourceViewArrays;
    xiiDynamicArray<xiiDynamicArray<xiiGALTextureViewVulkan*>> m_pBoundTextureResourceViewArrays;
    xiiDynamicArray<xiiDynamicArray<xiiGALBufferViewVulkan*>>  m_pBoundUnorderedAccessBufferResourceViewArrays;
    xiiDynamicArray<xiiDynamicArray<xiiGALTextureViewVulkan*>> m_pBoundUnorderedAccessTextureResourceViewArrays;
    xiiDynamicArray<xiiDynamicArray<xiiGALSamplerVulkan*>>     m_pBoundSamplerStateArrays;
  };

  XII_ALWAYS_INLINE xiiGALCommandListDataVulkan()  = default;
  XII_ALWAYS_INLINE ~xiiGALCommandListDataVulkan() = default;
  XII_ALWAYS_INLINE xiiGALCommandListDataVulkan(xiiGALCommandListDataVulkan&& other) noexcept
  {
    m_pBoundRenderTargets         = std::move(other.m_pBoundRenderTargets);
    m_pBoundDepthStencilTarget    = std::move(other.m_pBoundDepthStencilTarget);
    m_uiBoundRenderTargetCount    = other.m_uiBoundRenderTargetCount;
    m_uiSubpassIndex              = other.m_uiSubpassIndex;
    m_AttachmentClearValues       = std::move(other.m_AttachmentClearValues);
    m_bPipelineStateModified      = other.m_bPipelineStateModified;
    m_ResourceSets                = std::move(other.m_ResourceSets);
    m_DescriptorSets              = std::move(other.m_DescriptorSets);
    m_DynamicUniformBuffers       = std::move(other.m_DynamicUniformBuffers);
    m_DynamicUniformBufferOffsets = std::move(other.m_DynamicUniformBufferOffsets);
    m_bDescriptorsModified        = other.m_bDescriptorsModified;
    m_pDynamicBufferPoolVulkan    = std::move(other.m_pDynamicBufferPoolVulkan);
    m_pUploadStagingBufferPool    = std::move(other.m_pUploadStagingBufferPool);
    m_pDescriptorSetPoolVulkan    = std::move(other.m_pDescriptorSetPoolVulkan);
    m_pNullVertexBuffer           = std::move(other.m_pNullVertexBuffer);
    m_TemporaryQueryPools         = std::move(other.m_TemporaryQueryPools);
    m_uiActiveQueriesCounter      = other.m_uiActiveQueriesCounter;
  }

  XII_ALWAYS_INLINE xiiGALCommandListDataVulkan& operator=(xiiGALCommandListDataVulkan&& other) noexcept
  {
    m_pBoundRenderTargets         = std::move(other.m_pBoundRenderTargets);
    m_pBoundDepthStencilTarget    = std::move(other.m_pBoundDepthStencilTarget);
    m_uiBoundRenderTargetCount    = other.m_uiBoundRenderTargetCount;
    m_uiSubpassIndex              = other.m_uiSubpassIndex;
    m_AttachmentClearValues       = std::move(other.m_AttachmentClearValues);
    m_bPipelineStateModified      = other.m_bPipelineStateModified;
    m_ResourceSets                = std::move(other.m_ResourceSets);
    m_DescriptorSets              = std::move(other.m_DescriptorSets);
    m_DynamicUniformBuffers       = std::move(other.m_DynamicUniformBuffers);
    m_DynamicUniformBufferOffsets = std::move(other.m_DynamicUniformBufferOffsets);
    m_bDescriptorsModified        = other.m_bDescriptorsModified;
    m_pDynamicBufferPoolVulkan    = std::move(other.m_pDynamicBufferPoolVulkan);
    m_pUploadStagingBufferPool    = std::move(other.m_pUploadStagingBufferPool);
    m_pDescriptorSetPoolVulkan    = std::move(other.m_pDescriptorSetPoolVulkan);
    m_pNullVertexBuffer           = std::move(other.m_pNullVertexBuffer);
    m_TemporaryQueryPools         = std::move(other.m_TemporaryQueryPools);
    m_uiActiveQueriesCounter      = other.m_uiActiveQueriesCounter;

    return *this;
  }

  XII_ALWAYS_INLINE void Invalidate()
  {
    for (ResourceSetBindings& setBindings : m_ResourceSets)
    {
      setBindings.m_pBoundConstantBuffers.Clear();
      setBindings.m_pBoundBufferResourceViews.Clear();
      setBindings.m_pBoundTextureResourceViews.Clear();
      setBindings.m_pBoundAccelerationStructures.Clear();
      setBindings.m_pBoundUnorderedAccessBufferResourceViews.Clear();
      setBindings.m_pBoundUnorderedAccessTextureResourceViews.Clear();
      setBindings.m_pBoundSamplerStates.Clear();
      setBindings.m_pBoundBufferResourceViewArrays.Clear();
      setBindings.m_pBoundTextureResourceViewArrays.Clear();
      setBindings.m_pBoundUnorderedAccessBufferResourceViewArrays.Clear();
      setBindings.m_pBoundUnorderedAccessTextureResourceViewArrays.Clear();
      setBindings.m_pBoundSamplerStateArrays.Clear();
    }

    m_DescriptorSets.Clear();
    m_DynamicUniformBuffers.Clear();
    m_DynamicUniformBufferOffsets.Clear();
    m_bDescriptorsModified   = false;
    m_bPipelineStateModified = true;

    m_pBoundRenderTargets.Clear();
    m_pBoundDepthStencilTarget.Clear();
    m_uiBoundRenderTargetCount = 0U;

    m_uiSubpassIndex = 0U;
    m_AttachmentClearValues.Clear();
  }

  XII_ALWAYS_INLINE void Reset()
  {
    Invalidate();

    if (m_pDynamicBufferPoolVulkan)
    {
      m_pDynamicBufferPoolVulkan->Reset();
    }
    if (m_pUploadStagingBufferPool)
    {
      m_pUploadStagingBufferPool->Reset();
    }
  }

  xiiHybridArray<xiiSharedPtr<xiiGALTextureViewVulkan>, 2U> m_pBoundRenderTargets;
  xiiSharedPtr<xiiGALTextureViewVulkan>                     m_pBoundDepthStencilTarget;
  xiiUInt32                                                 m_uiBoundRenderTargetCount = 0U;

  xiiUInt32                          m_uiSubpassIndex = 0U;
  xiiHybridArray<vk::ClearValue, 2U> m_AttachmentClearValues;

  bool m_bPipelineStateModified = false;

  xiiHybridArray<ResourceSetBindings, 1U> m_ResourceSets;
  xiiHybridArray<vk::DescriptorSet, 4U>   m_DescriptorSets;
  xiiDeque<vk::DescriptorBufferInfo>      m_DynamicUniformBuffers;
  xiiHybridArray<xiiUInt32, 6U>           m_DynamicUniformBufferOffsets;
  bool                                    m_bDescriptorsModified = false;

  xiiUniquePtr<xiiGALDynamicBufferPoolVulkan> m_pDynamicBufferPoolVulkan;
  xiiUniquePtr<xiiGALStagingBufferPoolVulkan> m_pUploadStagingBufferPool;
  xiiUniquePtr<xiiGALDescriptorSetPoolVulkan> m_pDescriptorSetPoolVulkan;

  xiiSharedPtr<xiiGALBufferVulkan> m_pNullVertexBuffer; ///< In Vulkan, we cannot bind a null vertex buffer, so we have to create a zeroed-out vertex buffer.

  xiiDynamicArray<vk::QueryPool> m_TemporaryQueryPools; ///< Query pools created while recording and destroyed after GPU execution completes.

  xiiUInt32 m_uiActiveQueriesCounter = 0U;
};
