#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Declarations/Object.h>

/// \brief The xiiGALDevice class is the primary interface for interactions with rendering APIs.
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDevice : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDevice, xiiGALObject);

public:
  /// \brief Initialize device.
  xiiResult Initialize();

  /// \brief Adds a swapchain to be used for the next frame.
  ///
  /// This must be called before or during xiiGALDeviceEvent::BeforeBeginFrame event (xiiGALDevice::BeginFrame), and repeated every frame the swap chain is to be used.
  /// This approach gurantees that all swapchains of a frame, acquire and present at the same time, which improves frame pacing.
  ///
  /// \param pSwapChain - The swapchain to be used this frame. The device will acquire an image from the swapchain during xiiGALDevice::BeginFrame and present it when calling xiiGALDevice::EndFrame.
  void EnqueueFrameSwapChain(xiiSharedPtr<xiiGALSwapChain> pSwapChain);

  /// \brief Begins a render frame.
  void BeginFrame(const xiiUInt64 uiRenderFrame = 0U);

  /// \brief Ends a render frame.
  void EndFrame();

public:
  /// \brief This creates a swap chain object.
  ///
  /// \param description - The swap chain description. See xiiGALSwapChainCreationDescription.
  ///
  /// \return The reference-counted pointer to the created swap chain object.
  [[nodiscard]] xiiSharedPtr<xiiGALSwapChain> CreateSwapChain(const xiiGALSwapChainCreationDescription& description);


  /// \brief This creates a new blend state object.
  ///
  /// \param description - The blend state description. See xiiGALBlendStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created blend state object.
  [[nodiscard]] xiiSharedPtr<xiiGALBlendState> CreateBlendState(const xiiGALBlendStateCreationDescription& description);


  /// \brief This creates a new depth stencil state object.
  ///
  /// \param description - The depth stencil state description. See xiiGALDepthStencilStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created depth stencil state object.
  [[nodiscard]] xiiSharedPtr<xiiGALDepthStencilState> CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description);


  /// \brief This creates a new rasterizer state object.
  ///
  /// \param description - The rasterizer state description. See xiiGALRasterizerStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created rasterizer state object.
  [[nodiscard]] xiiSharedPtr<xiiGALRasterizerState> CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description);


  /// \brief This creates a new shader object.
  ///
  /// \param description - The shader description. See xiiGALShaderCreationDescription.
  ///
  /// \return The reference-counted pointer to the created shader object.
  [[nodiscard]] xiiSharedPtr<xiiGALShader> CreateShader(const xiiGALShaderCreationDescription& description);


  /// \brief This creates a new buffer object.
  ///
  /// \param description  - The buffer description. See xiiGALBufferCreationDescription.
  /// \param pInitialData - The pointer to the xiiGALBufferData structure that describes the initial buffer data or nullptr if no data is provided.
  ///                       Immutable buffers (xiiGALResourceUsage::Immutable) must be initialized during creation.
  ///
  /// \return The reference-counted pointer to the created buffer object.
  ///
  /// \remarks Size of a uniform buffer (xiiGALBindFlags::UniformBuffer) must be multiple of 16.\n
  /// Stride of a formatted buffer will be computed automatically from the format if the m_uiElementByteStride member of buffer description is set to default value (0).
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateBuffer(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr);


  /// \brief This creates a new texture object.
  ///
  /// \param description  - The texture description. See xiiGALTextureCreationDescription.
  /// \param pInitialData - The pointer to the xiiGALTextureData structure that describes the initial texture data or nullptr if no data is provided.
  ///                       Immutable textures (xiiGALResourceUsage::Immutable) must be initialized during creation.
  ///
  /// \return The reference-counted pointer to the created texture object.
  ///
  /// \remarks
  /// To create all mip levels, set the description.m_uiMipLevels to zero.\n Multi-sampled resources cannot be initialized with data when they are created. \n
  /// If initial data is provided, number of sub-resources must exactly match the number of sub-resources in the texture (which is the number of mip levels times the number of array slices. For a 3D texture, this is just the number of mip levels).
  ///
  /// For example, for a 15 x 6 x 2 2D texture array, the following array of sub-resources should be provided: \n 15x6, 7x3, 3x1, 1x1, 15x6, 7x3, 3x1, 1x1.\n
  /// For a 15 x 6 x 4 3D texture, the following array of sub-resources should be provided:\n 15x6x4, 7x3x2, 3x1x1, 1x1x1
  [[nodiscard]] xiiSharedPtr<xiiGALTexture> CreateTexture(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr);


  /// \brief This creates a new sampler object.
  ///
  /// \param description - The sampler description. See xiiGALSamplerCreationDescription.
  ///
  /// \return The reference-counted pointer to the created sampler object.
  ///
  /// \remark If an application attempts to create a sampler interface with the same attributes as an existing interface, the same interface will be returned.
  [[nodiscard]] xiiSharedPtr<xiiGALSampler> CreateSampler(const xiiGALSamplerCreationDescription& description);


  /// \brief This creates a new query object.
  ///
  /// \param description - The query description. See xiiGALQueryCreationDescription.
  ///
  /// \return The reference-counted pointer to the created query object.
  [[nodiscard]] xiiSharedPtr<xiiGALQuery> CreateQuery(const xiiGALQueryCreationDescription& description);


  /// \brief This creates a new fence object.
  ///
  /// \param description - The fence description. See xiiGALFenceCreationDescription.
  ///
  /// \return The reference-counted pointer to the created fence object.
  [[nodiscard]] xiiSharedPtr<xiiGALFence> CreateFence(const xiiGALFenceCreationDescription& description);


  /// \brief This creates a new render pass object.
  ///
  /// \param description - The render pass description. See xiiGALRenderPassCreationDescription.
  ///
  /// \return The reference-counted pointer to the created render pass object.
  [[nodiscard]] xiiSharedPtr<xiiGALRenderPass> CreateRenderPass(const xiiGALRenderPassCreationDescription& description);


  /// \brief This creates a new frame buffer object.
  ///
  /// \param description - The frame buffer description. See xiiGALFramebufferCreationDescription.
  ///
  /// \return The reference-counted pointer to the created frame buffer object.
  [[nodiscard]] xiiSharedPtr<xiiGALFramebuffer> CreateFramebuffer(const xiiGALFramebufferCreationDescription& description);


  /// \brief This creates a new bottom-level acceleration structure object.
  ///
  /// \param description - The bottom-level acceleration structure description. See xiiGALBottomLevelASCreationDescription.
  ///
  /// \return The reference-counted pointer to the created bottom-level acceleration structure object.
  [[nodiscard]] xiiSharedPtr<xiiGALBottomLevelAS> CreateBottomLevelAS(const xiiGALBottomLevelASCreationDescription& description);


  /// \brief This creates a new top-level acceleration structure object.
  ///
  /// \param description - The top-level acceleration structure description. See xiiGALTopLevelASCreationDescription.
  ///
  /// \return The reference-counted pointer to the created top-level acceleration structure object.
  [[nodiscard]] xiiSharedPtr<xiiGALTopLevelAS> CreateTopLevelAS(const xiiGALTopLevelASCreationDescription& description);


  /// \brief This creates a new pipeline resource signature object.
  ///
  /// \param description - The pipeline resource signature description. See xiiGALPipelineResourceSignatureCreationDescription.
  ///
  /// \return The reference-counted pointer to the created pipeline resource signature object.
  [[nodiscard]] xiiSharedPtr<xiiGALPipelineResourceSignature> CreatePipelineResourceSignature(xiiGALPipelineResourceSignatureCreationDescription& description);


  /// \brief This creates a new pipeline state object.
  ///
  /// \param description - The pipeline state description. See xiiGALPipelineStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created pipeline state object.
  [[nodiscard]] xiiSharedPtr<xiiGALPipelineState> CreatePipelineState(const xiiGALPipelineStateCreationDescription& description);


  /// \brief Waits until all outstanding operations on the GPU are complete and destroys any pending resources and GPU objects.
  ///
  /// \note The method blocks the execution of the calling thread until the GPU is idle.
  ///
  /// \remarks The method does not flush command queues, so it will only wait for commands that have been previously submitted for execution. An application should explicitly flush
  ///          the contexts using xiiGALCommandList::Flush() if it needs to make sure all recorded commands are complete when the method returns.
  void WaitIdle();

public:
  /// \brief Registers event handlers.
  static xiiEvent<const xiiGALDeviceEvent&, xiiMutex> s_Events;

  /// \brief Returns the creation description for this device.
  [[nodiscard]] const xiiGALDeviceCreationDescription& GetDescription() const;

  /// \brief Retrieves a pointer to the compute queue if available, null otherwise.
  ///
  /// \param queueType - The queue type that has the required feature.
  ///
  /// \note The default graphics queue is guaranteed to exist, for a successful device initialization.
  [[nodiscard]] virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType = xiiGALCommandQueueType::Graphics, bool bAllowGraphicsCommandQueueFallback = true) const = 0;

  /// \brief This retrieves the device properties. See xiiGraphicsDeviceAdapterDescription.
  [[nodiscard]] const xiiGALGraphicsDeviceAdapterDescription& GetGraphicsDeviceAdapterProperties() const;

  /// \brief This retrieves the device feature states. See xiiGALDeviceFeatures.
  [[nodiscard]] const xiiGALDeviceFeatures& GetFeatures() const;

  /// \brief This retrieves the device graphics API type. See xiiGALGraphicsDeviceType.
  [[nodiscard]] xiiEnum<xiiGALGraphicsDeviceType> GetGraphicsDeviceType() const;

  /// \brief This returns critical section lock.
  [[nodiscard]] xiiMutex& GetMutex() const;

  /// \brief Sets a default graphics device.
  ///
  /// \remarks This does not increase the reference count on the device.
  static void SetDefaultDevice(xiiSharedPtr<xiiGALDevice> pDefaultDevice);

  /// \brief Retrieves the default device. This will be nullptr if none is set.
  [[nodiscard]] static xiiSharedPtr<xiiGALDevice> GetDefaultDevice();

  /// \brief This returns true if there is a set default device.
  [[nodiscard]] static bool HasDefaultDevice();

protected:
  xiiGALDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& creationDescription);

  virtual ~xiiGALDevice();

  /// \brief Initialization after device capabilities are known.
  xiiResult PostInitialize();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  xiiGALDeviceCreationDescription m_Description;

  xiiProxyAllocator        m_Allocator;
  xiiLocalAllocatorWrapper m_AllocatorWrapper;

  mutable xiiMutex m_Mutex;

  xiiGALGraphicsDeviceAdapterDescription m_AdapterDescription;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a graphics API abstraction.
protected:
  friend class xiiMemoryUtils;

  virtual xiiResult InitializePlatform()     = 0;
  virtual xiiResult PostInitializePlatform() = 0;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains, const xiiUInt64 uiRenderFrame = 0U) = 0;
  virtual void EndFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains)                                       = 0;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)                                              = 0;
  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)                              = 0;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)                                  = 0;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description)                                                    = 0;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr)    = 0;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) = 0;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)                                                  = 0;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description)                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description)                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)                                          = 0;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)              = 0;
  virtual xiiInternal::NewInstance<xiiGALPipelineState>             CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description)                                      = 0;

  virtual void WaitIdlePlatform() = 0;

  virtual xiiResult FillCapabilitiesPlatform() = 0;

  /// \endcond

protected:
  void FinalizeTextureInternal(const xiiGALTextureCreationDescription& description, xiiSharedPtr<xiiGALTexture>& pTexture);
  void FinalizeBufferInternal(const xiiGALBufferCreationDescription& description, xiiSharedPtr<xiiGALBuffer>& pBuffer);

private:
  static xiiSharedPtr<xiiGALDevice> s_pDefaultDevice;

private:
  bool                                              m_bBeginFrameCalled = false;
  xiiHybridArray<xiiSharedPtr<xiiGALSwapChain>, 8U> m_FrameSwapChains;
};

#include <GraphicsFoundation/Device/Implementation/Device_inl.h>
