/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Containers/Map.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Declarations/Object.h>

/// The xiiGALDevice class is the primary interface for interactions with rendering APIs.
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDevice : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDevice, xiiGALObject);

public:
  /// Initialize device.
  xiiResult Initialize();

  /// Begins a render frame.
  void BeginFrame();

  /// Ends a render frame.
  void EndFrame();

public:
  /// \name Factory Methods.
  ///@{

  /// This creates a swap chain object.
  ///
  /// \param description - The swap chain description. See xiiGALSwapChainCreationDescription.
  ///
  /// \return The reference-counted pointer to the created swap chain object.
  [[nodiscard]] xiiSharedPtr<xiiGALSwapChain> CreateSwapChain(const xiiGALSwapChainCreationDescription& description);


  /// This creates a command list object.
  ///
  /// \param description - The command list description. See xiiGALCommandListCreationDescription.
  ///
  /// \return The reference-counted pointer to the created command list object.
  [[nodiscard]] xiiSharedPtr<xiiGALCommandList> CreateCommandList(const xiiGALCommandListCreationDescription& description);


  /// This creates a new blend state object.
  ///
  /// \param description - The blend state description. See xiiGALBlendStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created blend state object.
  [[nodiscard]] xiiSharedPtr<xiiGALBlendState> CreateBlendState(const xiiGALBlendStateCreationDescription& description);


  /// This creates a new depth stencil state object.
  ///
  /// \param description - The depth stencil state description. See xiiGALDepthStencilStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created depth stencil state object.
  [[nodiscard]] xiiSharedPtr<xiiGALDepthStencilState> CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description);


  /// This creates a new rasterizer state object.
  ///
  /// \param description - The rasterizer state description. See xiiGALRasterizerStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created rasterizer state object.
  [[nodiscard]] xiiSharedPtr<xiiGALRasterizerState> CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description);


  /// This creates a new shader object.
  ///
  /// \param description - The shader description. See xiiGALShaderCreationDescription.
  ///
  /// \return The reference-counted pointer to the created shader object.
  [[nodiscard]] xiiSharedPtr<xiiGALShader> CreateShader(const xiiGALShaderCreationDescription& description);


  /// This creates a new buffer object.
  ///
  /// \param description               - The buffer description. See xiiGALBufferCreationDescription.
  /// \param pInitialData              - The pointer to the xiiGALBufferData structure that describes the initial buffer data or nullptr if no data is provided.
  ///                                    Immutable buffers (xiiGALResourceUsage::Immutable) must be initialized during creation.
  /// \param externalMemoryKind       - The kind of external memory, if the buffer is to be created with externally allocated memory.
  ///
  /// \return The reference-counted pointer to the created buffer object.
  ///
  /// \remarks Size of a uniform buffer (xiiGALBindFlags::UniformBuffer) must be multiple of 16.\n
  /// Stride of a formatted buffer will be computed automatically from the format if the m_uiElementByteStride member of buffer description is set to default value (0).
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateBuffer(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind = xiiGALExternalMemoryKind::None);


  /// This creates a new texture object.
  ///
  /// \param description               - The texture description. See xiiGALTextureCreationDescription.
  /// \param pInitialData              - The pointer to the xiiGALTextureData structure that describes the initial texture data or nullptr if no data is provided.
  ///                                    Immutable textures (xiiGALResourceUsage::Immutable) must be initialized during creation.
  /// \param externalMemoryKind       - The kind of external memory, if the texture is to be created with externally allocated memory.
  ///
  /// \return The reference-counted pointer to the created texture object.
  ///
  /// \remarks  To create all mip levels, set the description.m_uiMipLevels to zero.\n Multi-sampled resources cannot be initialized with data when they are created. \n
  ///           If initial data is provided, number of sub-resources must exactly match the number of sub-resources in the texture (which is the number of mip levels times the number of array slices. For a 3D texture, this is just the number of mip levels).
  ///
  ///           For example, for a 15 x 6 x 2 2D texture array, the following array of sub-resources should be provided: \n 15x6, 7x3, 3x1, 1x1, 15x6, 7x3, 3x1, 1x1.\n
  ///           For a 15 x 6 x 4 3D texture, the following array of sub-resources should be provided:\n 15x6x4, 7x3x2, 3x1x1, 1x1x1
  [[nodiscard]] xiiSharedPtr<xiiGALTexture> CreateTexture(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind = xiiGALExternalMemoryKind::None);


  /// This creates a new sampler object.
  ///
  /// \param description - The sampler description. See xiiGALSamplerCreationDescription.
  ///
  /// \return The reference-counted pointer to the created sampler object.
  ///
  /// \remark If an application attempts to create a sampler interface with the same attributes as an existing interface, the same interface will be returned.
  [[nodiscard]] xiiSharedPtr<xiiGALSampler> CreateSampler(const xiiGALSamplerCreationDescription& description);


  /// This creates a new query object.
  ///
  /// \param description - The query description. See xiiGALQueryCreationDescription.
  ///
  /// \return The reference-counted pointer to the created query object.
  [[nodiscard]] xiiSharedPtr<xiiGALQuery> CreateQuery(const xiiGALQueryCreationDescription& description);


  /// This creates a new fence object.
  ///
  /// \param description - The fence description. See xiiGALFenceCreationDescription.
  ///
  /// \return The reference-counted pointer to the created fence object.
  [[nodiscard]] xiiSharedPtr<xiiGALFence> CreateFence(const xiiGALFenceCreationDescription& description);


  /// This creates a new render pass object.
  ///
  /// \param description - The render pass description. See xiiGALRenderPassCreationDescription.
  ///
  /// \return The reference-counted pointer to the created render pass object.
  [[nodiscard]] xiiSharedPtr<xiiGALRenderPass> CreateRenderPass(const xiiGALRenderPassCreationDescription& description);


  /// This creates a new frame buffer object.
  ///
  /// \param description - The frame buffer description. See xiiGALFramebufferCreationDescription.
  ///
  /// \return The reference-counted pointer to the created frame buffer object.
  [[nodiscard]] xiiSharedPtr<xiiGALFramebuffer> CreateFramebuffer(const xiiGALFramebufferCreationDescription& description);


  /// This creates a new bottom-level acceleration structure object.
  ///
  /// \param description - The bottom-level acceleration structure description. See xiiGALBottomLevelASCreationDescription.
  ///
  /// \return The reference-counted pointer to the created bottom-level acceleration structure object.
  [[nodiscard]] xiiSharedPtr<xiiGALBottomLevelAS> CreateBottomLevelAS(const xiiGALBottomLevelASCreationDescription& description);


  /// This creates a new top-level acceleration structure object.
  ///
  /// \param description - The top-level acceleration structure description. See xiiGALTopLevelASCreationDescription.
  ///
  /// \return The reference-counted pointer to the created top-level acceleration structure object.
  [[nodiscard]] xiiSharedPtr<xiiGALTopLevelAS> CreateTopLevelAS(const xiiGALTopLevelASCreationDescription& description);


  /// This creates a new pipeline resource signature object.
  ///
  /// \param description - The pipeline resource signature description. See xiiGALPipelineResourceSignatureCreationDescription.
  ///
  /// \return The reference-counted pointer to the created pipeline resource signature object.
  [[nodiscard]] xiiSharedPtr<xiiGALPipelineResourceSignature> CreatePipelineResourceSignature(xiiGALPipelineResourceSignatureCreationDescription& description);


  /// This creates a new graphics pipeline state object.
  ///
  /// \param description - The graphics pipeline state description. See xiiGALGraphicsPipelineStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created graphics pipeline state object.
  [[nodiscard]] xiiSharedPtr<xiiGALGraphicsPipelineState> CreateGraphicsPipelineState(const xiiGALGraphicsPipelineStateCreationDescription& description);


  /// This creates a new compute pipeline state object.
  ///
  /// \param description - The compute pipeline state description. See xiiGALComputePipelineStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created compute pipeline state object.
  [[nodiscard]] xiiSharedPtr<xiiGALComputePipelineState> CreateComputePipelineState(const xiiGALComputePipelineStateCreationDescription& description);


  /// This creates a new ray tracing pipeline state object.
  ///
  /// \param description - The ray tracing pipeline state description. See xiiGALRayTracingPipelineStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created ray tracing pipeline state object.
  [[nodiscard]] xiiSharedPtr<xiiGALRayTracingPipelineState> CreateRayTracingPipelineState(const xiiGALRayTracingPipelineStateCreationDescription& description);


  /// This creates a new tile pipeline state object.
  ///
  /// \param description - The tile pipeline state description. See xiiGALTilePipelineStateCreationDescription.
  ///
  /// \return The reference-counted pointer to the created tile pipeline state object.
  [[nodiscard]] xiiSharedPtr<xiiGALTilePipelineState> CreateTilePipelineState(const xiiGALTilePipelineStateCreationDescription& description);

  ///@}

  /// Waits until all outstanding operations on the GPU are complete and destroys any pending resources and GPU objects.
  ///
  /// \note The method blocks the execution of the calling thread until the GPU is idle.
  ///
  /// \remarks The method does not flush command queues, so it will only wait for commands that have been previously submitted for execution. An application should explicitly flush
  ///          the contexts using xiiGALCommandList::Flush() if it needs to make sure all recorded commands are complete when the method returns.
  void WaitIdle();

public:
  /// Registers event handlers.
  static xiiEvent<const xiiGALDeviceEvent&, xiiMutex> s_Events;

  /// Returns the creation description for this device.
  [[nodiscard]] const xiiGALDeviceCreationDescription& GetDescription() const;

  /// Retrieves a pointer to the compute queue if available, null otherwise.
  ///
  /// \param queueType - The queue type that has the required feature.
  ///
  /// \note The default graphics queue is guaranteed to exist, for a successful device initialization.
  [[nodiscard]] virtual xiiGALCommandQueue* GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags = xiiGALCommandQueueFlags::Graphics) const = 0;

  /// This retrieves the device properties. See xiiGraphicsDeviceAdapterDescription.
  [[nodiscard]] const xiiGALGraphicsDeviceAdapterDescription& GetGraphicsDeviceAdapterProperties() const;

  /// This retrieves the device feature states. See xiiGALDeviceFeatures.
  [[nodiscard]] const xiiGALDeviceFeatures& GetFeatures() const;

  /// This retrieves the device limits. See xiiGALDeviceLimits.
  [[nodiscard]] const xiiGALDeviceLimits& GetLimits() const;

  /// This retrieves the device graphics API type. See xiiGALGraphicsDeviceType.
  [[nodiscard]] xiiEnum<xiiGALGraphicsDeviceType> GetGraphicsDeviceType() const;

  /// This returns critical section lock.
  [[nodiscard]] xiiMutex& GetMutex() const;

  /// Sets a default graphics device.
  static void SetDefaultDevice(xiiSharedPtr<xiiGALDevice> pDefaultDevice);

  /// Retrieves the default device. This will be nullptr if none is set.
  [[nodiscard]] static xiiSharedPtr<xiiGALDevice> GetDefaultDevice();

  /// This returns true if there is a set default device.
  [[nodiscard]] static bool HasDefaultDevice();

protected:
  xiiGALDevice(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& creationDescription);

  virtual ~xiiGALDevice();

  /// Initialization after device capabilities are known.
  xiiResult PostInitialize();

  /// Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
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
  friend class xiiGALSampler;
  friend class xiiMemoryUtils;

  virtual xiiResult InitializePlatform()     = 0;
  virtual xiiResult PostInitializePlatform() = 0;

  virtual void BeginFramePlatform() = 0;
  virtual void EndFramePlatform()   = 0;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)                                                                                              = 0;
  virtual xiiInternal::NewInstance<xiiGALCommandList>               CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description)                                                                                          = 0;
  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)                                                                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)                                                                              = 0;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)                                                                                  = 0;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description)                                                                                                    = 0;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)    = 0;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) = 0;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)                                                                                                  = 0;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description)                                                                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description)                                                                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)                                                                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)                                                                                          = 0;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)                                                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)                                                                                            = 0;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)                                                              = 0;
  virtual xiiInternal::NewInstance<xiiGALGraphicsPipelineState>     CreateGraphicsPipelineStatePlatform(const xiiGALGraphicsPipelineStateCreationDescription& description)                                                                      = 0;
  virtual xiiInternal::NewInstance<xiiGALComputePipelineState>      CreateComputePipelineStatePlatform(const xiiGALComputePipelineStateCreationDescription& description)                                                                        = 0;
  virtual xiiInternal::NewInstance<xiiGALRayTracingPipelineState>   CreateRayTracingPipelineStatePlatform(const xiiGALRayTracingPipelineStateCreationDescription& description)                                                                  = 0;
  virtual xiiInternal::NewInstance<xiiGALTilePipelineState>         CreateTilePipelineStatePlatform(const xiiGALTilePipelineStateCreationDescription& description)                                                                              = 0;

  virtual void WaitIdlePlatform() = 0;

  virtual xiiResult FillCapabilitiesPlatform() = 0;

  /// \endcond

protected:
  void FinalizeTextureInternal(const xiiGALTextureCreationDescription& description, xiiSharedPtr<xiiGALTexture>& pTexture);
  void FinalizeBufferInternal(const xiiGALBufferCreationDescription& description, xiiSharedPtr<xiiGALBuffer>& pBuffer);

private:
  void UnregisterSampler(xiiUInt32 uiDescriptionHash, const xiiGALSampler* pSampler);

  static xiiSharedPtr<xiiGALDevice> s_pDefaultDevice;

private:
  xiiMap<xiiUInt32, xiiHybridArray<xiiGALSampler*, 1>> m_SamplerCache;

  bool m_bBeginFrameCalled = false;
};

#include <GraphicsFoundation/Device/Implementation/Device_inl.h>
