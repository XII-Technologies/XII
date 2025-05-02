#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/Device.h>

#include <type_traits>

class XII_GRAPHICSVULKAN_DLL xiiGALDeviceVulkan final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceVulkan, xiiGALDevice);

private:
  friend class xiiMemoryUtils;

  friend xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceVulkan(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  virtual ~xiiGALDeviceVulkan();

public:
  enum class DebugMode
  {
    Disabled,
    Utils,
    Report,
  };

  struct ExtensionFeatures
  {
    vk::PhysicalDeviceMeshShaderFeaturesEXT             m_MeshShader;
    vk::PhysicalDevice16BitStorageFeaturesKHR           m_Storage16Bit;
    vk::PhysicalDevice8BitStorageFeaturesKHR            m_Storage8Bit;
    vk::PhysicalDeviceShaderFloat16Int8FeaturesKHR      m_ShaderFloat16Int8;
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR  m_AccelerationStructure;
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR     m_RayTracingPipeline;
    vk::PhysicalDeviceRayQueryFeaturesKHR               m_RayQuery;
    vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR    m_BufferDeviceAddress;
    vk::PhysicalDeviceDescriptorIndexingFeaturesEXT     m_DescriptorIndexing;
    vk::PhysicalDevicePortabilitySubsetFeaturesKHR      m_PortabilitySubset;
    vk::PhysicalDeviceVertexAttributeDivisorFeaturesEXT m_VertexAttributeDivisor;
    vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR      m_TimelineSemaphore;
    vk::PhysicalDeviceHostQueryResetFeatures            m_HostQueryReset;
    vk::PhysicalDeviceFragmentShadingRateFeaturesKHR    m_ShadingRate;
    vk::PhysicalDeviceFragmentDensityMapFeaturesEXT     m_FragmentDensityMap;  // Only for desktop devices
    vk::PhysicalDeviceFragmentDensityMap2FeaturesEXT    m_FragmentDensityMap2; // Only for mobile devices
    vk::PhysicalDeviceMultiviewFeaturesKHR              m_Multiview;           // Required for RenderPass2
    vk::PhysicalDeviceMultiDrawFeaturesEXT              m_MultiDraw;
    vk::PhysicalDeviceShaderDrawParametersFeatures      m_ShaderDrawParameters;
    vk::PhysicalDeviceCustomBorderColorFeaturesEXT      m_CustomBorderColor;

    bool m_bSpirv14                  = false; // Ray tracing requires Vulkan 1.2 or SPIRV 1.4 extension
    bool m_bSpirv15                  = false; // DXC shaders with ray tracing requires Vulkan 1.2 with SPIRV 1.5
    bool m_bSubgroupOps              = false; // Requires Vulkan 1.1
    bool m_bHasPortabilitySubset     = false;
    bool m_bRenderPass2              = false;
    bool m_bDrawIndirectCount        = false;
    bool m_bShaderViewportIndexLayer = false;
  };

  struct ExtensionProperties
  {
    vk::PhysicalDeviceMeshShaderPropertiesEXT             m_MeshShader;
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR  m_AccelerationStructure;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR     m_RayTracingPipeline;
    vk::PhysicalDeviceDescriptorIndexingPropertiesEXT     m_DescriptorIndexing;
    vk::PhysicalDevicePortabilitySubsetPropertiesKHR      m_PortabilitySubset;
    vk::PhysicalDeviceSubgroupProperties                  m_Subgroup;
    vk::PhysicalDeviceVertexAttributeDivisorPropertiesEXT m_VertexAttributeDivisor;
    vk::PhysicalDeviceTimelineSemaphorePropertiesKHR      m_TimelineSemaphore;
    vk::PhysicalDeviceFragmentShadingRatePropertiesKHR    m_ShadingRate;
    vk::PhysicalDeviceFragmentDensityMapPropertiesEXT     m_FragmentDensityMap;
    vk::PhysicalDeviceMultiviewPropertiesKHR              m_Multiview;
    vk::PhysicalDeviceMaintenance3Properties              m_Maintenance3;
    vk::PhysicalDeviceFragmentDensityMap2PropertiesEXT    m_FragmentDensityMap2;
    vk::PhysicalDeviceMultiDrawPropertiesEXT              m_MultiDraw;
    vk::PhysicalDeviceCustomBorderColorPropertiesEXT      m_CustomBorderColor;
  };

public:
  virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  template <typename ObjectHandle, typename = typename std::enable_if<std::is_object<ObjectHandle>::value>::type>
  void SetVulkanObjectDebugName(ObjectHandle& vkObject, const char* szDebugName, VmaAllocation vmaAllocation = {})
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (m_DebugMode != DebugMode::Disabled)
    {
      if (vkObject == VK_NULL_HANDLE)
        return;

      vk::DebugUtilsObjectNameInfoEXT vkDebugObjectNameInfo = {};
      vkDebugObjectNameInfo.pNext                           = nullptr;
      vkDebugObjectNameInfo.objectType                      = vkObject.objectType;
      vkDebugObjectNameInfo.objectHandle                    = (uint64_t) static_cast<typename ObjectHandle::NativeType>(vkObject);
      vkDebugObjectNameInfo.pObjectName                     = szDebugName;

      m_LogicalDevice.setDebugUtilsObjectNameEXT(vkDebugObjectNameInfo, m_InstanceDispatchLoader);

      if (vmaAllocation != nullptr)
      {
        vmaSetAllocationUserData(m_vkVmaAllocator, vmaAllocation, (void*)vkDebugObjectNameInfo.pObjectName);
      }
    }
#endif
  }

  template <typename T, typename = void>
  struct HasObjectType : std::false_type
  {};

  template <typename T>
  struct HasObjectType<T, std::void_t<decltype(T::objectType)>> : std::true_type
  {};

  template <typename T, typename = std::enable_if_t<std::is_class_v<T> && HasObjectType<T>::value>>
  void SafeReleaseDeviceObject(T&& vkObject, VmaAllocation&& vmaAllocation = nullptr)
  {
    if (vkObject == VK_NULL_HANDLE)
      return;

    SafeReleaseDeviceObjectInternal(vkObject.objectType, static_cast<void*>(vkObject), vmaAllocation);
  }

  template <typename T>
  void ReclaimLater(T&& vkObject)
  {
    ReclaimLaterInternal(vkObject.objectType, (void*)vkObject);
  }

  void ReclaimCommandBufferLater(xiiGALCommandBufferPoolVulkan* pCommandBufferPool, vk::CommandBuffer&& vkCommandBuffer);

  // Internal objects retrieval.

  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocatorBase* GetAllocator() const { return m_Allocator.GetParent(); }
  [[nodiscard]] XII_ALWAYS_INLINE VmaAllocator      GetVulkanMemoryAllocator() const { return m_vkVmaAllocator; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::Instance GetVulkanInstance() const { return m_Instance; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32    GetVulkanVersion() const { return m_uiVulkanVersion; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::detail::DispatchLoaderDynamic& GetVulkanDynamicDispatchLoader() const { return m_InstanceDispatchLoader; }

  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::LayerProperties> GetVulkanInstanceLayers() const { return m_Layers; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::ExtensionProperties> GetVulkanInstanceExtensionProperties() const { return m_Extensions; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const char* const> GetVulkanInstanceEnabledExtensions() const { return m_EnabledExtensions; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::PhysicalDevice> GetVulkanPhysicalDevices() const { return m_PhysicalDevices; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceProperties& GetVulkanPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& GetVulkanPhysicalDeviceFeatures() const { return m_PhysicalDeviceFeatures; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceMemoryProperties& GetVulkanPhysicalDeviceMemoryProperties() const { return m_PhysicalDeviceMemoryProperties; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::QueueFamilyProperties> GetPhysicalDeviceQueueFamilyProperties() const { return m_PhysicalDeviceQueueFamilyProperties; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::ExtensionProperties> GetPhysicalDeviceSupportedExtensions() const { return m_PhysicalDeviceSupportedExtensions; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& GetPhysicalDeviceExtensionFeatures() const { return m_PhysicalDeviceExtensionFeatures; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionProperties& GetPhysicalDeviceExtensionProperties() const { return m_PhysicalDeviceExtensionProperties; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::Device GetVulkanLogicalDevice() const { return m_LogicalDevice; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& GetVulkanLogicalDeviceFeatures() const { return m_LogicalDeviceFeatures; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& GetVulkanLogicalDeviceExtensionFeatures() const { return m_LogicalDeviceExtensionFeatures; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::PipelineStageFlags> GetVulkanLogicalDeviceSupportedStagesFlags() const { return m_LogicalDeviceSupportedStagesFlags; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineStageFlags GetVulkanLogicalDeviceSupportedStagesFlags(xiiUInt32 uiQueueFamilyIndex) const { return m_LogicalDeviceSupportedStagesFlags[uiQueueFamilyIndex]; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::AccessFlags> GetVulkanLogicalDeviceSupportedAccessFlags() const { return m_LogicalDeviceSupportedAccessFlags; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::AccessFlags GetVulkanLogicalDeviceSupportedAccessFlags(xiiUInt32 uiQueueFamilyIndex) const { return m_LogicalDeviceSupportedAccessFlags[uiQueueFamilyIndex]; }

  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetGraphicsQueueInformation() const { return m_GraphicsQueueInformation; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetComputeQueueInformation() const { return m_ComputeQueueInformation; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetTransferQueueInformation() const { return m_TransferQueueInformation; }

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDeviceVulkan::DebugMode GetDebugMode() const { return m_DebugMode; }

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALFencePoolVulkan* GetVulkanFencePool() const { return m_pFencePool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALSemaphorePoolVulkan* GetVulkanSemaphorePool() const { return m_pSemaphorePool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDescriptorSetPoolVulkan* GetVulkanDescriptorSetPool() const { return m_pDescriptorSetPool.Borrow(); }

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanGraphicsCommandQueueQueryPool() const { return m_pGraphicsCommandQueueQueryPool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanComputeCommandQueueQueryPool() const { return m_pComputeCommandQueueQueryPool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanTransferCommandQueueQueryPool() const { return m_pTransferCommandQueueQueryPool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetQueryPoolForCommandQueue(xiiGALCommandQueueVulkan* pCommandQueueVulkan) const
  {
    if (m_pGraphicsCommandQueue == pCommandQueueVulkan)
      return m_pGraphicsCommandQueueQueryPool.Borrow();

    if (m_pComputeCommandQueue == pCommandQueueVulkan)
      return m_pComputeCommandQueueQueryPool ? m_pComputeCommandQueueQueryPool.Borrow() : nullptr;

    if (m_pTransferCommandQueue == pCommandQueueVulkan)
      return m_pTransferCommandQueueQueryPool ? m_pTransferCommandQueueQueryPool.Borrow() : nullptr;

    return nullptr;
  }

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains) override final;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr) override final;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) override final;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineState>             CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description) override final;

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
  class DeferredDeletionQueue
  {
  public:
    DeferredDeletionQueue(xiiGALDeviceVulkan* pDeviceVulkan);

    void EnqueueResource(vk::Fence vkFence, vk::ObjectType vkObjectType, void* pObject);
    void EnqueueResource(vk::Fence vkFence, vk::ObjectType vkObjectType, void* pObject, VmaAllocation vmaAllocation);

    void EnqueueResource(vk::Fence vkFence, xiiGALCommandBufferPoolVulkan* pCommandBufferPool, vk::CommandBuffer vkCommandBuffer);
    void EnqueueResource(vk::Fence vkFence, xiiGALSemaphorePoolVulkan* pSemaphorePool, vk::Semaphore vkSemaphore);
    void EnqueueResource(vk::Fence vkFence, xiiGALDescriptorSetPoolVulkan* pDescriptorSetPool, vk::DescriptorPool vkDescriptorPool);
    void EnqueueResource(vk::Fence vkFence, xiiGALFencePoolVulkan* pFencePool, vk::Fence vkReclaimFence);

    void ReleaseResources(bool bForceReleaseAll = false);

    XII_ALWAYS_INLINE bool IsEmpty() const { return m_DeletionQueue.IsEmpty(); }

  private:
    struct DeletionEntry
    {
      vk::Fence      m_vkFence       = VK_NULL_HANDLE;
      vk::ObjectType m_vkObjectType  = vk::ObjectType::eUnknown;
      void*          m_pObject       = VK_NULL_HANDLE;
      VmaAllocation  m_VmaAllocation = VK_NULL_HANDLE;

      xiiGALCommandBufferPoolVulkan* m_pCommandBufferPool = nullptr;
      vk::CommandBuffer              m_vkCommandBuffer    = VK_NULL_HANDLE;

      xiiGALSemaphorePoolVulkan* m_pSemaphorePool = nullptr;
      vk::Semaphore              m_vkSemaphore    = VK_NULL_HANDLE;

      xiiGALFencePoolVulkan* m_pFencePool     = nullptr;
      vk::Fence              m_vkReclaimFence = VK_NULL_HANDLE;

      xiiGALDescriptorSetPoolVulkan* m_pDescriptorSetPool = nullptr;
      vk::DescriptorPool             m_vkDescriptorPool   = VK_NULL_HANDLE;

      XII_ALWAYS_INLINE constexpr bool operator==(const DeletionEntry& rhs) const
      {
        return m_vkObjectType == rhs.m_vkObjectType && m_pObject == rhs.m_pObject && m_VmaAllocation == rhs.m_VmaAllocation &&
          m_vkCommandBuffer == rhs.m_vkCommandBuffer && m_vkFence == rhs.m_vkFence && m_vkSemaphore == rhs.m_vkSemaphore &&
          m_vkReclaimFence == rhs.m_vkReclaimFence && m_vkDescriptorPool == rhs.m_vkDescriptorPool;
      }
    };

    void DestroyObject(vk::Device vkLogicalDevice, vk::ObjectType vkObjectType, void* pObject);
    void DestroyObject(vk::ObjectType vkObjectType, void* pObject, VmaAllocation vmaAllocation);

    void DestroyCommandBuffer(xiiGALCommandBufferPoolVulkan* pCommandBufferPool, vk::CommandBuffer&& vkCommandBuffer);
    void DestroySemaphore(xiiGALSemaphorePoolVulkan* pSemaphorePool, vk::Semaphore&& vkSemaphore);
    void DestroyFence(xiiGALFencePoolVulkan* pFencePool, vk::Fence&& vkReclaimFence);
    void DestroyDescriptorPool(xiiGALDescriptorSetPoolVulkan* pDescriptorSetPool, vk::DescriptorPool&& vkDescriptorPool);

    xiiGALDeviceVulkan*     m_pDeviceVulkan;
    xiiDeque<DeletionEntry> m_DeletionQueue;
    xiiMutex                m_DeletionQueueMutex;
  };

  void SafeReleaseDeviceObjectInternal(vk::ObjectType vkObjectType, void* pObject, VmaAllocation vmaAllocation);
  void ReclaimLaterInternal(vk::ObjectType vkObjectType, void* pObject);

  enum class VulkanObjectType : xiiUInt32
  {
    CommandPool,
    CommandBuffer,
    Buffer,
    BufferView,
    Image,
    ImageView,
    DeviceMemory,
    Fence,
    RenderPass,
    Pipeline,
    ShaderModule,
    PipelineLayout,
    Sampler,
    Framebuffer,
    DescriptorPool,
    DescriptorSetLayout,
    DescriptorSet,
    Semaphore,
    Queue,
    Event,
    QueryPool,
    AccelerationStructureKHR,
    PipelineCache
  };

  struct SafeReleaseDescription
  {
    XII_DECLARE_POD_TYPE();

    vk::ObjectType m_vkObjectType  = vk::ObjectType::eUnknown;
    void*          m_pObject       = nullptr;
    VmaAllocation  m_VmaAllocation = {};
  };

  struct SafeReclaimResource
  {
    XII_DECLARE_POD_TYPE();

    vk::ObjectType m_vkObjectType = vk::ObjectType::eUnknown;
    void*          m_pObject      = nullptr;
  };

  struct SafeReclaimCommandBuffer
  {
    XII_DECLARE_POD_TYPE();

    xiiGALCommandBufferPoolVulkan* m_pCommandBufferPool = nullptr;
    vk::CommandBuffer              m_vkCommandBuffer    = VK_NULL_HANDLE;
  };

  struct PerFrameData
  {
    xiiUInt64 m_uiFenceValue = xiiInvalidIndex;

    xiiDeque<SafeReleaseDescription>   m_SafeReleaseDescriptions;
    xiiDeque<SafeReclaimResource>      m_SafeReclaimResources;
    xiiDeque<SafeReclaimCommandBuffer> m_SafeReclaimCommandBuffers;
  };

  // Vulkan Instance Information.
  vk::Instance                      m_Instance;
  xiiUInt32                         m_uiVulkanVersion = 0U;
  vk::detail::DispatchLoaderDynamic m_InstanceDispatchLoader;

  // Vulkan Instance Objects.
  xiiDynamicArray<vk::LayerProperties>     m_Layers;
  xiiDynamicArray<vk::ExtensionProperties> m_Extensions;
  xiiDynamicArray<const char*>             m_EnabledExtensions;
  xiiDynamicArray<vk::PhysicalDevice>      m_PhysicalDevices;

  // Vulkan Physical Device Objects.
  vk::PhysicalDevice                         m_PhysicalDevice;
  vk::PhysicalDeviceProperties               m_PhysicalDeviceProperties;
  vk::PhysicalDeviceFeatures                 m_PhysicalDeviceFeatures;
  vk::PhysicalDeviceMemoryProperties         m_PhysicalDeviceMemoryProperties;
  xiiDynamicArray<vk::QueueFamilyProperties> m_PhysicalDeviceQueueFamilyProperties;
  xiiDynamicArray<vk::ExtensionProperties>   m_PhysicalDeviceSupportedExtensions;
  ExtensionFeatures                          m_PhysicalDeviceExtensionFeatures;
  ExtensionProperties                        m_PhysicalDeviceExtensionProperties;

  // Vulkan Logical Device Objects.
  vk::Device                              m_LogicalDevice;
  vk::PhysicalDeviceFeatures              m_LogicalDeviceFeatures;
  xiiDynamicArray<const char*>            m_LogicalDeviceEnabledExtensions;
  ExtensionFeatures                       m_LogicalDeviceExtensionFeatures;
  xiiDynamicArray<vk::PipelineStageFlags> m_LogicalDeviceSupportedStagesFlags;
  xiiDynamicArray<vk::AccessFlags>        m_LogicalDeviceSupportedAccessFlags;

  // Vulkan Debug Resources.
  DebugMode                  m_DebugMode = DebugMode::Disabled;
  vk::DebugUtilsMessengerEXT m_DebugMessenger;
  vk::DebugReportCallbackEXT m_DebugCallback;

  // Vulkan Memory Allocation.
  VmaAllocator m_vkVmaAllocator = VK_NULL_HANDLE;

  // Graphics Queue Information.
  xiiGALQueueInformationVulkan           m_GraphicsQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pGraphicsCommandQueue;

  // Compute Queue Information.
  xiiGALQueueInformationVulkan           m_ComputeQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pComputeCommandQueue;

  // Transfer Queue Information.
  xiiGALQueueInformationVulkan           m_TransferQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pTransferCommandQueue;

  // Pools.
  xiiUniquePtr<xiiGALFencePoolVulkan>         m_pFencePool;
  xiiUniquePtr<xiiGALSemaphorePoolVulkan>     m_pSemaphorePool;
  xiiUniquePtr<xiiGALDescriptorSetPoolVulkan> m_pDescriptorSetPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pGraphicsCommandQueueQueryPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pComputeCommandQueueQueryPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pTransferCommandQueueQueryPool;

  // Deletion Queue.
  xiiUniquePtr<DeferredDeletionQueue> m_pDeferredDeletionQueue;

  // Per Frame Data.
  xiiUInt32 m_uiFrameCounter = 0U;

private:
  vk::PhysicalDevice SelectPhysicalDevice(xiiUInt32 uiAdapterID) const;
  xiiResult          InitializePhysicalDeviceProperties();

  bool EnumerateInstanceExtensions(const char* szLayerName, xiiDynamicArray<vk::ExtensionProperties>& extensions);
  bool IsLayerAvailable(xiiArrayPtr<const vk::LayerProperties> pLayers, const char* szLayerName, xiiUInt32* pVersion = nullptr) const;
  bool IsExtensionAvailable(xiiArrayPtr<const vk::ExtensionProperties> pExtensions, const char* szExtensionName) const;
  bool IsExtensionEnabled(const char* szExtensionName) const;
  bool IsLogicalDeviceExtensionEnabled(const char* szExtensionName) const;

  xiiGALDeviceFeatures ConvertVulkanFeaturesToDeviceFeatures(xiiUInt32 uiVulkanVersion, const vk::PhysicalDeviceFeatures& vkFeatures, const vk::PhysicalDeviceProperties& vkDeviceProperties, const ExtensionFeatures& extensionFeatures, const ExtensionProperties& extensionProperties, xiiGALDeviceFeatureState::Enum optionalState = xiiGALDeviceFeatureState::Enabled);
  xiiGALDeviceFeatures GetEnabledDeviceFeatures(const xiiGALDeviceFeatures& supportedDeviceFeatures, const xiiGALDeviceFeatures& requestedDeviceFeatures);

  xiiUInt32 FindQueueFamily(vk::QueueFlags queueFlags, xiiArrayPtr<xiiUInt32> excludedQueueIndices = xiiArrayPtr<xiiUInt32>()) const;
};
