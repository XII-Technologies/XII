#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandEncoder);

protected:
  xiiGALCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderState& state, xiiGALCommandEncoderCommonPlatformInterface& commonImpl);

  virtual ~xiiGALCommandEncoder();

public:
  // State setting functions

  void SetShader(xiiGALShaderHandle hShader);
  void SetConstantBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer);
  void SetSampler(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSamplerHandle hSampler);
  void SetBufferView(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferViewHandle hBufferView);
  void SetTextureView(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureViewHandle hTextureView);
  void SetUnorderedAccessBufferView(xiiUInt32 uiSlot, xiiGALBufferViewHandle hUnorderedAccessBufferView);
  void SetUnorderedAccessTextureView(xiiUInt32 uiSlot, xiiGALTextureViewHandle hUnorderedAccessTextureView);

  bool UnsetBufferView(const xiiGALBuffer* pBuffer);
  bool UnsetTextureView(const xiiGALTexture* pTexture);
  bool UnsetUnorderedAccessBufferView(const xiiGALBuffer* pBuffer);
  bool UnsetUnorderedAccessTextureView(const xiiGALTexture* pTexture);

  // Query functions

  void      BeginQuery(xiiGALQueryHandle hQuery);
  void      EndQuery(xiiGALQueryHandle hQuery);
  xiiResult GetQueryResult(xiiGALQueryHandle hQuery, void* pData);

  // Fence functions

  // Resource functions

  void ClearUnorderedAccessView(xiiGALBufferViewHandle hBufferView, xiiVec4 vClearValues);
  void ClearUnorderedAccessView(xiiGALTextureViewHandle hTextureView, xiiVec4 vClearValues);
  void ClearUnorderedAccessView(xiiGALBufferViewHandle hBufferView, xiiVec4U32 vClearValues);
  void ClearUnorderedAccessView(xiiGALTextureViewHandle hTextureView, xiiVec4U32 vClearValues);

  void CopyBuffer(xiiGALBufferHandle hDestination, xiiGALBufferHandle hSource);
  void CopyBufferRegion(xiiGALBufferHandle hDestination, xiiUInt32 uiDestinationOffset, xiiGALBufferHandle hSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount);
  void UpdateBuffer(xiiGALBufferHandle hDestination, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard);

  void CopyTexture(xiiGALTextureHandle hDestination, xiiGALTextureHandle hSource);
  void CopyTextureRegion(xiiGALTextureHandle hDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTextureHandle hSource, const xiiGALTextureSubResourceData& sourceSubResource, const xiiBoundingBoxu32& box);
  void UpdateTexture(xiiGALTextureHandle hDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureData& sourceData);
  void ResolveTexture(xiiGALTextureHandle hDestination, const xiiGALTextureSubResourceData& destinationSubResource, xiiGALTextureHandle hSource, const xiiGALTextureSubResourceData& sourceSubResource);
  void ReadbackTexture(xiiGALTextureHandle hTexture, xiiGALTextureHandle hStagingTexture);
  void CopyTextureReadbackResult(xiiGALTextureHandle hTexture, xiiGALTextureHandle hStagingTexture, xiiArrayPtr<xiiGALTextureSubResourceData> sourceSubResource, xiiArrayPtr<xiiGALTextureData> targetData);
  void GenerateMipMaps(xiiGALTextureViewHandle hTextureView);

  // Miscellaneous

  void Flush();

  // Debug helper functions

  void PushMarker(xiiStringView sMarker);
  void PopMarker();
  void InsertEventMarker(xiiStringView sMarker);

public:
  xiiGALDevice& GetDevice();

  virtual void ClearStatisticsCounters();

  void InvalidateState();

protected:
  friend class xiiGALDevice;

  void AssertRenderingThread() const;

  void CountStateChange();

  void CountRedundantStateChange();

private:
  friend class xiiMemoryUtils;

  // Parent Device
  xiiGALDevice& m_Device;

  // Statistic variables
  xiiUInt32 m_uiStateChanges          = 0;
  xiiUInt32 m_uiRedundantStateChanges = 0;

  xiiGALCommandEncoderState& m_State;

  xiiGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandEncoder_inl.h>
