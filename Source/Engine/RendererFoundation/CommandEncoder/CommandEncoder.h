
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

class XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandEncoder);

public:
  // State setting functions

  void SetShader(xiiGALShaderHandle hShader);

  void SetConstantBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer);
  void SetSamplerState(xiiGALShaderStage::Enum stage, xiiUInt32 uiSlot, xiiGALSamplerStateHandle hSamplerState);
  void SetResourceView(xiiGALShaderStage::Enum stage, xiiUInt32 uiSlot, xiiGALResourceViewHandle hResourceView);
  void SetUnorderedAccessView(xiiUInt32 uiSlot, xiiGALUnorderedAccessViewHandle hUnorderedAccessView);

  // Returns whether a resource view has been unset for the given resource
  bool UnsetResourceViews(const xiiGALResourceBase* pResource);
  // Returns whether a unordered access view has been unset for the given resource
  bool UnsetUnorderedAccessViews(const xiiGALResourceBase* pResource);

  // Query functions

  void BeginQuery(xiiGALQueryHandle hQuery);
  void EndQuery(xiiGALQueryHandle hQuery);

  /// \return Success if retrieving the query succeeded.
  xiiResult GetQueryResult(xiiGALQueryHandle hQuery, xiiUInt64& ref_uiQueryResult);

  // Timestamp functions

  xiiGALTimestampHandle InsertTimestamp();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView, xiiVec4 vClearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView, xiiVec4U32 vClearValues);

  void CopyBuffer(xiiGALBufferHandle hDest, xiiGALBufferHandle hSource);
  void CopyBufferRegion(xiiGALBufferHandle hDest, xiiUInt32 uiDestOffset, xiiGALBufferHandle hSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount);
  void UpdateBuffer(xiiGALBufferHandle hDest, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiGALUpdateMode::Enum updateMode = xiiGALUpdateMode::Discard);

  void CopyTexture(xiiGALTextureHandle hDest, xiiGALTextureHandle hSource);
  void CopyTextureRegion(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTextureHandle hSource, const xiiGALTextureSubresource& sourceSubResource, const xiiBoundingBoxu32& box);

  void UpdateTexture(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALSystemMemoryDescription& sourceData);

  void ResolveTexture(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& destinationSubResource, xiiGALTextureHandle hSource, const xiiGALTextureSubresource& sourceSubResource);

  void ReadbackTexture(xiiGALTextureHandle hTexture);
  void CopyTextureReadbackResult(xiiGALTextureHandle hTexture, xiiArrayPtr<xiiGALTextureSubresource> sourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> targetData);

  void GenerateMipMaps(xiiGALResourceViewHandle hResourceView);

  // Misc

  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  virtual void ClearStatisticsCounters();

  XII_ALWAYS_INLINE xiiGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class xiiGALDevice;

  xiiGALCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderState& state, xiiGALCommandEncoderCommonPlatformInterface& commonImpl);
  virtual ~xiiGALCommandEncoder();


  void AssertRenderingThread()
  {
    XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

  void CountStateChange() { m_uiStateChanges++; }
  void CountRedundantStateChange() { m_uiRedundantStateChanges++; }

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
