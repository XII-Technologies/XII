#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Shader/Shader.h>

/// \brief This describes the pipeline state shading rate flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSetVertexBufferFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None  = 0x0U,       ///< No additional operations.
    Reset = XII_BIT(0), ///< Reset the vertex buffers outside the range of the currently set vertex buffers. All buffers previously bound to the pipeline will be unbound.

    Default = None
  };

  struct Bits
  {
    StorageType Reset : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSetVertexBufferFlags);

/// \brief This describes the resource state transition mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStateTransitionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0,   ///< Perform no state transitions and no validation. Resource states are not accessed (either read or written) by the command.
    Transition, ///< Transition resources to the states required by the specific command. Resources in unknown state are ignored.
    Verify,     /// Do not transition, but verify that states are correct. No validation is performed on Shipping builds.

    Default = None
  };
};

/// \brief This describes the viewport.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALViewport : public xiiHashableStruct<xiiGALViewport>
{
  XII_DECLARE_POD_TYPE();

  float m_fTopLeftX = 0.0f;
  float m_fTopLeftY = 0.0f;
  float m_fWidth    = 0.0f;
  float m_fHeight   = 0.0f;
  float m_fMinDepth = 0.0f;
  float m_fMaxDepth = 1.0f;
};

/// \brief This describes the viewport.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBeginRenderPassDescription : public xiiHashableStruct<xiiGALBeginRenderPassDescription>
{
  xiiSharedPtr<xiiGALRenderPass>                                            m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer>                                           m_pFramebuffer;
  xiiStaticArray<xiiGALOptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT> m_ClearValues;
};

/// \brief This describes the command list creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListCreationDescription : public xiiHashableStruct<xiiGALCommandListCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueType> m_QueueType = xiiGALCommandQueueType::Unknown; ///< The command queue type that this command list uses.
};

/// \brief Interface that defines methods to manipulate a command list object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandListCreationDescription& GetDescription() const { return m_Description; };

  /// \brief This returns the command queue for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALCommandQueue> GetCommandQueue() const { return m_pCommandQueue; };

  /// \brief This returns the active pipeline state handle for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALPipelineState> GetPipelineState() const { return m_pPipelineState; };

  /// \brief This returns the active pipeline resource signature handle for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALPipelineResourceSignature> GetPipelineResourceSignature() const { return m_pPipelineResourceSignature; };

  /// \brief This returns the active render pass handle for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALRenderPass> GetRenderPass() const { return m_pRenderPass; };

  /// \brief This returns the active frame buffer handle for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALFramebuffer> GetFramebuffer() const { return m_pFramebuffer; };

public:
  /// \brief Begins the command list for recording commands. This method should be called before any command is issued.
  ///
  /// \remarks This method is called automatically when using xiiGALCommandQueue::BeginCommandList to request a command list.
  void Begin();

  /// \brief Ends the command list. This method should be called after all commands are issued.
  ///
  /// \remarks This method is called automatically when using xiiGALCommandQueue::Submit execute a command list.
  void End();

  /// \brief Resets the command list. This method is used to clear all commands that have been recorded in the command list.
  ///
  /// \remarks This method can be called only if the command list has not yet been submitted for execution.
  void Reset();

  /// \brief Submits a command list to the command queue for execution. The command list is reset after the execution on the command queue.
  ///
  /// \return The current internal fence value.
  xiiUInt64 Submit();

  // State functions.

  /// \brief Sets the pipeline state object for the command list.
  ///
  /// \param hPipelineState - The handle to the pipeline state object.
  void SetPipelineState(xiiSharedPtr<xiiGALPipelineState> pPipelineState);

  /// \brief Sets the stencil reference value used in the stencil test.
  ///
  /// \param uiStencilRef - Stencil reference value.
  void SetStencilRef(xiiUInt32 uiStencilRef);

  /// \brief Sets the blend factors used in the blend state.
  ///
  /// \brief blendFactor - The blend factors represented by a xiiColor.
  void SetBlendFactor(const xiiColor& blendFactor);

  /// \brief Sets the viewports used in the rasterizer stage. This defines the area of the render target to which the rasterizer will clip.
  ///
  /// \param pViewports - The array of viewports structures, describing the viewports to bind.
  void SetViewports(xiiArrayPtr<xiiGALViewport> pViewports);

  /// \brief Sets the scissor rectangles used in the rasterizer stage. This defines the area of the render target to which the rasterizer will clip.
  ///
  /// \param pRects - The array of rectangle structures, describing the scissor rectangles to bind.
  void SetScissorRects(xiiArrayPtr<xiiRectU32> pRects);

  /// \brief Sets the index buffer for the input-assembler stage of the pipeline. This contains the indices into the vertex buffers.
  ///
  /// \param hIndexBuffer - The handle to the index buffer object. The index buffer must be created with the xiiGALBindFlags::IndexBuffer bind flag.
  /// \param uiByteOffset - The byte offset into the index buffer. That is, from the beginning of the buffer to the start of the index data.
  void SetIndexBuffer(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset = 0U);

  /// \brief Sets the vertex buffers for the input-assembler stage of the pipeline. This contains the vertex data.
  ///
  /// \param uiStartSlot    - The starting slot for the vertex buffers, which is the first input slot for binding. The first vertex buffer is explicitly bound to the start slot; each additional vertex buffer in the array is implicitly bound to each subsequent input slot.
  /// \param pVertexBuffers - The array of handles to the vertex buffer objects. The vertex buffers must be created with the xiiGALBindFlags::VertexBuffer bind flag.
  /// \param pByteOffsets   - The array of offset values; one offset value for each buffer in the vertex-buffer array. Each offset is the number of bytes between the first element of a vertex buffer and the first element that will be used. If this parameter is an empty array, zero offsets for all buffers will be used.
  /// \param flags          - Additional flags for setting vertex buffers. See xiiGALSetVertexBufferFlags for more information.
  void SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags = xiiGALSetVertexBufferFlags::None);

  /// \brief This is used to set the constant (uniform) buffer for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param hConstantBuffer    - The handle to the constant (uniform) buffer object to set.
  void SetConstantBuffer(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer);

  /// \brief This is used to set the buffer view for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param hBufferView        - The handle to the buffer view object to set.
  void SetShaderResourceBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView);

  /// \brief This is used to set the texture view for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param hTextureView       - The handle to the texture view object to set.
  void SetShaderResourceTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView);

  /// This is used to set the buffer view for an unordered access.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param hBufferView        - The handle to the buffer view object to set.
  void SetUnorderedAccessBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView);

  /// \brief This is used to set the texture view for an unordered access.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param hTextureView       - The handle to the texture view object to set.
  void SetUnorderedAccessTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView);

  /// \brief This is used to set the sampler for a sampler resource.
  ///
  /// \param bindingInformation - This describes the binding information for the sampler resource, see xiiGALPipelineResourceDescription for details.
  /// \param hSampler           - The handle to the sampler object to set.
  void SetSampler(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler);

  /// \brief This commits the pipeline shader resources to the GPU, and ensures that all necessary state transitions are performed.
  ///
  /// \param mode - The state transition mode. The default is xiiGALStateTransitionMode::Transition.
  xiiResult CommitShaderResources(xiiEnum<xiiGALStateTransitionMode> mode = xiiGALStateTransitionMode::Transition);

  /// \brief This clears the specified render target view to the specified color.
  ///
  /// \param hRenderTargetView - The handle to the render target view object. The view must be a xiiGALTextureViewType::RenderTarget.
  /// \param clearColor        - The color to which to clear the render target view.
  void ClearRenderTargetView(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor);

  /// \brief This clears the specified depth stencil view to the specified depth and stencil values.
  ///
  /// \param hDepthStencilView - The handle to the depth stencil view object. The view must be a xiiGALTextureViewType::DepthStencil.
  /// \param bClearDepth       - Whether to clear the depth portion of the buffer.
  /// \param bClearStencil     - Whether to clear the stencil portion of the buffer.
  /// \param fDepthClear       - The value to which to clear the depth portion of the buffer with.
  /// \param uiStencilClear    - The value to which to clear the stencil portion of the buffer with.
  void ClearDepthStencilView(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  /// \brief This begins a render pass, which contains a collection of attachments, subpasses, and dependencies between the subpasses, and describes how the attachments are used over the course of the subpasses.
  ///
  /// \param beginRenderPass - The description of the render pass. See xiiGALBeginRenderPassDescription for more information.
  void BeginRenderPass(const xiiGALBeginRenderPassDescription& beginRenderPass);

  /// \brief This transitions to the next subpass of the begun render pass.
  void NextSubpass();

  /// \brief This ends a render pass that has already begun.
  void EndRenderPass();

  /// \todo GraphicsFoundation: Add unordered access view clear.

  // Draw functions.

  /// \brief Draws non-indexed primitives.
  ///
  /// \param uiVertexCount - The number of vertices to draw.
  /// \param uiStartVertex - The index of the first vertex to draw.
  xiiResult Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);

  /// \brief Draws indexed primitives.
  ///
  /// \param uiIndexCount - The number of indices to draw.
  /// \param uiStartIndex - The index of the first index to use.
  /// \param uiBaseVertex - A value added to each index before reading a vertex from the vertex buffer.
  xiiResult DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex = 0U);

  /// \brief Draws indexed, instanced primitives.
  ///
  /// \param uiIndexCountPerInstance - The number of indices to draw for each instance.
  /// \param uiInstanceCount         - The number of instances to draw.
  /// \param uiStartIndex            - The index of the first index to use.
  /// \param uiBaseVertex            - A value added to each index before reading a vertex from the vertex buffer.
  /// \param uiFirstInstance         - The index of the first instance to draw.
  xiiResult DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex = 0U, xiiUInt32 uiFirstInstance = 0U);

  /// \brief Draws indexed, instanced primitives using an indirect argument buffer.
  ///
  /// \param hIndirectArgumentBuffer - The handle to the indirect argument buffer object.
  /// \param uiArgumentOffsetInBytes - Byte offset into the indirect argument buffer where the arguments start.
  xiiResult DrawIndexedInstancedIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  /// \brief Draws instanced primitives.
  ///
  /// \param uiVertexCountPerInstance - The number of vertices to draw for each instance.
  /// \param uiInstanceCount          - The number of instances to draw.
  /// \param uiStartVertex            - The index of the first vertex to draw.
  /// \param uiFirstInstance          - The index of the first instance to draw.
  xiiResult DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex = 0U, xiiUInt32 uiFirstInstance = 0U);

  /// \brief Draws instanced primitives using an indirect argument buffer.
  ///
  /// \param hIndirectArgumentBuffer - The handle to the indirect argument buffer object.
  /// \param uiArgumentOffsetInBytes - Byte offset into the indirect argument buffer where the arguments start.
  xiiResult DrawInstancedIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  /// \brief Draws a mesh.
  ///
  /// \param uiThreadGroupCountX - The number of thread groups to dispatch in the X dimension.
  /// \param uiThreadGroupCountY - The number of thread groups to dispatch in the Y dimension.
  /// \param uiThreadGroupCountZ - The number of thread groups to dispatch in the Z dimension.
  xiiResult DrawMesh(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);

  /// \todo GraphicsFoundation: Add indirect mesh draw via DrawIndirect().

  // Dispatch functions.

  /// \brief Dispatches a compute shader.
  ///
  /// \param uiThreadGroupCountX - The number of thread groups to dispatch in the X dimension.
  /// \param uiThreadGroupCountY - The number of thread groups to dispatch in the Y dimension.
  /// \param uiThreadGroupCountZ - The number of thread groups to dispatch in the Z dimension.
  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);

  /// \brief Dispatches a compute shader using an indirect argument buffer.
  ///
  /// \param hIndirectArgumentBuffer - The handle to the indirect argument buffer object.
  /// \param uiArgumentOffsetInBytes - Byte offset into the indirect argument buffer where the arguments start.
  xiiResult DispatchIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Query functions.

  /// \brief Begins a query.
  ///
  /// \param hQuery - The handle to the query object.
  void BeginQuery(xiiSharedPtr<xiiGALQuery> pQuery);

  /// \brief Ends a query.
  ///
  /// \param hQuery - The handle to the query object.
  void EndQuery(xiiSharedPtr<xiiGALQuery> pQuery);

  // Buffer methods.

  /// \brief Updates a buffer.
  ///
  /// \param hBuffer             - The handle to the buffer object.
  /// \param uiDestinationOffset - Byte offset into the buffer where the update should start.
  /// \param pSourceData         - Pointer to the source data.
  void UpdateBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData);

  /// \brief Copies the entire contents of the source buffer to the destination buffer.
  ///
  /// \param hSourceBuffer      - The handle to the source buffer object.
  /// \param hDestinationBuffer - The handle to the destination buffer object.
  void CopyBuffer(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer);

  /// \brief Copies a region from the source buffer to the destination buffer.
  ///
  /// \param hSourceBuffer       - The handle to the source buffer object.
  /// \param uiSourceOffset      - Byte offset into the source buffer where the copy should start.
  /// \param hDestinationBuffer  - The handle to the destination buffer object.
  /// \param uiDestinationOffset - Byte offset into the destination buffer where the copy should start.
  /// \param uiSize              - Size in bytes of the region to copy.
  void CopyBufferRegion(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize);

  /// \brief Maps a buffer into the CPU's address space.
  ///
  /// \param hBuffer     - The handle to the buffer object.
  /// \param mapType     - Specifies the CPU's access pattern for the map operation. See xiiGALMapType for details.
  /// \param mapFlags    - Flags specifying how the buffer should be mapped. See xiiGALMapFlags for details.
  /// \param pMappedData - Pointer to the mapped data.
  xiiResult MapBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData);

  /// \brief Unmaps a buffer from the CPU's address space.
  ///
  /// \param hBuffer - The handle to the buffer object.
  /// \param mapType - Specifies the CPU's access pattern for the map operation. See xiiGALMapType for details.
  xiiResult UnmapBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType);

  // Texture methods.

  /// \brief Updates a texture.
  ///
  /// \param hTexture            - The handle to the texture object.
  /// \param textureMiplevelData - Specifies the subresource to update. See xiiGALTextureMipLevelData for details.
  /// \param textureBox          - Specifies the region within the subresource to update.
  /// \param subresourceData     - Specifies the new data. See xiiGALTextureSubResourceData for details.
  void UpdateTexture(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData);

  /// \brief Copies the entire contents of the source texture to the destination texture.
  ///
  /// \param hSourceTexture      - The handle to the source texture object.
  /// \param hDestinationTexture - The handle to the destination texture object.
  void CopyTexture(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture);

  /// \brief Copies a region from the source texture to the destination texture.
  ///
  /// \param hSourceTexture          - The handle to the source texture object.
  /// \param sourceMipLevelData      - Specifies the subresource in the source texture. See xiiGALTextureMipLevelData for details.
  /// \param box                     - Specifies the region within the source subresource to copy.
  /// \param hDestinationTexture     - The handle to the destination texture object.
  /// \param destinationMipLevelData - Specifies the subresource in the destination texture. See xiiGALTextureMipLevelData for details.
  /// \param vDestinationPoint       - Specifies the point within the destination subresource where the region should be copied to.
  void CopyTextureRegion(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint);

  /// \brief Resolves a multisampled source texture into a non-multisampled destination texture.
  ///
  /// \param hSourceTexture          - The handle to the source texture object.
  /// \param sourceMipLevelData      - Specifies the subresource in the source texture. See xiiGALTextureMipLevelData for details.
  /// \param hDestinationTexture     - The handle to the destination texture object.
  /// \param destinationMipLevelData - Specifies the subresource in the destination texture. See xiiGALTextureMipLevelData for details.
  void ResolveTextureSubResource(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData);

  /// \brief Generates mipmap levels for a texture.
  ///
  /// \param hTextureView - The handle to the texture view object. The texture view must be of type xiiGALTextureViewType::ShaderResource.
  ///
  /// \remarks This method must only be called on a shader resource view. The texture must be created with xiiGALMiscTextureFlags::GenerateMips.
  void GenerateMips(xiiSharedPtr<xiiGALTextureView> pTextureView);

  /// \brief Maps a texture subresource into the address space of the command list.
  ///
  /// \param hTexture            - The handle to the texture object. This is the texture that contains the subresource to map.
  /// \param textureMipLevelData - Specifies the subresource to map. This is the mipmap level of the texture to map.
  /// \param mapType             - Specifies the CPU's read and write access to a resource.
  /// \param mapFlags            - Specifies the behavior of the map operation.
  /// \param pTextureBox         - Specifies the region of the resource to map. If this parameter is null, the entire resource is mapped.
  /// \param mappedData          - Receives information about the resource data when the function returns.
  xiiResult MapTextureSubresource(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData);

  /// \brief Unmaps a texture subresource from the address space of the command list.
  ///
  /// \param hTexture            - The handle to the texture object.
  /// \param textureMipLevelData - Specifies the subresource to unmap.
  xiiResult UnmapTextureSubresource(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData);

  // Debug functions.

  /// \brief Begins a new debug group with a specified name and color.
  ///
  /// \param sName - The name of the debug group.
  /// \param color - The color associated with the debug group.
  void BeginDebugGroup(xiiStringView sName, const xiiColor& color = xiiColor::Black);

  /// \brief Ends the current debug group.
  void EndDebugGroup();

  /// \brief Inserts a debug label into the command list.
  ///
  /// \param sName - The name of the debug label.
  /// \param color - The color associated with the debug label.
  void InsertDebugLabel(xiiStringView sName, const xiiColor& color = xiiColor::Black);

  /// \brief Invalidates the current state of the command list. It is typically called when the command list is reset or when the pipeline state is changed.
  void InvalidateState();

public:
  /// \brief Enum class representing the state of a command list recording.
  enum class RecordingState
  {
    Recording, ///< The command list is currently being recorded.
    Ended,     ///< The recording of the command list has ended.
    Reset,     ///< The command list has been reset and is ready to be recorded again.
    Submitted  ///< The command list has been submitted and is no longer available for recording commands. A new command list has to be requested for recording more commands.
  };

  XII_ALWAYS_INLINE void AssertRenderingThread() const { XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function may only be executed on the main thread."); };

  XII_ALWAYS_INLINE RecordingState GetRecordingState() const { return m_RecordingState; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALCommandList(xiiSharedPtr<xiiGALDevice> pDevice, xiiSharedPtr<xiiGALCommandQueue> pCommandQueue, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandList();

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  void ValidateTextureRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& box);
  void ValidateTextureUpdateRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox, const xiiGALTextureSubResourceData& subresourceData);

  // These functions need to be implemented by a graphics API abstraction.
protected:
  virtual void BeginPlatform() = 0;
  virtual void EndPlatform()   = 0;
  virtual void ResetPlatform() = 0;

  virtual xiiUInt64 SubmitPlatform() = 0;

  virtual void SetPipelineStatePlatform(xiiSharedPtr<xiiGALPipelineState> pPipelineState) = 0;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef)       = 0;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) = 0;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports) = 0;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)      = 0;

  virtual void      SetIndexBufferPlatform(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset)                                                                                        = 0;
  virtual void      SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags) = 0;
  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)                                             = 0;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)                                   = 0;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)                                = 0;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)                                  = 0;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)                               = 0;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler)                                                          = 0;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)                                                                                                         = 0;

  virtual void ClearRenderTargetViewPlatform(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor)                                                       = 0;
  virtual void ClearDepthStencilViewPlatform(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual void BeginRenderPassPlatform(xiiSharedPtr<xiiGALRenderPass> pRenderPass, xiiSharedPtr<xiiGALFramebuffer> pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) = 0;
  virtual void NextSubpassPlatform()                                                                                                                                                                 = 0;
  virtual void EndRenderPassPlatform()                                                                                                                                                               = 0;

  virtual xiiResult DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)                                                                                                        = 0;
  virtual xiiResult DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)                                                                           = 0;
  virtual xiiResult DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance) = 0;
  virtual xiiResult DrawIndexedInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)                                           = 0;
  virtual xiiResult DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)                              = 0;
  virtual xiiResult DrawInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)                                                  = 0;
  virtual xiiResult DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)                                                         = 0;

  virtual xiiResult DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)   = 0;
  virtual xiiResult DispatchIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) = 0;

  virtual void BeginQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery) = 0;
  virtual void EndQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery)   = 0;

  virtual void      UpdateBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)                                                             = 0;
  virtual void      CopyBufferPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer)                                                                                  = 0;
  virtual void      CopyBufferRegionPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize) = 0;
  virtual xiiResult MapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)                                              = 0;
  virtual xiiResult UnmapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType)                                                                                                      = 0;

  virtual void      UpdateTexturePlatform(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)                                                                                              = 0;
  virtual void      CopyTexturePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture)                                                                                                                                                                                         = 0;
  virtual void      CopyTextureRegionPlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) = 0;
  virtual void      ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)                                                            = 0;
  virtual void      GenerateMipsPlatform(xiiSharedPtr<xiiGALTextureView> pTextureView)                                                                                                                                                                                                                                       = 0;
  virtual xiiResult MapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)                                     = 0;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData)                                                                                                                                                                                     = 0;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)  = 0;
  virtual void EndDebugGroupPlatform()                                              = 0;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) = 0;

  virtual void InvalidateStatePlatform() = 0;

  /// \endcond

protected:
  xiiGALCommandListCreationDescription m_Description;

  xiiSharedPtr<xiiGALCommandQueue> m_pCommandQueue;

  RecordingState m_RecordingState = RecordingState::Reset;

  xiiSharedPtr<xiiGALPipelineState>             m_pPipelineState;
  xiiSharedPtr<xiiGALPipelineResourceSignature> m_pPipelineResourceSignature;

  xiiSharedPtr<xiiGALBuffer> m_VertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  xiiSharedPtr<xiiGALBuffer> m_pIndexBuffer;
  xiiUInt64                  m_uiIndexDataOffset = 0ULL;

  xiiSharedPtr<xiiGALRenderPass>  m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;

  xiiColor  m_BlendFactors = xiiColor::Black;
  xiiUInt32 m_uiStencilRef = 0U;

  xiiHybridArray<xiiGALViewport, 2U> m_Viewports;
  xiiHybridArray<xiiRectU32, 2U>     m_ScissorRects;

private:
  XII_ALWAYS_INLINE void CountDispatchCall() { ++m_uiDispatchCalls; };
  XII_ALWAYS_INLINE void CountDrawCall() { ++m_uiDrawCalls; };
  XII_ALWAYS_INLINE void ClearStatisticCounters()
  {
    m_uiDrawCalls     = 0;
    m_uiDispatchCalls = 0;
  };

  // Statistic variables.
  xiiUInt32 m_uiDrawCalls     = 0U;
  xiiUInt32 m_uiDispatchCalls = 0U;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 m_uiDebugGroupCount = 0;

  xiiMap<xiiGALBuffer*, xiiEnum<xiiGALMapType>> m_MappedBuffers;
#endif
};
