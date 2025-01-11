#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

class XII_GRAPHICSVULKAN_DLL xiiGALDeviceVulkan final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceVulkan, xiiGALDevice);

private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& description);

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
  };

  struct QueueInformation
  {
    XII_DECLARE_POD_TYPE();

    vk::Queue m_vkQueue            = VK_NULL_HANDLE;
    xiiUInt32 m_uiQueueFamilyIndex = xiiInvalidIndex;
    xiiUInt32 m_uiQueueIndex       = 0U;
  };

  ~xiiGALDeviceVulkan();

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

  template <typename T>
  void SafeReleaseDeviceObject(T& vkObject, VmaAllocation vmaAlloaction)
  {
    SafeReleaseDeviceObjectInternal(vkObject.objectType, (void*)vkObject, vmaAlloaction);
  }

  template <typename T>
  void SafeReleaseDeviceObject(T& vkObject)
  {
    SafeReleaseDeviceObjectInternal(vkObject.objectType, (void*)vkObject, nullptr);
  }

  template <typename T>
  void ReclaimLater(T& vkObject)
  {
    ReclaimLaterInternal(vkObject.objectType, (void*)vkObject);
  }

  // Internal objects retrieval.

  XII_ALWAYS_INLINE xiiAllocatorBase* GetAllocator() const { return m_Allocator.GetParent(); }
  XII_ALWAYS_INLINE VmaAllocator      GetVulkanMemoryAllocator() const { return m_vkVmaAllocator; }

  XII_ALWAYS_INLINE vk::Instance GetVulkanInstance() const { return m_Instance; }
  XII_ALWAYS_INLINE xiiUInt32    GetVulkanVersion() const { return m_uiVulkanVersion; }
  XII_ALWAYS_INLINE const vk::DispatchLoaderDynamic& GetVulkanDynamicDispatchLoader() const { return m_InstanceDispatchLoader; }

  XII_ALWAYS_INLINE xiiArrayPtr<const vk::LayerProperties> GetVulkanInstanceLayers() const { return m_Layers; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::ExtensionProperties> GetVulkanInstanceExtensionProperties() const { return m_Extensions; }
  XII_ALWAYS_INLINE xiiArrayPtr<const char* const> GetVulkanInstanceEnabledExtensions() const { return m_EnabledExtensions; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::PhysicalDevice> GetVulkanPhysicalDevices() const { return m_PhysicalDevices; }

  XII_ALWAYS_INLINE vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
  XII_ALWAYS_INLINE const vk::PhysicalDeviceProperties& GetVulkanPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }
  XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& GetVulkanPhysicalDeviceFeatures() const { return m_PhysicalDeviceFeatures; }
  XII_ALWAYS_INLINE const vk::PhysicalDeviceMemoryProperties& GetVulkanPhysicalDeviceMemoryProperties() const { return m_PhysicalDeviceMemoryProperties; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::QueueFamilyProperties> GetPhysicalDeviceQueueFamilyProperties() const { return m_PhysicalDeviceQueueFamilyProperties; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::ExtensionProperties> GetPhysicalDeviceSupportedExtensions() const { return m_PhysicalDeviceSupportedExtensions; }
  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& GetPhysicalDeviceExtensionFeatures() const { return m_PhysicalDeviceExtensionFeatures; }
  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionProperties& GetPhysicalDeviceExtensionProperties() const { return m_PhysicalDeviceExtensionProperties; }

  XII_ALWAYS_INLINE vk::Device GetVulkanLogicalDevice() const { return m_LogicalDevice; }
  XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& GetVulkanLogicalDeviceFeatures() const { return m_LogicalDeviceFeatures; }
  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& GetVulkanLogicalDeviceExtensionFeatures() const { return m_LogicalDeviceExtensionFeatures; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::PipelineStageFlags> GetVulkanLogicalDeviceSupportedStagesFlags() const { return m_LogicalDeviceSupportedStagesFlags; }
  XII_ALWAYS_INLINE vk::PipelineStageFlags GetVulkanLogicalDeviceSupportedStagesFlags(xiiUInt32 uiQueueFamilyIndex) const { return m_LogicalDeviceSupportedStagesFlags[uiQueueFamilyIndex]; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::AccessFlags> GetVulkanLogicalDeviceSupportedAccessFlags() const { return m_LogicalDeviceSupportedAccessFlags; }
  XII_ALWAYS_INLINE vk::AccessFlags GetVulkanLogicalDeviceSupportedAccessFlags(xiiUInt32 uiQueueFamilyIndex) const { return m_LogicalDeviceSupportedAccessFlags[uiQueueFamilyIndex]; }

  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& GetGraphicsQueueInformation() const { return m_GraphicsQueueInformation; }
  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& GetComputeQueueInformation() const { return m_ComputeQueueInformation; }
  XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& GetTransferQueueInformation() const { return m_TransferQueueInformation; }

  XII_ALWAYS_INLINE xiiGALDeviceVulkan::DebugMode GetDebugMode() const { return m_DebugMode; }

  XII_ALWAYS_INLINE xiiGALFencePoolVulkan* GetVulkanFencePool() const { return m_FencePool.Borrow(); }
  XII_ALWAYS_INLINE xiiGALSemaphorePoolVulkan* GetVulkanSemaphorePool() const { return m_SemaphorePool.Borrow(); }
  XII_ALWAYS_INLINE xiiGALDescriptorSetPoolVulkan* GetVulkanDescriptorSetPool() const { return m_DescriptorSetPool.Borrow(); }

  XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanGraphicsCommandQueueQueryPool() const { return m_pGraphicsCommandQueueQueryPool.Borrow(); }
  XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanComputeCommandQueueQueryPool() const { return m_pComputeCommandQueueQueryPool.Borrow(); }
  XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetVulkanTransferCommandQueueQueryPool() const { return m_pTransferCommandQueueQueryPool.Borrow(); }
  XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* GetQueryPoolForCommandQueue(xiiGALCommandQueueVulkan* pCommandQueueVulkan) const
  {
    if (m_pGraphicsCommandQueue == pCommandQueueVulkan)
      return m_pGraphicsCommandQueueQueryPool.Borrow();

    if (m_pComputeCommandQueue == pCommandQueueVulkan)
      return m_pComputeCommandQueueQueryPool ? m_pComputeCommandQueueQueryPool.Borrow() : nullptr;

    if (m_pTransferCommandQueue == pCommandQueueVulkan)
      return m_pTransferCommandQueueQueryPool ? m_pTransferCommandQueueQueryPool.Borrow() : nullptr;

    return nullptr;
  }

  xiiGALFenceVulkan* CreateFenceInternal(const xiiGALFenceCreationDescription& description);
  void               DestroyFenceInternal(xiiGALFence* pFence);

  xiiGALSamplerVulkan* CreateSamplerInternal(const xiiGALSamplerCreationDescription& description);
  void                 DestroySamplerInternal(xiiGALSampler* pSampler);

  void ReleasePerFrameResources(xiiUInt64 uiCompletedValue);

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;
  virtual xiiResult ShutdownPlatform() override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains) override final;

  virtual xiiGALSwapChain* CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual void             DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState) override final;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState) override final;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override final;

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader) override final;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr) override final;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer) override final;

  virtual xiiGALBufferView* CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description) override final;
  virtual void              DestroyBufferViewPlatform(xiiGALBufferView* pBufferView) override final;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) override final;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture) override final;

  virtual xiiGALTextureView* CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description) override final;
  virtual void               DestroyTextureViewPlatform(xiiGALTextureView* pTextureView) override final;

  virtual xiiGALSampler* CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual void           DestroySamplerPlatform(xiiGALSampler* pSampler) override final;

  virtual xiiGALInputLayout* CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description) override final;
  virtual void               DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout) override final;

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery) override final;

  virtual xiiGALFence* CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual void         DestroyFencePlatform(xiiGALFence* pFence) override final;

  virtual xiiGALRenderPass* CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual void              DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass) override final;

  virtual xiiGALFramebuffer* CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual void               DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer) override final;

  virtual xiiGALBottomLevelAS* CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual void                 DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS) override final;

  virtual xiiGALTopLevelAS* CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual void              DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS) override final;

  virtual xiiGALPipelineResourceSignature* CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual void                             DestroyPipelineResourceSignaturePlatform(xiiGALPipelineResourceSignature* pPipelineResourceSignature) override final;

  virtual xiiGALPipelineState* CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description) override final;
  virtual void                 DestroyPipelineStatePlatform(xiiGALPipelineState* pPipelineState) override final;

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
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

  struct PerFrameData
  {
    xiiUInt64 m_uiFrameNumber = xiiInvalidIndex;

    xiiDeque<SafeReleaseDescription> m_SafeReleaseDescriptions;
    xiiDeque<SafeReclaimResource>    m_SafeReclaimResources;
  };

  // Vulkan Instance Information.
  vk::Instance              m_Instance;
  xiiUInt32                 m_uiVulkanVersion = 0U;
  vk::DispatchLoaderDynamic m_InstanceDispatchLoader;

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
  QueueInformation                       m_GraphicsQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pGraphicsCommandQueue;

  // Compute Queue Information.
  QueueInformation                       m_ComputeQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pComputeCommandQueue;

  // Graph Queue Information.
  QueueInformation                       m_TransferQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_pTransferCommandQueue;

  // Pools.
  xiiUniquePtr<xiiGALFencePoolVulkan>         m_FencePool;
  xiiUniquePtr<xiiGALSemaphorePoolVulkan>     m_SemaphorePool;
  xiiUniquePtr<xiiGALDescriptorSetPoolVulkan> m_DescriptorSetPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pGraphicsCommandQueueQueryPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pComputeCommandQueueQueryPool;
  xiiUniquePtr<xiiGALQueryPoolVulkan>         m_pTransferCommandQueueQueryPool;

  // Per Frame Data.
  xiiUInt32              m_uiFrameCounter = 0U;
  xiiGALFenceVulkan*     m_pFrameFence    = nullptr;
  xiiDeque<PerFrameData> m_PerFrameData;

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
