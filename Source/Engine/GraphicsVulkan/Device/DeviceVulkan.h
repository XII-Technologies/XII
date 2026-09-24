/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Device/Device.h>

namespace vk
{
  class Semaphore;
}

class XII_GRAPHICSVULKAN_DLL xiiGALDeviceVulkan final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceVulkan, xiiGALDevice);

private:
  friend class xiiMemoryUtils;

  friend xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceVulkan(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

  virtual ~xiiGALDeviceVulkan();

public:
  enum class DebugMode
  {
    Disabled,
    Utils,
    Report,
  };

  struct InstanceFlags
  {
    DebugMode m_DebugMode                      = DebugMode::Disabled;
    bool      m_bExternalMemoryCapabilities    = false;
    bool      m_bExternalSemaphoreCapabilities = false;
    bool      m_bExternalFenceCapabilities     = false;
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
    vk::PhysicalDeviceFragmentDensityMapFeaturesEXT     m_FragmentDensityMap;  ///< Only for desktop devices.
    vk::PhysicalDeviceFragmentDensityMap2FeaturesEXT    m_FragmentDensityMap2; ///< Only for mobile devices.
    vk::PhysicalDeviceMultiviewFeaturesKHR              m_Multiview;           ///< Required for RenderPass2.
    vk::PhysicalDeviceMultiDrawFeaturesEXT              m_MultiDraw;
    vk::PhysicalDeviceShaderDrawParametersFeatures      m_ShaderDrawParameters;
    vk::PhysicalDeviceCustomBorderColorFeaturesEXT      m_CustomBorderColor;

    bool m_bSpirv14                  = false; ///< Ray tracing requires Vulkan 1.2 or SPIRV 1.4 extension.
    bool m_bSpirv15                  = false; ///< DXC shaders with ray tracing requires Vulkan 1.2 with SPIRV 1.5.
    bool m_bSubgroupOps              = false; ///< Requires Vulkan 1.1.
    bool m_bHasPortabilitySubset     = false;
    bool m_bRenderPass2              = false;
    bool m_bDrawIndirectCount        = false;
    bool m_bShaderViewportIndexLayer = false;
    bool m_bExternalMemory           = false; ///< External memory support.
    bool m_bExternalMemoryWin32      = false;
    bool m_bExternalMemoryFd         = false;
    bool m_bExternalSemaphore        = false; ///< External semaphore support.
    bool m_bExternalSemaphoreWin32   = false;
    bool m_bExternalSemaphoreFd      = false;
    bool m_bExternalFence            = false; ///< External fence support.
    bool m_bExternalFenceWin32       = false;
    bool m_bExternalFenceFd          = false;
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
    vk::PhysicalDeviceDepthStencilResolveProperties       m_DepthStencilResolve;
    vk::ExternalMemoryPropertiesKHR                       m_ExternalMemoryProperty;
  };

  class DeferredDeletionQueue;

public:
  virtual xiiGALCommandQueue* GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

  template <typename ObjectHandle, typename = typename std::enable_if<std::is_object<ObjectHandle>::value>::type>
  XII_FORCE_INLINE void SetVulkanObjectDebugName(ObjectHandle& vkObject, const char* szDebugName, xiiVulkanAllocation allocation = {}) const;

  template <typename T, typename = std::enable_if_t<std::is_class_v<T> && HasObjectType<T>::value>>
  XII_FORCE_INLINE void SafeReleaseDeviceObject(T&& vkObject, xiiVulkanAllocation&& allocation = nullptr, vk::DeviceMemory&& vkExternalMemory = nullptr);

  template <typename T>
  XII_ALWAYS_INLINE void ReclaimLater(T&& vkObject) { ReclaimLaterInternal(vkObject.objectType, (void*)vkObject); }

  // Internal objects retrieval.

  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocator*             GetAllocator() const { return m_Allocator.GetParent(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVulkanMemoryAllocator* GetVulkanMemoryAllocator() const { return m_pVulkanMemoryAllocator.Borrow(); }

  [[nodiscard]] XII_ALWAYS_INLINE vk::Instance GetVulkanInstance() const { return m_Instance; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32    GetVulkanVersion() const { return m_uiVulkanVersion; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::detail::DispatchLoaderDynamic& GetVulkanDynamicDispatchLoader() const { return m_InstanceDispatchLoader; }

  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALDeviceVulkan::InstanceFlags& GetVulkanInstanceFlags() const { return m_InstanceFlags; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const char* const> GetVulkanInstanceEnabledExtensions() const { return m_EnabledExtensions; }

  [[nodiscard]] XII_ALWAYS_INLINE vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceProperties& GetVulkanPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& GetVulkanPhysicalDeviceFeatures() const { return m_PhysicalDeviceFeatures; }
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PhysicalDeviceMemoryProperties& GetVulkanPhysicalDeviceMemoryProperties() const { return m_PhysicalDeviceMemoryProperties; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::QueueFamilyProperties> GetPhysicalDeviceQueueFamilyProperties() const { return m_PhysicalDeviceQueueFamilyProperties; }
  /// Queue families that may execute engine command lists. Resources shared by the
  /// render graph use this list with VK_SHARING_MODE_CONCURRENT, which avoids
  /// implicit ownership assumptions when dedicated compute/transfer families are
  /// available.
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetActiveQueueFamilyIndices() const { return m_ActiveQueueFamilyIndices; }
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

  XII_ALWAYS_INLINE void                                              LockCommandQueueAndRun(xiiBitflags<xiiGALCommandQueueFlags> queueFlags, xiiDelegate<void(const vk::Queue&)> action);
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetCommandQueueInformation(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;
  [[nodiscard]] XII_ALWAYS_INLINE xiiMutex&                           GetCommandQueueMutex(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandBufferPoolVulkan*      GetCommandBufferPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolVulkan*              GetCommandQueueQueryPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineStageFlags GetSupportedStagesFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;
  [[nodiscard]] XII_ALWAYS_INLINE vk::AccessFlags GetSupportedAccessFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDeviceVulkan::DebugMode GetDebugMode() const { return m_InstanceFlags.m_DebugMode; }

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALFencePoolVulkan*     GetVulkanFencePool() const { return m_pFencePool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALSemaphorePoolVulkan* GetVulkanSemaphorePool() const { return m_pSemaphorePool.Borrow(); }

  // Deactivate Doxygen document generation for the following block. (API implementation only)
  /// \cond

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;

  virtual void BeginFramePlatform() override final;
  virtual void EndFramePlatform() override final;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALCommandList>               CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALGraphicsPipelineState>     CreateGraphicsPipelineStatePlatform(const xiiGALGraphicsPipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALComputePipelineState>      CreateComputePipelineStatePlatform(const xiiGALComputePipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRayTracingPipelineState>   CreateRayTracingPipelineStatePlatform(const xiiGALRayTracingPipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTilePipelineState>         CreateTilePipelineStatePlatform(const xiiGALTilePipelineStateCreationDescription& description) override final;

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

  /// \endcond

private:
  void SafeReleaseDeviceObjectInternal(vk::ObjectType vkObjectType, void* pObject, xiiVulkanAllocation allocation, vk::DeviceMemory vkExternalMemory);
  void ReclaimLaterInternal(vk::ObjectType vkObjectType, void* pObject);
  void SetVulkanAllocationDebugName(xiiVulkanAllocation allocation, const char* szDebugName) const;

  vk::PhysicalDevice SelectPhysicalDevice(xiiArrayPtr<vk::PhysicalDevice> pPhysicalDevices, xiiUInt32 uiAdapterID) const;
  xiiResult          InitializePhysicalDeviceProperties();

  bool EnumerateInstanceExtensions(const char* szLayerName, xiiDynamicArray<vk::ExtensionProperties>& extensions);
  bool IsLayerAvailable(xiiArrayPtr<const vk::LayerProperties> pLayers, const char* szLayerName, xiiUInt32* pVersion = nullptr) const;
  bool IsExtensionAvailable(xiiArrayPtr<const vk::ExtensionProperties> pExtensions, const char* szExtensionName) const;
  bool IsExtensionEnabled(xiiArrayPtr<const char*> pEnabledExtensions, const char* szExtensionName) const;
  bool IsLogicalDeviceExtensionEnabled(const char* szExtensionName) const;

  xiiGALDeviceFeatures ConvertVulkanFeaturesToDeviceFeatures(xiiUInt32 uiVulkanVersion, const vk::PhysicalDeviceFeatures& vkFeatures, const vk::PhysicalDeviceProperties& vkDeviceProperties, const ExtensionFeatures& extensionFeatures, const ExtensionProperties& extensionProperties, xiiGALDeviceFeatureState::Enum optionalState = xiiGALDeviceFeatureState::Enabled);
  xiiGALDeviceFeatures GetEnabledDeviceFeatures(const xiiGALDeviceFeatures& supportedDeviceFeatures, const xiiGALDeviceFeatures& requestedDeviceFeatures);

  xiiUInt32 FindQueueFamily(vk::QueueFlags queueFlags, xiiArrayPtr<xiiUInt32> excludedQueueIndices = xiiArrayPtr<xiiUInt32>()) const;

private:
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

    vk::ObjectType      m_vkObjectType     = vk::ObjectType::eUnknown;
    void*               m_pObject          = nullptr;
    xiiVulkanAllocation m_VulkanAllocation = {};
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
  InstanceFlags                m_InstanceFlags;
  xiiDynamicArray<const char*> m_EnabledExtensions;

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
  xiiHybridArray<xiiUInt32, 3U>           m_ActiveQueueFamilyIndices;

  // Vulkan Debug Resources.
  vk::DebugUtilsMessengerEXT m_DebugMessenger;
  vk::DebugReportCallbackEXT m_DebugCallback;

  // Vulkan Memory Allocator.
  xiiUniquePtr<xiiVulkanMemoryAllocator> m_pVulkanMemoryAllocator;

  // Graphics Queue Information.
  mutable xiiMutex                            m_GraphicsQueueMutex;
  xiiGALQueueInformationVulkan                m_GraphicsQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan>      m_pGraphicsCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pGraphicsCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandBufferPoolVulkan> m_pGraphicsCommandBufferPool;

  // Compute Queue Information.
  mutable xiiMutex                            m_ComputeQueueMutex;
  xiiGALQueueInformationVulkan                m_ComputeQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan>      m_pComputeCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pComputeCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandBufferPoolVulkan> m_pComputeCommandBufferPool;

  // Transfer Queue Information.
  mutable xiiMutex                            m_TransferQueueMutex;
  xiiGALQueueInformationVulkan                m_TransferQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan>      m_pTransferCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pTransferCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandBufferPoolVulkan> m_pTransferCommandBufferPool;

  // Pools.
  xiiUniquePtr<xiiGALFencePoolVulkan>     m_pFencePool;
  xiiUniquePtr<xiiGALSemaphorePoolVulkan> m_pSemaphorePool;

  // Deletion Queue.
  xiiUniquePtr<DeferredDeletionQueue> m_pDeferredDeletionQueue;
};

#include <GraphicsVulkan/Device/Implementation/DeviceVulkan_inl.h>
