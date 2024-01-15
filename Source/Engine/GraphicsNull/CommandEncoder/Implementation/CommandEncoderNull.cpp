#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsNull/CommandEncoder/CommandEncoderNull.h>
#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BufferNull.h>
#include <GraphicsNull/Resources/BufferViewNull.h>
#include <GraphicsNull/Resources/FenceNull.h>
#include <GraphicsNull/Resources/FramebufferNull.h>
#include <GraphicsNull/Resources/QueryNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>
#include <GraphicsNull/Resources/SamplerNull.h>
#include <GraphicsNull/Resources/TextureNull.h>
#include <GraphicsNull/Resources/TextureViewNull.h>
#include <GraphicsNull/Shader/InputLayoutNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>
#include <GraphicsNull/States/BlendStateNull.h>
#include <GraphicsNull/States/DepthStencilStateNull.h>
#include <GraphicsNull/States/RasterizerStateNull.h>

xiiGALCommandEncoderNull::xiiGALCommandEncoderNull(xiiGALDeviceNull& deviceNull) :
  m_GALDeviceNull(deviceNull)
{
}

xiiGALCommandEncoderNull::~xiiGALCommandEncoderNull()
{
}

void xiiGALCommandEncoderNull::SetShaderPlatform(xiiGALShader* pShader)
{
  // auto pShaderNull = static_cast<xiiGALShaderNull*>(pShader);
}

void xiiGALCommandEncoderNull::SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer)
{
  // auto pBufferNull = static_cast<xiiGALBufferNull*>(pBuffer);
}

void xiiGALCommandEncoderNull::SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler)
{
  // auto pSamplerNull = static_cast<xiiGALSamplerNull*>(pSampler);
}

void xiiGALCommandEncoderNull::SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView)
{
  // auto pBufferViewNull = static_cast<xiiGALBufferViewNull*>(pBufferView);
}

void xiiGALCommandEncoderNull::SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pTextureView)
{
  // auto pTextureViewNull = static_cast<xiiGALTextureViewNull*>(pTextureView);
}

void xiiGALCommandEncoderNull::SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView)
{
  // auto pUnorderedAccessBufferViewNull = static_cast<xiiGALBufferViewNull*>(pUnorderedAccessBufferView);
}

void xiiGALCommandEncoderNull::SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView)
{
  // auto pUnorderedAccessTextureViewNull = static_cast<xiiGALTextureViewNull*>(pUnorderedAccessTextureView);
}

void xiiGALCommandEncoderNull::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  // auto pQueryNull = static_cast<xiiGALQueryNull*>(pQuery);
}

void xiiGALCommandEncoderNull::EndQueryPlatform(xiiGALQuery* pQuery)
{
  // auto pQueryNull = static_cast<xiiGALQueryNull*>(pQuery);
}

void xiiGALCommandEncoderNull::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderNull::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderNull::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderNull::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderNull::CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource)
{
  // auto pSourceBufferNull      = static_cast<xiiGALBufferNull*>(pSource);
  // auto pDestinationBufferNull = static_cast<xiiGALBufferNull*>(pDestination);
}

void xiiGALCommandEncoderNull::CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  // auto pSourceBufferNull      = static_cast<xiiGALBufferNull*>(pSource);
  // auto pDestinationBufferNull = static_cast<xiiGALBufferNull*>(pDestination);
}

void xiiGALCommandEncoderNull::UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(sourceData.GetPtr());

  auto        pDestinationBufferNull = static_cast<xiiGALBufferNull*>(pDestination);
  const auto& bufferDescription      = pDestinationBufferNull->GetDescription();

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == bufferDescription.m_uiSize, "Uniform (constant) buffers cannot be mapped partially, there are no checks for partial constant buffer updates.");
  }
}

void xiiGALCommandEncoderNull::CopyTexturePlatform(xiiGALTexture* pDestination, xiiGALTexture* pSource)
{
  // auto pSourceTexture      = static_cast<xiiGALTextureNull*>(pSource);
  // auto pDestinationTexture = static_cast<xiiGALTextureNull*>(pDestination);
}

void xiiGALCommandEncoderNull::CopyTextureRegionPlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box)
{
  // auto pSourceTextureNull      = static_cast<xiiGALTextureNull*>(pSource);
  // auto pDestinationTextureNull = static_cast<xiiGALTextureNull*>(pDestination);
}

void xiiGALCommandEncoderNull::UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureSubResourceData& sourceData)
{
  auto pDestinationTextureNull = static_cast<xiiGALTextureNull*>(pDestination);

  xiiUInt32 uiWidth  = xiiMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1U);
  xiiUInt32 uiHeight = xiiMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1U);
  xiiUInt32 uiDepth  = xiiMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1U);

  const auto& textureDescription = pDestinationTextureNull->GetDescription();
  const auto& formatProperties   = xiiGALGraphicsUtilities::GetTextureFormatProperties(textureDescription.m_Format);

  switch (textureDescription.m_Usage)
  {
    case xiiGALResourceUsage::Default:
    {
      xiiUInt32 uiRowPitch   = uiWidth * formatProperties.m_uiComponentSize;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;

      XII_ASSERT_DEV(sourceData.m_uiStride == uiRowPitch, "Invalid row pitch. Expected {0} got {1}.", uiRowPitch, sourceData.m_uiStride);
      XII_ASSERT_DEV(sourceData.m_uiDepthStride == 0 || sourceData.m_uiDepthStride == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, sourceData.m_uiDepthStride);
    }
    break;
    case xiiGALResourceUsage::Dynamic:
    {
      xiiUInt32 uiRowPitch   = uiWidth * formatProperties.m_uiComponentSize;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;

      XII_ASSERT_DEV(sourceData.m_uiStride == uiRowPitch, "Invalid row pitch. Expected {0} got {1}.", uiRowPitch, sourceData.m_uiStride);
      XII_ASSERT_DEV(sourceData.m_uiDepthStride == 0 || sourceData.m_uiDepthStride == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, sourceData.m_uiDepthStride);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderNull::ResolveTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource)
{
  // auto pSourceTextureNull      = static_cast<xiiGALTextureNull*>(pSource);
  // auto pDestinationTextureNull = static_cast<xiiGALTextureNull*>(pDestination);
}

void xiiGALCommandEncoderNull::ReadbackTexturePlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture)
{
  // auto pTextureNull        = static_cast<xiiGALTextureNull*>(pTexture);
  // auto pStagingTextureNull = static_cast<xiiGALTextureNull*>(pTexture);
}

void xiiGALCommandEncoderNull::CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALTextureSubResourceData> targetData)
{
  // auto pTextureNull        = static_cast<xiiGALTextureNull*>(pTexture);
  // auto pStagingTextureNull = static_cast<xiiGALTextureNull*>(pStagingTexture);

  XII_ASSERT_DEV(mipLevelData.GetCount() == targetData.GetCount(), "Source and target arrays must be of the same size.");
}

void xiiGALCommandEncoderNull::GenerateMipMapsPlatform(xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandEncoderNull::FlushPlatform()
{
}

void xiiGALCommandEncoderNull::PushMarkerPlatform(xiiStringView sMarker)
{
}

void xiiGALCommandEncoderNull::PopMarkerPlatform()
{
}

void xiiGALCommandEncoderNull::InsertEventMarkerPlatform(xiiStringView sMarker, const xiiColor& color)
{
}

void xiiGALCommandEncoderNull::ClearRenderTargetPlatform(xiiGALTextureView* pTextureView, const xiiColor& clearColor)
{
  // auto pTextureViewNull = static_cast<xiiGALTextureViewNull*>(pTextureView);
}

void xiiGALCommandEncoderNull::ClearDepthStencilPlatform(xiiGALTextureView* pTextureView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  // auto pTextureViewNull = static_cast<xiiGALTextureViewNull*>(pTextureView);
}

void xiiGALCommandEncoderNull::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
}

void xiiGALCommandEncoderNull::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
}

void xiiGALCommandEncoderNull::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
}

void xiiGALCommandEncoderNull::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

void xiiGALCommandEncoderNull::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
}

void xiiGALCommandEncoderNull::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

void xiiGALCommandEncoderNull::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  // auto pIndexBufferNull = static_cast<xiiGALBufferNull*>(pIndexBuffer);
}

void xiiGALCommandEncoderNull::SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer)
{
  // auto pVertexBufferNull = static_cast<xiiGALBufferNull*>(pVertexBuffer);
}

void xiiGALCommandEncoderNull::SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  // auto pInputLayoutNull = static_cast<xiiGALInputLayoutNull*>(pInputLayout);
}

void xiiGALCommandEncoderNull::SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology)
{
}

void xiiGALCommandEncoderNull::SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask)
{
  // auto pBlendStateNull = static_cast<xiiGALBlendStateNull*>(pBlendState);
}

void xiiGALCommandEncoderNull::SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  // auto pDepthStencilStateNull = static_cast<xiiGALDepthStencilStateNull*>(pDepthStencilState);
}

void xiiGALCommandEncoderNull::SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  // auto pRasterizerStateNull = static_cast<xiiGALRasterizerStateNull*>(pRasterizerState);
}

void xiiGALCommandEncoderNull::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
}

void xiiGALCommandEncoderNull::SetScissorRectPlatform(const xiiRectU32& rect)
{
}

void xiiGALCommandEncoderNull::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
}

void xiiGALCommandEncoderNull::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

void xiiGALCommandEncoderNull::BeginRendering(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPassNull* pRenderPassNull, xiiGALFramebufferNull* pFramebufferNull)
{
}

void xiiGALCommandEncoderNull::EndRendering()
{
}

void xiiGALCommandEncoderNull::BeginCompute()
{
}

void xiiGALCommandEncoderNull::EndCompute()
{
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandEncoderNull);
