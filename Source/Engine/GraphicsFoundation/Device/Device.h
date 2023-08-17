#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

/// \brief The xiiRenderDevice class is the primary interface for interactions with rendering APIs.
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDevice
{
public:
  xiiResult Initialize();
  xiiResult Shutdown();

  void BeginPipeline(xiiStringView Name, xiiGALSwapChainHandle hSwapChain);
  void EndPipeline(xiiGALSwapChainHandle hSwapChain);

  xiiGALPass* BeginPass(xiiStringView Name);
  void        EndPass(xiiGALPass* pPass);

public:
  /// \brief This creates a new blend state object.
  ///
  /// \param description - The blend state description. See xiiGALBlendStateCreationDescription.
  ///
  /// \return The handle to the created blend state object. The function calls AddRef(), so that the new object will have one reference.
  xiiGALBlendStateHandle CreateBlendState(const xiiGALBlendStateCreationDescription& description);

  /// \brief This destroys the blend state with the given handle.
  void DestroyBlendState(xiiGALBlendStateHandle hBlendState);


  /// \brief This creates a new depth stencil state object.
  ///
  /// \param description - The depth stencil state description. See xiiGALDepthStencilStateCreationDescription.
  ///
  /// \return The handle to the created depth stencil state object. The function calls AddRef(), so that the new object will have one reference.
  xiiGALDepthStencilStateHandle CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description);

  /// \brief This destroys the depth stencil state with the given handle.
  void DestroyDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState);


  /// \brief This creates a new rasterizer state object.
  ///
  /// \param description - The rasterizer state description. See xiiGALRasterizerStateCreationDescription.
  ///
  /// \return The handle to the created rasterizer state object. The function calls AddRef(), so that the new object will have one reference.
  xiiGALRasterizerStateHandle CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description);

  /// \brief This destroys the rasterizer state with the given handle.
  void DestroyRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);


  /// \brief This creates a new shader object.
  ///
  /// \param description - The shader description. See xiiGALShaderCreationDescription.
  ///
  /// \return The handle to the created shader object. The function calls AddRef(), so that the new object will have one reference.
  xiiGALShaderHandle CreateShader(const xiiGALShaderCreationDescription& description);

  /// \brief This destroys the shader with the given handle.
  void DestroyShader(xiiGALShaderHandle hShader);


  /// \brief This creates a new buffer object.
  ///
  /// \param description  - The buffer description. See xiiGALBufferCreationDescription.
  /// \param pInitialData - The pointer to the xiiGALBufferData structure that describes the initial buffer data or nullptr if no data is provided.
  ///                       Immutable buffers (xiiGALResourceUsage::Immutable) must be initialized during creation.
  ///
  /// \return The handle to the created buffer object. The function calls AddRef(), so that the new object will have one reference.
  ///
  /// \remarks Size of a uniform buffer (xiiGALBindFlags) must be multiple of 16.\n
  /// Stride of a formatted buffer will be computed automatically from the format if the m_uiElementByteStride member of buffer description is set to default value (0).
  xiiGALBufferHandle CreateBuffer(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr);

  /// \brief This destroys the buffer with the given handle.
  void DestroyBuffer(xiiGALBufferHandle hBuffer);


  /// \brief This creates a new texture object.
  ///
  /// \param description  - The texture description. See xiiGALTextureCreationDescription.
  /// \param pInitialData - The pointer to the xiiGALTextureData structure that describes the initial texture data or nullptr if no data is provided.
  ///                       Immutable textures (xiiGALResourceUsage::Immutable) must be initialized during creation.
  ///
  /// \return The handle to the created texture object. The function calls AddRef(), so that the new object will have one reference.
  ///
  /// \remarks
  /// To create all mip levels, set the description.m_uiMipLevels to zero.\n Multi-sampled resources cannot be initialized with data when they are created. \n
  /// If initial data is provided, number of sub-resources must exactly match the number of sub-resources in the texture (which is the number of mip levels times the number of array slices. For a 3D texture, this is just the number of mip levels).
  ///
  /// For example, for a 15 x 6 x 2 2D texture array, the following array of sub-resources should be provided: \n 15x6, 7x3, 3x1, 1x1, 15x6, 7x3, 3x1, 1x1.\n
  /// For a 15 x 6 x 4 3D texture, the following array of sub-resources should be provided:\n 15x6x4, 7x3x2, 3x1x1, 1x1x1
  xiiGALTextureHandle CreateTexture(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr);

  /// \brief This destroys the texture with the given handle.
  void DestroyTexture(xiiGALTextureHandle hTexture);


  /// \brief This creates a new sampler object.
  ///
  /// \param description - The sampler description. See xiiGALSamplerCreationDescription.
  ///
  /// \return The handle to the created sampler object. The function calls AddRef(), so that the new object will have one reference.
  ///
  /// \remark If an application attempts to create a sampler interface with the same attributes as an existing interface, the same interface will be returned.
  xiiGALSamplerHandle CreateSampler(const xiiGALSamplerCreationDescription& description);

  /// \brief This destroys the sampler with the given handle.
  void DestroySampler(xiiGALSamplerHandle hSamplerState);


  /// \brief This creates a new input layout object.
  ///
  /// \param description - The input layout description. See xiiGALInputLayoutCreationDescription.
  ///
  /// \return The handle to the created input layout object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALInputLayoutHandle CreateVertexDeclaration(const xiiGALInputLayoutCreationDescription& description);

  /// \brief This destroys the input layout with the given handle.
  virtual void DestroyVertexDeclaration(xiiGALInputLayoutHandle hInputLayout);


  /// \brief This creates a new query object.
  ///
  /// \param description - The query description. See xiiGALQueryCreationDescription.
  ///
  /// \return The handle to the created query object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALQueryHandle CreateQuery(const xiiGALQueryCreationDescription& description);

  /// \brief This destroys the sampler with the given handle.
  virtual void DestroyQuery(xiiGALQueryHandle hQuery);


  /// \brief This creates a new fence object.
  ///
  /// \param description - The fence description. See xiiGALFenceCreationDescription.
  ///
  /// \return The handle to the created fence object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALFenceHandle CreateFence(const xiiGALFenceCreationDescription& description);

  /// \brief This destroys the fence with the given handle.
  virtual void DestroyFence(xiiGALFenceHandle hFence);


  /// \brief This creates a new render pass object.
  ///
  /// \param description - The render pass description. See xiiGALRenderPassCreationDescription.
  ///
  /// \return The handle to the created render pass object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALRenderPassHandle CreateRenderPass(const xiiGALRenderPassCreationDescription& description);

  /// \brief This destroys the render pass with the given handle.
  virtual void DestroyRenderPass(xiiGALRenderPassHandle hRenderPass);


  /// \brief This creates a new frame buffer object.
  ///
  /// \param description - The frame buffer description. See xiiGALFramebufferCreationDescription.
  ///
  /// \return The handle to the created frame buffer object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALFramebufferHandle CreateFramebuffer(const xiiGALFramebufferCreationDescription& description);

  /// \brief This destroys the frame buffer with the given handle.
  virtual void DestroyFramebuffer(xiiGALFramebufferHandle hFramebuffer);


  /// \brief This creates a new bottom-level acceleration structure object.
  ///
  /// \param description - The bottom-level acceleration structure description. See xiiGALBottomLevelASCreationDescription.
  ///
  /// \return The handle to the created bottom-level acceleration structure object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALBottomLevelASHandle CreateBottomLevelAS(const xiiGALBottomLevelASCreationDescription& description);

  /// \brief This destroys the bottom-level acceleration structure with the given handle.
  virtual void DestroyBottomLevelAS(xiiGALBottomLevelASHandle hBottomLevelAS);


  /// \brief This creates a new top-level acceleration structure object.
  ///
  /// \param description - The top-level acceleration structure description. See xiiGALTopLevelASCreationDescription.
  ///
  /// \return The handle to the created top-level acceleration structure object. The function calls AddRef(), so that the new object will have one reference.
  virtual xiiGALTopLevelASHandle CreateTopLevelAS(const xiiGALTopLevelASCreationDescription& description);

  /// \brief This destroys the top-level acceleration structure with the given handle.
  virtual void DestroyTopLevelAS(xiiGALTopLevelASHandle hTopLevelAS);


  /// \brief Waits until all outstanding operations on the GPU are complete and destroys any pending resources and GPU objects.
  ///
  /// \note The method blocks the execution of the calling thread until the GPU is idle.
  ///
  /// \remarks The method does not flush immediate contexts, so it will only wait for commands that have been previously submitted for execution. An application should explicitly flush
  ///          the contexts using xiiGALCommandEncoder::Flush() if it needs to make sure all recorded commands are complete when the method returns.
  virtual void WaitIdle();


public:
  xiiEvent<const xiiGALDeviceEvent&> m_Events;
};

#include <GraphicsFoundation/Device/Implementation/Device_inl.h>
