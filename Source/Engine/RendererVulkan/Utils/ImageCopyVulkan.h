#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class xiiGALBufferVulkan;
class xiiGALTextureVulkan;
class xiiGALRenderTargetViewVulkan;
class xiiGALResourceViewVulkan;
class xiiGALUnorderedAccessViewVulkan;


/// \brief
class XII_RENDERERVULKAN_DLL xiiImageCopyVulkan
{
public:
  xiiImageCopyVulkan(xiiGALDeviceVulkan& GALDeviceVulkan);
  ~xiiImageCopyVulkan();
  void Init(const xiiGALTextureVulkan* pSource, const xiiGALTextureVulkan* pTarget, xiiShaderUtils::xiiBuiltinShaderType type);

  void Copy(const xiiVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const xiiVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const xiiVec3U32& extends);

  static void Initialize(xiiGALDeviceVulkan& GALDeviceVulkan);
  static void DeInitialize(xiiGALDeviceVulkan& GALDeviceVulkan);

  struct RenderPassCacheKey
  {
    XII_DECLARE_POD_TYPE();

    vk::Format              targetFormat;
    vk::SampleCountFlagBits targetSamples;
  };

  struct FramebufferCacheKey
  {
    XII_DECLARE_POD_TYPE();

    vk::RenderPass m_renderpass;
    vk::ImageView  m_targetView;
    xiiVec3U32     m_extends;
    uint32_t       m_layerCount;
  };

  struct ImageViewCacheKey
  {
    XII_DECLARE_POD_TYPE();

    vk::Image                  m_image;
    vk::ImageSubresourceLayers m_subresourceLayers;
  };

  struct ImageViewCacheValue
  {
    XII_DECLARE_POD_TYPE();

    vk::ImageSubresourceLayers m_subresourceLayers;
    vk::ImageView              m_imageView;
  };

private:
  void RenderInternal(const xiiVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const xiiVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const xiiVec3U32& extends);

  static void OnBeforeImageDestroyed(xiiGALDeviceVulkan::OnBeforeImageDestroyedData data);


private:
  xiiGALDeviceVulkan& m_GALDeviceVulkan;

  // Init input
  const xiiGALTextureVulkan*           m_pSource = nullptr;
  const xiiGALTextureVulkan*           m_pTarget = nullptr;
  xiiShaderUtils::xiiBuiltinShaderType m_type    = xiiShaderUtils::xiiBuiltinShaderType::CopyImage;

  // Init derived Vulkan objects
  vk::RenderPass                               m_renderPass;
  xiiShaderUtils::xiiBuiltinShader             m_shader;
  xiiGALVertexDeclarationHandle                m_hVertexDecl;
  xiiResourceCacheVulkan::PipelineLayoutDesc   m_LayoutDesc;
  xiiResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Pipeline                                 m_pipeline;

  // Cache to keep important resources alive
  // This avoids recreating them every frame
  struct Cache
  {
    Cache(xiiAllocatorBase* pAllocator);
    ~Cache();

    xiiHashTable<xiiGALShaderHandle, xiiGALVertexDeclarationHandle>                      m_vertexDeclarations;
    xiiHashTable<RenderPassCacheKey, vk::RenderPass>                                     m_renderPasses;
    xiiHashTable<ImageViewCacheKey, vk::ImageView>                                       m_sourceImageViews;
    xiiHashTable<vk::Image, ImageViewCacheValue>                                         m_imageToSourceImageViewCacheKey;
    xiiHashTable<ImageViewCacheKey, vk::ImageView>                                       m_targetImageViews;
    xiiHashTable<vk::Image, ImageViewCacheValue>                                         m_imageToTargetImageViewCacheKey;
    xiiHashTable<FramebufferCacheKey, vk::Framebuffer>                                   m_framebuffers;
    xiiHashTable<xiiShaderUtils::xiiBuiltinShaderType, xiiShaderUtils::xiiBuiltinShader> m_shaders;

    xiiEventSubscriptionID m_onBeforeImageDeletedSubscription;
  };

  static xiiUniquePtr<Cache> s_cache;
};
