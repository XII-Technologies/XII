
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class xiiGALBlendStateVulkan;
class xiiGALBufferVulkan;
class xiiGALDepthStencilStateVulkan;
class xiiGALRasterizerStateVulkan;
class xiiGALResourceViewVulkan;
class xiiGALSamplerStateVulkan;
class xiiGALShaderVulkan;
class xiiGALUnorderedAccessViewVulkan;
class xiiGALDeviceVulkan;

class XII_RENDERERVULKAN_DLL xiiGALCommandEncoderImplVulkan : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderRenderPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderImplVulkan(xiiGALDeviceVulkan& device);
  ~xiiGALCommandEncoderImplVulkan();

  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, xiiPipelineBarrierVulkan* pipelineBarrier);

  // xiiGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const xiiGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void      BeginQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual void      EndQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual xiiResult GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const xiiGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData) override;

  virtual void GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView) override;

  void CopyImageToBuffer(const xiiGALTextureVulkan* pSource, const xiiGALBufferVulkan* pDestination);

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  // xiiGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const xiiGALRenderingSetup& renderingSetup);
  void EndRendering();

  // Draw functions

  virtual void ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;
  virtual void EndStreamOutPlatform() override;

  // State functions

  virtual void SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer) override;
  virtual void SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset) override;


  // xiiGALCommandEncoderComputePlatformInterface
  // Dispatch
  void BeginCompute();
  void EndCompute();

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

private:
  void FlushDeferredStateChanges();

  xiiGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device          m_vkDevice;

  vk::CommandBuffer*        m_pCommandBuffer   = nullptr;
  xiiPipelineBarrierVulkan* m_pPipelineBarrier = nullptr;


  // Cache flags.
  bool                  m_bPipelineStateDirty = true;
  bool                  m_bViewportDirty      = true;
  bool                  m_bIndexBufferDirty   = false;
  bool                  m_bDescriptorsDirty   = false;
  xiiGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool                  m_bRenderPassActive = false; ///< #TODO_VULKAN Disabling and re-enabling the render pass is buggy as we might execute a clear twice.
  bool                  m_bClearSubmitted   = false; ///< Start render pass is lazy so if no draw call is executed we need to make sure the clear is executed anyways.
  bool                  m_bInsideCompute    = false; ///< Within BeginCompute / EndCompute block.


  // Bound objects for deferred state flushes
  xiiResourceCacheVulkan::PipelineLayoutDesc                         m_LayoutDesc;
  xiiResourceCacheVulkan::GraphicsPipelineDesc                       m_PipelineDesc;
  xiiResourceCacheVulkan::ComputePipelineDesc                        m_ComputeDesc;
  vk::Framebuffer                                                    m_frameBuffer;
  vk::RenderPassBeginInfo                                            m_renderPass;
  xiiHybridArray<vk::ClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
  vk::ImageAspectFlags                                               m_depthMask = {};
  xiiUInt32                                                          m_uiLayers  = 0;

  vk::Viewport m_viewport;
  vk::Rect2D   m_scissor;
  bool         m_bScissorEnabled = false;

  const xiiGALRenderTargetView* m_pBoundRenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  const xiiGALRenderTargetView* m_pBoundDepthStencilTarget                            = nullptr;
  xiiUInt32                     m_uiBoundRenderTargetCount;

  const xiiGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer                m_pBoundVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize            m_VertexBufferOffsets[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  const xiiGALBufferVulkan*                                  m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiHybridArray<const xiiGALResourceViewVulkan*, 16>        m_pBoundShaderResourceViews[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiHybridArray<const xiiGALUnorderedAccessViewVulkan*, 16> m_pBoundUnoderedAccessViews;
  const xiiGALSamplerStateVulkan*                            m_pBoundSamplerStates[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};

  xiiHybridArray<vk::WriteDescriptorSet, 16> m_DescriptorWrites;
};
