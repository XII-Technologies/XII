#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandEncoderD3D12 final : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderGraphicsPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12);
  ~xiiGALCommandEncoderD3D12();

  //  xiiGALCommandEncoderCommonPlatformInterface

  // State setting functions

  virtual void SetShaderPlatform(const xiiGALShader* pShader) override;
  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer) override;
  virtual void SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALSampler* pSampler) override;
  virtual void SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALBufferView* pBufferView) override;
  virtual void SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALTextureView* pRTextureView) override;
  virtual void SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, const xiiGALBufferView* pUnorderedAccessBufferView) override;
  virtual void SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, const xiiGALTextureView* pUnorderedAccessTextureView) override;

  // Query functions

  virtual void      BeginQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual void      EndQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual xiiResult GetQueryResultPlatform(const xiiGALQuery* pQuery, void* pData) override;

  // Fence functions

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const xiiGALBufferView* pBufferView, xiiVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALTextureView* pTextureView, xiiVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues) override;

  virtual void CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) override;
  virtual void UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags) override;

  virtual void CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiVec3U32& vDestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubResourceData& sourceSubResource, const xiiBoundingBoxu32& box) override;
  virtual void UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureData& sourceData) override;
  virtual void ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubResourceData& sourceSubResource) override;
  virtual void ReadbackTexturePlatform(const xiiGALTexture* pTexture, const xiiGALTexture* pStagingTexture) override;
  virtual void CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, const xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureSubResourceData> sourceSubResource, xiiArrayPtr<xiiGALTextureData> targetData) override;
  virtual void GenerateMipMapsPlatform(const xiiGALTextureView* pTextureView) override;

  // Miscellaneous

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(xiiStringView sMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(xiiStringView sMarker) override;

  // xiiGALCommandEncoderGraphicsPlatformInterface

  // Draw functions

  virtual void ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

  virtual void BeginStreamOutPlatform() override;
  virtual void EndStreamOutPlatform() override;

  // State functions

  virtual void SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer) override;
  virtual void SetInputLayoutPlatform(const xiiGALInputLayout* pInputLayout) override;
  virtual void SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology) override;

  virtual void SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset) override;

  // xiiGALCommandEncoderComputePlatformInterface

  // Dispatch

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

private:
  friend class xiiGALPassD3D12;

  xiiGALDeviceD3D12&    m_GALDeviceD3D12;
  xiiGALCommandEncoder* m_pOwner = nullptr;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandEncoderD3D12_inl.h>
