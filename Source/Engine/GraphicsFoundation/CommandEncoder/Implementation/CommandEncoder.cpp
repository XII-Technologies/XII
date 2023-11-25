#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

xiiGALCommandEncoder::xiiGALCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderState& state, xiiGALCommandEncoderCommonPlatformInterface& commonImpl) :
  m_Device(device), m_State(state), m_CommonImpl(commonImpl)
{
}

xiiGALCommandEncoder::~xiiGALCommandEncoder() = default;

void xiiGALCommandEncoder::SetShader(xiiGALShaderHandle hShader)
{
  AssertRenderingThread();

  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALShader* pShader = m_Device.GetShader(hShader);
  XII_ASSERT_DEV(pShader != nullptr, "The given shader handle is invalid, this may be a use after destroy!");

  m_CommonImpl.SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;

  CountStateChange();
}

void xiiGALCommandEncoder::SetConstantBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer)
{
  AssertRenderingThread();

  XII_ASSERT_RELEASE(uiSlot < XII_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index is out of range!");

  if (m_State.m_hConstantBuffers[uiSlot] == hBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  XII_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer), "Expected xiiGALBindFlags::UniformBuffer bind flag on buffer.");

  m_CommonImpl.SetConstantBufferPlatform(uiSlot, pBuffer);

  m_State.m_hConstantBuffers[uiSlot] = hBuffer;

  CountStateChange();
}

void xiiGALCommandEncoder::SetSampler(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSamplerHandle hSampler)
{
  AssertRenderingThread();

  XII_ASSERT_RELEASE(uiSlot < XII_GAL_MAX_SAMPLER_COUNT, "Sampler state slot index is out of range!");

  if (m_State.m_hSamplers[xiiGALShaderStage::GetStageIndex(stage)][uiSlot] == hSampler)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALSampler* pSampler = m_Device.GetSampler(hSampler);

  m_CommonImpl.SetSamplerPlatform(stage, uiSlot, pSampler);

  m_State.m_hSamplers[xiiGALShaderStage::GetStageIndex(stage)][uiSlot] = hSampler;

  CountStateChange();
}

void xiiGALCommandEncoder::SetBufferView(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferViewHandle hBufferView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  auto& boundResourceViews = m_State.m_hResourceViews[xiiGALShaderStage::GetStageIndex(stage)];
  if (uiSlot < boundResourceViews.GetCount() && boundResourceViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Buffer && boundResourceViews[uiSlot].m_hBufferView == hBufferView)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALBufferView* pBufferView = m_Device.GetBufferView(hBufferView);
  if (pBufferView != nullptr)
  {
    if (UnsetUnorderedAccessBufferView(pBufferView->GetBuffer()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetBufferViewPlatform(stage, uiSlot, pBufferView);

  boundResourceViews.EnsureCount(uiSlot + 1);
  boundResourceViews[uiSlot] = {
    .m_Type        = xiiGALCommandEncoderState::ResourceBinding::Buffer,
    .m_hBufferView = hBufferView,
    .m_pBuffer     = pBufferView != nullptr ? pBufferView->GetBuffer() : nullptr,
  };

  CountStateChange();
}

void xiiGALCommandEncoder::SetTextureView(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureViewHandle hTextureView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  auto& boundResourceViews = m_State.m_hResourceViews[xiiGALShaderStage::GetStageIndex(stage)];
  if (uiSlot < boundResourceViews.GetCount() && boundResourceViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Texture && boundResourceViews[uiSlot].m_hTextureView == hTextureView)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALTextureView* pTextureView = m_Device.GetTextureView(hTextureView);
  if (pTextureView != nullptr)
  {
    if (UnsetUnorderedAccessTextureView(pTextureView->GetTexture()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetTextureViewPlatform(stage, uiSlot, pTextureView);

  boundResourceViews.EnsureCount(uiSlot + 1);
  boundResourceViews[uiSlot] = {
    .m_Type         = xiiGALCommandEncoderState::ResourceBinding::Texture,
    .m_hTextureView = hTextureView,
    .m_pTexture     = pTextureView != nullptr ? pTextureView->GetTexture() : nullptr,
  };

  CountStateChange();
}

void xiiGALCommandEncoder::SetUnorderedAccessBufferView(xiiUInt32 uiSlot, xiiGALBufferViewHandle hUnorderedAccessBufferView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (uiSlot < m_State.m_hUnorderedAccessViews.GetCount() && m_State.m_hUnorderedAccessViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Buffer && m_State.m_hUnorderedAccessViews[uiSlot].m_hBufferView == hUnorderedAccessBufferView)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALBufferView* pUnorderedAccessBufferView = m_Device.GetBufferView(hUnorderedAccessBufferView);
  if (pUnorderedAccessBufferView != nullptr)
  {
    if (UnsetBufferView(pUnorderedAccessBufferView->GetBuffer()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetUnorderedAccessBufferViewPlatform(uiSlot, pUnorderedAccessBufferView);

  m_State.m_hUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_hUnorderedAccessViews[uiSlot] = {
    .m_Type         = xiiGALCommandEncoderState::ResourceBinding::Buffer,
    .m_hBufferView  = hUnorderedAccessBufferView,
    .m_hTextureView = xiiGALTextureViewHandle(),
    .m_pBuffer      = pUnorderedAccessBufferView != nullptr ? pUnorderedAccessBufferView->GetBuffer() : nullptr,
    .m_pTexture     = nullptr,
  };

  CountStateChange();
}

void xiiGALCommandEncoder::SetUnorderedAccessTextureView(xiiUInt32 uiSlot, xiiGALTextureViewHandle hUnorderedAccessTextureView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (uiSlot < m_State.m_hUnorderedAccessViews.GetCount() && m_State.m_hUnorderedAccessViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Texture && m_State.m_hUnorderedAccessViews[uiSlot].m_hTextureView == hUnorderedAccessTextureView)
  {
    CountRedundantStateChange();
    return;
  }

  xiiGALTextureView* pUnorderedAccessTextureView = m_Device.GetTextureView(hUnorderedAccessTextureView);
  if (pUnorderedAccessTextureView != nullptr)
  {
    if (UnsetTextureView(pUnorderedAccessTextureView->GetTexture()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetUnorderedAccessTextureViewPlatform(uiSlot, pUnorderedAccessTextureView);

  m_State.m_hUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_hUnorderedAccessViews[uiSlot] = {
    .m_Type         = xiiGALCommandEncoderState::ResourceBinding::Texture,
    .m_hBufferView  = xiiGALBufferViewHandle(),
    .m_hTextureView = hUnorderedAccessTextureView,
    .m_pBuffer      = nullptr,
    .m_pTexture     = pUnorderedAccessTextureView != nullptr ? pUnorderedAccessTextureView->GetTexture() : nullptr,
  };

  CountStateChange();
}

bool xiiGALCommandEncoder::UnsetBufferView(xiiGALBuffer* pBuffer)
{
  bool bResult = false;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_hResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_State.m_hResourceViews[stage][uiSlot].m_pBuffer == pBuffer)
      {
        XII_ASSERT_DEV(m_State.m_hResourceViews[stage][uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Buffer, "");

        m_CommonImpl.SetBufferViewPlatform(xiiGALShaderStage::GetStageFlag(stage), uiSlot, nullptr);

        m_State.m_hResourceViews[stage][uiSlot].m_hBufferView.Invalidate();
        m_State.m_hResourceViews[stage][uiSlot].m_pBuffer = nullptr;

        bResult = true;
      }
    }
  }

  return bResult;
}

bool xiiGALCommandEncoder::UnsetTextureView(xiiGALTexture* pTexture)
{
  bool bResult = false;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_hResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_State.m_hResourceViews[stage][uiSlot].m_pTexture == pTexture)
      {
        XII_ASSERT_DEV(m_State.m_hResourceViews[stage][uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Texture, "");

        m_CommonImpl.SetTextureViewPlatform(xiiGALShaderStage::GetStageFlag(stage), uiSlot, nullptr);

        m_State.m_hResourceViews[stage][uiSlot].m_hTextureView.Invalidate();
        m_State.m_hResourceViews[stage][uiSlot].m_pTexture = nullptr;

        bResult = true;
      }
    }
  }

  return bResult;
}

bool xiiGALCommandEncoder::UnsetUnorderedAccessBufferView(xiiGALBuffer* pBuffer)
{
  bool bResult = false;

  for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_hUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_State.m_hUnorderedAccessViews[uiSlot].m_pBuffer == pBuffer)
    {
      XII_ASSERT_DEV(m_State.m_hUnorderedAccessViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Texture, "");

      m_CommonImpl.SetUnorderedAccessBufferViewPlatform(uiSlot, nullptr);

      m_State.m_hUnorderedAccessViews[uiSlot].m_hBufferView.Invalidate();
      m_State.m_hUnorderedAccessViews[uiSlot].m_pBuffer = nullptr;

      bResult = true;
    }
  }

  return bResult;
}

bool xiiGALCommandEncoder::UnsetUnorderedAccessTextureView(xiiGALTexture* pTexture)
{
  bool bResult = false;

  for (xiiUInt32 uiSlot = 0; uiSlot < m_State.m_hUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_State.m_hUnorderedAccessViews[uiSlot].m_pTexture == pTexture)
    {
      XII_ASSERT_DEV(m_State.m_hUnorderedAccessViews[uiSlot].m_Type == xiiGALCommandEncoderState::ResourceBinding::Texture, "");

      m_CommonImpl.SetUnorderedAccessTextureViewPlatform(uiSlot, nullptr);

      m_State.m_hUnorderedAccessViews[uiSlot].m_hTextureView.Invalidate();
      m_State.m_hUnorderedAccessViews[uiSlot].m_pTexture = nullptr;

      bResult = true;
    }
  }

  return bResult;
}

void xiiGALCommandEncoder::BeginQuery(xiiGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto pQuery = m_Device.GetQuery(hQuery);
  XII_ASSERT_DEV(!pQuery->m_bStarted, "Attempting to begin query '{0}' twice. A query must be ended before it can be begun again.", pQuery->GetDescription().m_sName);

  m_CommonImpl.BeginQueryPlatform(pQuery);
}

void xiiGALCommandEncoder::EndQuery(xiiGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto pQuery = m_Device.GetQuery(hQuery);
  XII_ASSERT_DEV(pQuery->m_bStarted, "Attempting to end query '{0}' that has not been begun.", pQuery->GetDescription().m_sName);

  m_CommonImpl.EndQueryPlatform(pQuery);
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALBufferViewHandle hBufferView, xiiVec4 vClearValues)
{
  AssertRenderingThread();

  xiiGALBufferView* pUnorderedAccessBufferView = m_Device.GetBufferView(hBufferView);
  if (pUnorderedAccessBufferView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access buffer view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessBufferView, vClearValues);
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALTextureViewHandle hTextureView, xiiVec4 vClearValues)
{
  AssertRenderingThread();

  xiiGALTextureView* pUnorderedAccessTextureView = m_Device.GetTextureView(hTextureView);
  if (pUnorderedAccessTextureView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access texture view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessTextureView, vClearValues);
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALBufferViewHandle hBufferView, xiiVec4U32 vClearValues)
{
  AssertRenderingThread();

  xiiGALBufferView* pUnorderedAccessBufferView = m_Device.GetBufferView(hBufferView);
  if (pUnorderedAccessBufferView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access buffer view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessBufferView, vClearValues);
}

void xiiGALCommandEncoder::ClearUnorderedAccessView(xiiGALTextureViewHandle hTextureView, xiiVec4U32 vClearValues)
{
  AssertRenderingThread();

  xiiGALTextureView* pUnorderedAccessTextureView = m_Device.GetTextureView(hTextureView);
  if (pUnorderedAccessTextureView == nullptr)
  {
    XII_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access texture view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessTextureView, vClearValues);
}

void xiiGALCommandEncoder::CopyBuffer(xiiGALBufferHandle hDestination, xiiGALBufferHandle hSource)
{
  AssertRenderingThread();

  xiiGALBuffer* pDestination = m_Device.GetBuffer(hDestination);
  xiiGALBuffer* pSource      = m_Device.GetBuffer(hSource);

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDestination, pSource);
  }
  else
  {
    XII_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}.", xiiArgP(pDestination), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::CopyBufferRegion(xiiGALBufferHandle hDestination, xiiUInt32 uiDestinationOffset, xiiGALBufferHandle hSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  AssertRenderingThread();

  xiiGALBuffer* pDestination = m_Device.GetBuffer(hDestination);
  xiiGALBuffer* pSource      = m_Device.GetBuffer(hSource);

  if (pDestination != nullptr && pSource != nullptr)
  {
    const xiiUInt64 uiDestSize   = pDestination->GetDescription().m_uiSize;
    const xiiUInt64 uiSourceSize = pSource->GetDescription().m_uiSize;

    XII_ASSERT_DEV(uiDestSize >= uiDestinationOffset + uiByteCount, "Destination buffer too small (or offset too large).");
    XII_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too large).");

    m_CommonImpl.CopyBufferRegionPlatform(pDestination, uiDestinationOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    XII_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}.", xiiArgP(pDestination), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::UpdateBuffer(xiiGALBufferHandle hDestination, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");

  xiiGALBuffer* pDestination = m_Device.GetBuffer(hDestination);

  if (pDestination != nullptr)
  {
    XII_ASSERT_DEV(pDestination->GetDescription().m_uiSize >= (uiDestinationOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too large) for {} bytes.", pDestination->GetDescription().m_uiSize, uiDestinationOffset, sourceData.GetCount());
    m_CommonImpl.UpdateBufferPlatform(pDestination, uiDestinationOffset, sourceData, mapFlags);
  }
  else
  {
    XII_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid.");
  }
}

void xiiGALCommandEncoder::CopyTexture(xiiGALTextureHandle hDestination, xiiGALTextureHandle hSource)
{
  AssertRenderingThread();

  xiiGALTexture* pDestination = m_Device.GetTexture(hDestination);
  xiiGALTexture* pSource      = m_Device.GetTexture(hSource);

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDestination, pSource);
  }
  else
  {
    XII_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}.", xiiArgP(pDestination), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::CopyTextureRegion(xiiGALTextureHandle hDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTextureHandle hSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box)
{
  AssertRenderingThread();

  xiiGALTexture* pDestination = m_Device.GetTexture(hDestination);
  xiiGALTexture* pSource      = m_Device.GetTexture(hSource);

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDestination, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    XII_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}.", xiiArgP(pDestination), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::UpdateTexture(xiiGALTextureHandle hDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureSubResourceData& sourceData)
{
  AssertRenderingThread();

  xiiGALTexture* pDestination = m_Device.GetTexture(hDestination);

  if (pDestination != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDestination, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    XII_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}.", xiiArgP(pDestination));
  }
}

void xiiGALCommandEncoder::ResolveTexture(xiiGALTextureHandle hDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTextureHandle hSource, const xiiGALTextureMipLevelData& sourceSubResource)
{
  AssertRenderingThread();

  xiiGALTexture* pDestination = m_Device.GetTexture(hDestination);
  xiiGALTexture* pSource      = m_Device.GetTexture(hSource);

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDestination, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    XII_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}.", xiiArgP(pDestination), xiiArgP(pSource));
  }
}

void xiiGALCommandEncoder::ReadbackTexture(xiiGALTextureHandle hTexture, xiiGALTextureHandle hStagingTexture)
{
  AssertRenderingThread();

  xiiGALTexture* pTexture        = m_Device.GetTexture(hTexture);
  xiiGALTexture* pStagingTexture = m_Device.GetTexture(hStagingTexture);

  if (pTexture != nullptr && pStagingTexture != nullptr)
  {
    m_CommonImpl.ReadbackTexturePlatform(pTexture, pStagingTexture);
  }
  else
  {
    XII_REPORT_FAILURE("ReadbackTexture failed, texture handle invalid - texture = {0}, staging texture = {1}.", xiiArgP(pTexture), xiiArgP(pStagingTexture));
  }
}

void xiiGALCommandEncoder::CopyTextureReadbackResult(xiiGALTextureHandle hTexture, xiiGALTextureHandle hStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALTextureSubResourceData> targetData)
{
  AssertRenderingThread();

  xiiGALTexture* pTexture        = m_Device.GetTexture(hTexture);
  xiiGALTexture* pStagingTexture = m_Device.GetTexture(hStagingTexture);

  if (pTexture != nullptr && pStagingTexture != nullptr)
  {
    m_CommonImpl.CopyTextureReadbackResultPlatform(pTexture, pStagingTexture, mipLevelData, targetData);
  }
  else
  {
    XII_REPORT_FAILURE("CopyTextureReadbackResult failed, texture handle invalid - texture = {0}, staging texture = {1}.", xiiArgP(pTexture), xiiArgP(pStagingTexture));
  }
}

void xiiGALCommandEncoder::GenerateMipMaps(xiiGALTextureViewHandle hTextureView)
{
  AssertRenderingThread();

  xiiGALTextureView* pTextureView = m_Device.GetTextureView(hTextureView);

  if (pTextureView != nullptr)
  {
    XII_ASSERT_DEV(!pTextureView->GetDescription().m_hTexture.IsInvalidated(), "Texture view requires a valid texture handle to generate mip maps.");
    XII_ASSERT_DEV(pTextureView->GetDescription().m_Flags.IsSet(xiiGALTextureViewFlags::AllowMipGeneration), "xiiGALTextureViewFlags::AllowMipGeneration flag must be set on texture view allow mip generation.");

    const xiiGALTexture* pTexture = m_Device.GetTexture(pTextureView->GetDescription().m_hTexture);

    XII_ASSERT_DEV(pTexture->GetDescription().m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips), "xiiGALMiscTextureFlags::GenerateMips flag must be set on texture to generate mips.");

    m_CommonImpl.GenerateMipMapsPlatform(pTextureView);
  }
}

void xiiGALCommandEncoder::Flush()
{
  AssertRenderingThread();

  m_CommonImpl.FlushPlatform();
}

void xiiGALCommandEncoder::PushMarker(xiiStringView sMarker)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(!sMarker.IsEmpty(), "Marker must not be empty.");

  m_CommonImpl.PushMarkerPlatform(sMarker);
}

void xiiGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void xiiGALCommandEncoder::InsertEventMarker(xiiStringView sMarker, const xiiColor& color /* = xiiColor::White */)
{
  AssertRenderingThread();

  XII_ASSERT_DEV(!sMarker.IsEmpty(), "Marker must not be empty.");

  m_CommonImpl.InsertEventMarkerPlatform(sMarker, color);
}

void xiiGALCommandEncoder::ClearStatisticsCounters()
{
  // Reset counters for various statistics
  m_uiStateChanges          = 0U;
  m_uiRedundantStateChanges = 0U;
}

void xiiGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandEncoder);
