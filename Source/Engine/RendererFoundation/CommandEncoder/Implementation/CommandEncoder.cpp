#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void xiiGALCommandEncoder::SetShader(xiiGALShaderHandle hShader)
{
  AssertRenderingThread();
  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALShader* pShader = m_Device.GetShader(hShader);
  XII_ASSERT_DEV(pShader != nullptr, "The given shader handle isn't valid, this may be a use after destroy!");

  m_CommonImpl.SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;
  CountStateChange();
}

void xiiGALCommandEncoder::SetConstantBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer)
{
  AssertRenderingThread();
  XII_ASSERT_RELEASE(uiSlot < XII_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index too big!");

  if (m_State.m_hConstantBuffers[uiSlot] == hBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  XII_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferType == xiiGALBufferType::ConstantBuffer, "Wrong buffer type");

  m_CommonImpl.SetConstantBufferPlatform(uiSlot, pBuffer);

  m_State.m_hConstantBuffers[uiSlot] = hBuffer;

  CountStateChange();
}

void xiiGALCommandEncoder::SetSamplerState(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, xiiGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();
  XII_ASSERT_RELEASE(uiSlot < XII_GAL_MAX_SAMPLER_COUNT, "Sampler state slot index too big!");

  if (m_State.m_hSamplerStates[Stage][uiSlot] == hSamplerState)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALSamplerState* pSamplerState = m_Device.GetSamplerState(hSamplerState);

  m_CommonImpl.SetSamplerStatePlatform(Stage, uiSlot, pSamplerState);

  m_State.m_hSamplerStates[Stage][uiSlot] = hSamplerState;

  CountStateChange();
}

void xiiGALCommandEncoder::SetResourceView(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, xiiGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  auto& boundResourceViews = m_State.m_hResourceViews[Stage];
  if (uiSlot < boundResourceViews.GetCount() && boundResourceViews[uiSlot] == hResourceView)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    if (UnsetUnorderedAccessViews(pResourceView->GetResource()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetResourceViewPlatform(Stage, uiSlot, pResourceView);

  boundResourceViews.EnsureCount(uiSlot + 1);
  boundResourceViews[uiSlot] = hResourceView;

  auto& boundResources = m_State.m_pResourcesForResourceViews[Stage];
  boundResources.EnsureCount(uiSlot + 1);
  boundResources[uiSlot] = pResourceView != nullptr ? pResourceView->GetResource()->GetParentResource() : nullptr;

  CountStateChange();
}

void xiiGALCommandEncoder::SetUnorderedAccessView(xiiUInt32 uiSlot, xiiGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (uiSlot < m_State.m_hUnorderedAccessViews.GetCount() && m_State.m_hUnorderedAccessViews[uiSlot] == hUnorderedAccessView)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView != nullptr)
  {
    if (UnsetResourceViews(pUnorderedAccessView->GetResource()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetUnorderedAccessViewPlatform(uiSlot, pUnorderedAccessView);

  m_State.m_hUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_hUnorderedAccessViews[uiSlot] = hUnorderedAccessView;

  m_State.m_pResourcesForUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource()->GetParentResource() : nullptr;

  CountStateChange();
}

bool xiiGALCommandEncoder::UnsetResourceViews(const xiiGALResourceBase* pResource)
{
  XII_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_pResourcesForResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_State.m_pResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        m_CommonImpl.SetResourceViewPlatform((xiiGALShaderStage::Enum)stage, uiSlot, nullptr);

        m_State.m_hResourceViews[stage][uiSlot].Invalidate();
        m_State.m_pResourcesForResourceViews[stage][uiSlot] = nullptr;

        bResult = true;
      }
    }
  }

  return bResult;
}

bool xiiGALCommandEncoder::UnsetUnorderedAccessViews(const xiiGALResourceBase* pResource)
{
  XII_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_pResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_State.m_pResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_CommonImpl.SetUnorderedAccessViewPlatform(uiSlot, nullptr);

      m_State.m_hUnorderedAccessViews[uiSlot].Invalidate();
      m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = nullptr;

      bResult = true;
    }
  }

  return bResult;
}

void xiiGALCommandEncoder::BeginQuery(xiiGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  XII_ASSERT_DEV(!query->m_bStarted, "Can't stat xiiGALQuery because it is already running.");

  m_CommonImpl.BeginQueryPlatform(query);
}

void xiiGALCommandEncoder::EndQuery(xiiGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  XII_ASSERT_DEV(query->m_bStarted, "Can't end xiiGALQuery, query hasn't started yet.");

  m_CommonImpl.EndQueryPlatform(query);
}

xiiResult xiiGALCommandEncoder::GetQueryResult(xiiGALQueryHandle hQuery, xiiUInt64& uiQueryResult)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  XII_ASSERT_DEV(!query->m_bStarted, "Can't retrieve data from xiiGALQuery while query is still running.");

  return m_CommonImpl.GetQueryResultPlatform(query, uiQueryResult);
}

xiiGALTimestampHandle xiiGALCommandEncoder::InsertTimestamp()
{
  xiiGALTimestampHandle hTimestamp = m_Device.GetTimestamp();

  m_CommonImpl.InsertTimestampPlatform(hTimestamp);

  return hTimestamp;
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView, xiiVec4 clearValues)
{
  AssertRenderingThread();

  const xiiGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, clearValues);
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView, xiiVec4U32 clearValues)
{
  AssertRenderingThread();

  const xiiGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, clearValues);
}

void xiiGALCommandEncoder::CopyBuffer(xiiGALBufferHandle hDest, xiiGALBufferHandle hSource)
{
  AssertRenderingThread();

  const xiiGALBuffer* pDest   = m_Device.GetBuffer(hDest);
  const xiiGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    XII_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", xiiArgP(pDest), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::CopyBufferRegion(
  xiiGALBufferHandle hDest,
  xiiUInt32          uiDestOffset,
  xiiGALBufferHandle hSource,
  xiiUInt32          uiSourceOffset,
  xiiUInt32          uiByteCount)
{
  AssertRenderingThread();

  const xiiGALBuffer* pDest   = m_Device.GetBuffer(hDest);
  const xiiGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const xiiUInt32 uiDestSize   = pDest->GetSize();
    const xiiUInt32 uiSourceSize = pSource->GetSize();

    XII_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    XII_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    XII_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", xiiArgP(pDest), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::UpdateBuffer(
  xiiGALBufferHandle          hDest,
  xiiUInt32                   uiDestOffset,
  xiiArrayPtr<const xiiUInt8> pSourceData,
  xiiGALUpdateMode::Enum      updateMode)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(!pSourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const xiiGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    XII_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + pSourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, pSourceData.GetCount());
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, pSourceData, updateMode);
  }
  else
  {
    XII_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void xiiGALCommandEncoder::CopyTexture(xiiGALTextureHandle hDest, xiiGALTextureHandle hSource)
{
  AssertRenderingThread();

  const xiiGALTexture* pDest   = m_Device.GetTexture(hDest);
  const xiiGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    XII_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", xiiArgP(pDest), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::CopyTextureRegion(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, xiiGALTextureHandle hSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  AssertRenderingThread();

  const xiiGALTexture* pDest   = m_Device.GetTexture(hDest);
  const xiiGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDest, DestinationSubResource, DestinationPoint, pSource, SourceSubResource, Box);
  }
  else
  {
    XII_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", xiiArgP(pDest), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::UpdateTexture(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData)
{
  AssertRenderingThread();

  const xiiGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDest, DestinationSubResource, DestinationBox, pSourceData);
  }
  else
  {
    XII_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", xiiArgP(pDest));
  }
}

void xiiGALCommandEncoder::ResolveTexture(xiiGALTextureHandle hDest, const xiiGALTextureSubresource& DestinationSubResource, xiiGALTextureHandle hSource, const xiiGALTextureSubresource& SourceSubResource)
{
  AssertRenderingThread();

  const xiiGALTexture* pDest   = m_Device.GetTexture(hDest);
  const xiiGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDest, DestinationSubResource, pSource, SourceSubResource);
  }
  else
  {
    XII_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", xiiArgP(pDest), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::ReadbackTexture(xiiGALTextureHandle hTexture)
{
  AssertRenderingThread();

  const xiiGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    XII_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
                       "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.ReadbackTexturePlatform(pTexture);
  }
}

void xiiGALCommandEncoder::CopyTextureReadbackResult(xiiGALTextureHandle hTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData)
{
  AssertRenderingThread();

  const xiiGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    XII_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
                       "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.CopyTextureReadbackResultPlatform(pTexture, SourceSubResource, TargetData);
  }
}

void xiiGALCommandEncoder::GenerateMipMaps(xiiGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const xiiGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    XII_ASSERT_DEV(!pResourceView->GetDescription().m_hTexture.IsInvalidated(), "Resource view needs a valid texture to generate mip maps.");
    const xiiGALTexture* pTexture = m_Device.GetTexture(pResourceView->GetDescription().m_hTexture);
    XII_ASSERT_DEV(pTexture->GetDescription().m_bAllowDynamicMipGeneration,
                   "Dynamic mip map generation needs to be enabled (m_bAllowDynamicMipGeneration = true)!");

    m_CommonImpl.GenerateMipMapsPlatform(pResourceView);
  }
}

void xiiGALCommandEncoder::Flush()
{
  AssertRenderingThread();

  m_CommonImpl.FlushPlatform();
}

// Debug helper functions

void xiiGALCommandEncoder::PushMarker(const char* Marker)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(Marker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(Marker);
}

void xiiGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void xiiGALCommandEncoder::InsertEventMarker(const char* Marker)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(Marker != nullptr, "Invalid marker!");

  m_CommonImpl.InsertEventMarkerPlatform(Marker);
}

void xiiGALCommandEncoder::ClearStatisticsCounters()
{
  // Reset counters for various statistics
  m_uiStateChanges          = 0;
  m_uiRedundantStateChanges = 0;
}

xiiGALCommandEncoder::xiiGALCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderState& state, xiiGALCommandEncoderCommonPlatformInterface& commonImpl) :
  m_Device(device), m_State(state), m_CommonImpl(commonImpl)
{
}

xiiGALCommandEncoder::~xiiGALCommandEncoder() = default;

void xiiGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_CommandEncoder);
