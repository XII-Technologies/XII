#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandEncoder);

public:
  // State setting functions

  void SetShader(xiiGALShaderHandle hShader);

  void SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer);
  void SetSamplerStatePlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALSampler* pSamplerState);
  void SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALBufferView* pBufferView);
  void SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALTextureView* pRTextureView);

  bool UnsetResourceViews(const xiiGALResourceBase* pResource);
  bool UnsetUnorderedAccessViews(const xiiGALResourceBase* pResource);

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

  void ReadbackTexture(xiiGALTextureHandle hTexture);

  void CopyTextureReadbackResult(xiiGALTextureHandle hTexture, xiiArrayPtr<xiiGALTextureSubResourceData> sourceSubResource, xiiArrayPtr<xiiGALTextureData> targetData);

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

  xiiGALCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderState& state, xiiGALCommandEncoderCommonPlatformInterface& commonImpl);

  virtual ~xiiGALCommandEncoder();

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
