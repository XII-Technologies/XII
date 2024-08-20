#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>

class XII_GRAPHICSVULKAN_DLL xiiGALDeviceVulkan final : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& description);

public:
  ~xiiGALDeviceVulkan();

public:
  virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType) const override final;

  // Internal objects retrieval.

  void ReportLiveGPUObjects();

  void FlushPendingObjects();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;
  virtual xiiResult ShutdownPlatform() override final;

  virtual xiiResult CreateCommandQueuesPlatform() override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains) override final;

  virtual xiiGALSwapChain* CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual void             DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual xiiGALCommandQueue* CreateCommandQueuePlatform(const xiiGALCommandQueueCreationDescription& description) override final;
  virtual void                DestroyCommandQueuePlatform(xiiGALCommandQueue* pCommandQueue) override final;

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

  void CreateCommandQueues();

private:
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

    bool m_bSpirv14              = false; // Ray tracing requires Vulkan 1.2 or SPIRV 1.4 extension
    bool m_bSpirv15              = false; // DXC shaders with ray tracing requires Vulkan 1.2 with SPIRV 1.5
    bool m_bSubgroupOps          = false; // Requires Vulkan 1.1
    bool m_bHasPortabilitySubset = false;
    bool m_bRenderPass2          = false;
    bool m_bDrawIndirectCount    = false;
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

  vk::Instance              m_Instance;
  xiiUInt32                 m_uiVulkanVersion = VK_API_VERSION_1_1;
  vk::DispatchLoaderDynamic m_InstanceDispatchLoader;

  // Vulkan Instance Objects.
  xiiDynamicArray<vk::LayerProperties>     m_Layers;
  xiiDynamicArray<vk::ExtensionProperties> m_Extensions;
  xiiDynamicArray<const char*>             m_EnabledExtensions;
  xiiDynamicArray<vk::PhysicalDevice>      m_PhysicalDevices;

  // Vulkan Device Objects.
  vk::Device                                 m_Device;
  vk::PhysicalDevice                         m_PhysicalDevice;
  vk::PhysicalDeviceProperties               m_PhysicalDeviceProperties;
  vk::PhysicalDeviceFeatures                 m_PhysicalDeviceFeatures;
  vk::PhysicalDeviceMemoryProperties         m_PhysicalDeviceMemoryProperties;
  xiiDynamicArray<vk::QueueFamilyProperties> m_PhysicalDeviceQueueFamilyProperties;
  xiiDynamicArray<vk::ExtensionProperties>   m_PhysicalDeviceSupportedExtensions;
  ExtensionFeatures                          m_PhysicalDeviceExtensionFeatures;
  ExtensionProperties                        m_PhysicalDeviceExtensionProperties;

  // Vulkan Debug Resources.
  DebugMode                  m_DebugMode = DebugMode::Disabled;
  vk::DebugUtilsMessengerEXT m_DebugMessenger;
  vk::DebugReportCallbackEXT m_DebugCallback;

  // 0 : Graphics Queue
  // 1 : Compute Queue
  // 2 : Transfer Queue
  // 3 : Sparse Queue
  xiiUniquePtr<xiiGALCommandQueueVulkan> m_CommandQueues[4];

private:
  vk::PhysicalDevice SelectPhysicalDevice(xiiUInt32 uiAdapterID) const;
  xiiResult          InitializePhysicalDeviceProperties();

  bool EnumerateInstanceExtensions(const char* szLayerName, xiiDynamicArray<vk::ExtensionProperties>& extensions);
  bool IsLayerAvailable(xiiArrayPtr<const vk::LayerProperties> pLayers, const char* szLayerName, xiiUInt32* pVersion = nullptr);
  bool IsExtensionAvailable(xiiArrayPtr<const vk::ExtensionProperties> pExtensions, const char* szExtensionName);
  bool IsExtensionEnabled(const char* szExtensionName);

  xiiGALDeviceFeatures ConvertVulkanFeaturesToDeviceFeatures(xiiUInt32 uiVulkanVersion, const vk::PhysicalDeviceFeatures& vkFeatures, const vk::PhysicalDeviceProperties& vkDeviceProperties, const ExtensionFeatures& extensionFeatures, const ExtensionProperties& extensionProperties, xiiGALDeviceFeatureState::Enum optionalState = xiiGALDeviceFeatureState::Enabled);
  xiiGALDeviceFeatures GetEnabledDeviceFeatures(const xiiGALDeviceFeatures& supportedDeviceFeatures, const xiiGALDeviceFeatures& requestedDeviceFeatures);
};

#include <GraphicsVulkan/Device/Implementation/DeviceVulkan_inl.h>
