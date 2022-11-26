
#pragma once

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

using xiiGALFormatLookupEntryVulkan = xiiGALFormatLookupEntry<vk::Format, (vk::Format)0>;
using xiiGALFormatLookupTableVulkan = xiiGALFormatLookupTable<xiiGALFormatLookupEntryVulkan>;

class xiiGALBufferVulkan;
class xiiGALTextureVulkan;
class xiiGALPassVulkan;
class xiiPipelineBarrierVulkan;
class xiiCommandBufferPoolVulkan;
class xiiStagingBufferPoolVulkan;
class xiiQueryPoolVulkan;
class xiiInitContextVulkan;

/// \brief The Vulkan device implementation of the graphics abstraction layer.
class XII_RENDERERVULKAN_DLL xiiGALDeviceVulkan : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& Description);

public:
  virtual ~xiiGALDeviceVulkan();

public:
  struct PendingDeletion
  {
    XII_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void*          m_pObject;
    union
    {
      xiiVulkanAllocation m_allocation;
      void*               m_pContext;
    };
  };

  struct ReclaimResource
  {
    XII_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void*          m_pObject;
    void*          m_pContext = nullptr;
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif XII_ENABLED(XII_SUPPORTS_GLFW)
#else
#  error "Vulkan Platform not supported"
#endif

    bool                                m_bDebugUtils                       = false;
    PFN_vkCreateDebugUtilsMessengerEXT  pfn_vkCreateDebugUtilsMessengerEXT  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT    pfn_vkSetDebugUtilsObjectNameEXT    = nullptr;

    bool m_bDeviceSwapChain          = false;
    bool m_bShaderViewportIndexLayer = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool                                           m_bBorderColorFloat = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    xiiUInt32 m_uiQueueFamily = -1;
    xiiUInt32 m_uiQueueIndex  = 0;
  };

  xiiUInt64 GetCurrentFrame() const { return m_uiFrameCounter; }
  xiiUInt64 GetSafeFrame() const { return m_uiSafeFrame; }

  vk::Instance GetVulkanInstance() const;
  vk::Device   GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice                  GetVulkanPhysicalDevice() const;
  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_properties; }
  const Extensions&                   GetExtensions() const { return m_extensions; }
  vk::PipelineStageFlags              GetSupportedStages() const;

  vk::CommandBuffer&          GetCurrentCommandBuffer();
  xiiPipelineBarrierVulkan&   GetCurrentPipelineBarrier();
  xiiQueryPoolVulkan&         GetQueryPool() const;
  xiiStagingBufferPoolVulkan& GetStagingBufferPool() const;
  xiiInitContextVulkan&       GetInitContext() const;
  xiiProxyAllocator&          GetAllocator();

  xiiGALTextureHandle CreateTextureInternal(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData, vk::Format OverrideFormat, bool bLinearCPU = false);
  xiiGALBufferHandle  CreateBufferInternal(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData, bool bCPU = false);

  const xiiGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  xiiInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore);

  void DeleteLater(const PendingDeletion& deletion);

  template <typename T>
  void DeleteLater(T& object, xiiVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, allocation});
    }
    object     = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, void* pContext)
  {
    if (object)
    {
      PendingDeletion del = {object.objectType, (void*)object, nullptr};
      del.m_pContext      = pContext;
      DeleteLater(static_cast<const PendingDeletion&>(del));
    }
    object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, nullptr});
    }
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object, void* pContext = nullptr)
  {
    ReclaimLater({object.objectType, (void*)object, pContext});
    object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, xiiVulkanAllocation allocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& object, xiiVulkanAllocation allocation = nullptr)
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType   = object.objectType;
      nameInfo.objectHandle = (uint64_t) static_cast<typename T::NativeType>(object);
      nameInfo.pObjectName  = szName;

      SetDebugName(nameInfo, allocation);
    }
#endif
  }

  void ReportLiveGpuObjects();

  static void UploadBufferStaging(xiiStagingBufferPoolVulkan* pStagingBufferPool, xiiPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const xiiGALBufferVulkan* pBuffer, xiiArrayPtr<const xiiUInt8> pInitialData, vk::DeviceSize dstOffset = 0);
  static void UploadTextureStaging(xiiStagingBufferPoolVulkan* pStagingBufferPool, xiiPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const xiiGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const xiiGALSystemMemoryDescription& data);

  struct OnBeforeImageDestroyedData
  {
    vk::Image           image;
    xiiGALDeviceVulkan& GALDeviceVulkan;
  };
  xiiEvent<OnBeforeImageDestroyedData> OnBeforeImageDestroyed;


  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(xiiHybridArray<const char*, 6>& extensions);
  vk::Result SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, xiiHybridArray<const char*, 6>& extensions);

  virtual xiiResult InitPlatform() override;
  virtual xiiResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain) override;

  virtual xiiGALPass* BeginPassPlatform(const char* szName) override;
  virtual void        EndPassPlatform(xiiGALPass* pPass) override;


  // State creation functions

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description) override;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState) override;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description) override;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState) override;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description) override;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override;

  virtual xiiGALSamplerState* CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description) override;
  virtual void                DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& Description) override;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader) override;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer) override;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture) override;

  virtual xiiGALResourceView* CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) override;
  virtual void                DestroyResourceViewPlatform(xiiGALResourceView* pResourceView) override;

  virtual xiiGALRenderTargetView* CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) override;
  virtual void                    DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView) override;

  xiiGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void               DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pResource) override;

  // Other rendering creation functions

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& Description) override;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery) override;

  virtual xiiGALVertexDeclaration* CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description) override;
  virtual void                     DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual xiiGALTimestampHandle GetTimestampPlatform() override;
  virtual xiiResult             GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result) override;

  // Misc functions

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  struct PerFrameData
  {
    /// \brief These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    xiiHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    //ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double    m_fInvTicksPerSecond = -1.0;
    xiiUInt64 m_uiFrame            = -1;

    xiiMutex                  m_pendingDeletionsMutex;
    xiiDeque<PendingDeletion> m_pendingDeletions;
    xiiDeque<PendingDeletion> m_pendingDeletionsPrevious;

    xiiMutex                  m_reclaimResourcesMutex;
    xiiDeque<ReclaimResource> m_reclaimResources;
    xiiDeque<ReclaimResource> m_reclaimResourcesPrevious;
  };

  void DeletePendingResources(xiiDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(xiiDeque<ReclaimResource>& resources);

  void FillFormatLookupTable();

  xiiUInt64 m_uiFrameCounter        = 1; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  xiiUInt64 m_uiSafeFrame           = 0;
  xiiUInt8  m_uiCurrentPerFrameData = 0;
  xiiUInt8  m_uiNextPerFrameData    = 1;

  vk::Instance                 m_instance;
  vk::PhysicalDevice           m_physicalDevice;
  vk::PhysicalDeviceProperties m_properties;
  vk::Device                   m_device;
  Queue                        m_graphicsQueue;
  Queue                        m_transferQueue;

  xiiGALFormatLookupTableVulkan      m_FormatLookupTable;
  vk::PipelineStageFlags             m_supportedStages;
  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  xiiUniquePtr<xiiGALPassVulkan>           m_pDefaultPass;
  xiiUniquePtr<xiiPipelineBarrierVulkan>   m_pPipelineBarrier;
  xiiUniquePtr<xiiCommandBufferPoolVulkan> m_pCommandBufferPool;
  xiiUniquePtr<xiiStagingBufferPoolVulkan> m_pStagingBufferPool;
  xiiUniquePtr<xiiQueryPoolVulkan>         m_pQueryPool;
  xiiUniquePtr<xiiInitContextVulkan>       m_pInitContext;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[4];

#if XII_ENABLED(XII_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;
#endif

  Extensions m_extensions;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
