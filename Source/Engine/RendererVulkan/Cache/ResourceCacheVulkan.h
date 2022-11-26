#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

#include <vulkan/vulkan.hpp>

class xiiGALRasterizerStateVulkan;
class xiiGALBlendStateVulkan;
class xiiGALDepthStencilStateVulkan;
class xiiGALShaderVulkan;
class xiiGALVertexDeclarationVulkan;
class xiiRefCounted;

XII_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class XII_RENDERERVULKAN_DLL xiiResourceCacheVulkan
{
public:
  static void Initialize(xiiGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::RenderPass  RequestRenderPass(const xiiGALRenderingSetup& renderingSetup);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass renderPass, const xiiGALRenderTargetSetup& renderTargetSetup, xiiSizeU32& out_Size, xiiEnum<xiiGALMSAASampleCount>& out_msaa, xiiUInt32& out_uiLayers);

  struct PipelineLayoutDesc
  {
    XII_DECLARE_POD_TYPE();
    vk::DescriptorSetLayout m_layout;
  };

  struct GraphicsPipelineDesc
  {
    XII_DECLARE_POD_TYPE();
    vk::RenderPass                       m_renderPass;
    vk::PipelineLayout                   m_layout;
    xiiEnum<xiiGALPrimitiveTopology>     m_topology;
    xiiEnum<xiiGALMSAASampleCount>       m_msaa;
    xiiUInt8                             m_uiAttachmentCount                                    = 0;
    const xiiGALRasterizerStateVulkan*   m_pCurrentRasterizerState                              = nullptr;
    const xiiGALBlendStateVulkan*        m_pCurrentBlendState                                   = nullptr;
    const xiiGALDepthStencilStateVulkan* m_pCurrentDepthStencilState                            = nullptr;
    const xiiGALShaderVulkan*            m_pCurrentShader                                       = nullptr;
    const xiiGALVertexDeclarationVulkan* m_pCurrentVertexDecl                                   = nullptr;
    xiiUInt32                            m_VertexBufferStrides[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  };

  struct ComputePipelineDesc
  {
    XII_DECLARE_POD_TYPE();
    vk::PipelineLayout        m_layout;
    const xiiGALShaderVulkan* m_pCurrentShader = nullptr;
  };

  static vk::PipelineLayout RequestPipelineLayout(const PipelineLayoutDesc& desc);
  static vk::Pipeline       RequestGraphicsPipeline(const GraphicsPipelineDesc& desc);
  static vk::Pipeline       RequestComputePipeline(const ComputePipelineDesc& desc);

  struct DescriptorSetLayoutDesc
  {
    mutable xiiUInt32                                 m_uiHash = 0;
    xiiHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
  };
  static vk::DescriptorSetLayout RequestDescriptorSetLayout(const xiiGALShaderVulkan::DescriptorSetLayoutDesc& desc);

  /// \brief Invalidates any caches that use this resource. Basically all pointer types in GraphicsPipelineDesc except for xiiGALShaderVulkan.
  static void ResourceDeleted(const xiiRefCounted* pResource);
  /// \brief Invalidates any caches that use this shader resource.
  static void ShaderDeleted(const xiiGALShaderVulkan* pShader);

private:
  struct FramebufferKey
  {
    vk::RenderPass          m_renderPass;
    xiiGALRenderTargetSetup m_renderTargetSetup;
  };

  /// \brief Hashable version without pointers of vk::FramebufferCreateInfo
  struct FramebufferDesc
  {
    VkRenderPass                                                      renderPass;
    xiiSizeU32                                                        m_size = {0, 0};
    uint32_t                                                          layers = 1;
    xiiHybridArray<vk::ImageView, XII_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
    xiiEnum<xiiGALMSAASampleCount>                                    m_msaa;
  };

  /// \brief Hashable version without pointers or redundant data of vk::AttachmentDescription
  struct AttachmentDesc
  {
    XII_DECLARE_POD_TYPE();
    vk::Format              format  = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    // Not set at all right now
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    // Not set at all right now
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    // No support for eDontCare in XII right now
    vk::AttachmentLoadOp  loadOp         = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp        = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp  stencilLoadOp  = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eStore;
  };

  /// \brief Hashable version without pointers of vk::RenderPassCreateInfo
  struct RenderPassDesc
  {
    xiiHybridArray<AttachmentDesc, XII_GAL_MAX_RENDERTARGET_COUNT> attachments;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const RenderPassDesc& renderingSetup);
    static bool      Equal(const RenderPassDesc& a, const RenderPassDesc& b);

    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);

    static xiiUInt32 Hash(const FramebufferKey& renderTargetSetup);
    static bool      Equal(const FramebufferKey& a, const FramebufferKey& b);

    static xiiUInt32 Hash(const PipelineLayoutDesc& desc);
    static bool      Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b);

    static bool      Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);
    static xiiUInt32 Hash(const GraphicsPipelineDesc& desc);
    static bool      Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);

    static bool Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b);
    static bool Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b);

    static xiiUInt32 Hash(const xiiGALShaderVulkan::DescriptorSetLayoutDesc& desc) { return desc.m_uiHash; }
    static bool      Equal(const xiiGALShaderVulkan::DescriptorSetLayoutDesc& a, const xiiGALShaderVulkan::DescriptorSetLayoutDesc& b);
  };

  struct FrameBufferCache
  {
    vk::Framebuffer                m_frameBuffer;
    xiiSizeU32                     m_size;
    xiiEnum<xiiGALMSAASampleCount> m_msaa;
    xiiUInt32                      m_layers = 0;
    XII_DECLARE_POD_TYPE();
  };

  static vk::RenderPass RequestRenderPassInternal(const RenderPassDesc& desc);
  static void           GetRenderPassDesc(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static void           GetFrameBufferDesc(vk::RenderPass renderPass, const xiiGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc);

public:
  using GraphicsPipelineMap = xiiMap<xiiResourceCacheVulkan::GraphicsPipelineDesc, vk::Pipeline, xiiResourceCacheVulkan::ResourceCacheHash>;
  using ComputePipelineMap  = xiiMap<xiiResourceCacheVulkan::ComputePipelineDesc, vk::Pipeline, xiiResourceCacheVulkan::ResourceCacheHash>;


private:
  static xiiGALDeviceVulkan* s_pDevice;
  static vk::Device          s_device;
  // We have a N to 1 mapping for xiiGALRenderingSetup to vk::RenderPass as multiple xiiGALRenderingSetup can share the same RenderPassDesc.
  // Thus, we have a two stage resolve to the vk::RenderPass. If a xiiGALRenderingSetup is not present in s_shallowRenderPasses we create the RenderPassDesc which has a 1 to 1 relationship with vk::RenderPass and look that one up in s_renderPasses. Finally we add the entry to s_shallowRenderPasses to make sure a shallow lookup will work on the next query.
  static xiiHashTable<xiiGALRenderingSetup, vk::RenderPass, ResourceCacheHash> s_shallowRenderPasses; //#TODO_VULKAN cache invalidation
  static xiiHashTable<RenderPassDesc, vk::RenderPass, ResourceCacheHash>       s_renderPasses;
  static xiiHashTable<FramebufferKey, FrameBufferCache, ResourceCacheHash>     s_frameBuffers; //#TODO_VULKAN cache invalidation

  static xiiHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash>        s_pipelineLayouts;
  static GraphicsPipelineMap                                                            s_graphicsPipelines;
  static ComputePipelineMap                                                             s_computePipelines;
  static xiiMap<const xiiRefCounted*, xiiHybridArray<GraphicsPipelineMap::Iterator, 1>> s_graphicsPipelineUsedBy;
  static xiiMap<const xiiRefCounted*, xiiHybridArray<ComputePipelineMap::Iterator, 1>>  s_computePipelineUsedBy;

  static xiiHashTable<xiiGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ResourceCacheHash> s_descriptorSetLayouts;
};
