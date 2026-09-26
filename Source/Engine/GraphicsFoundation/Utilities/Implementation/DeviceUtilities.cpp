/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

xiiEnum<xiiGALGraphicsAdapterVendor> xiiGALDeviceUtilities::GetVendorFromID(xiiUInt32 uiID)
{
  switch (uiID)
  {
    case 0x01002: // AMD
      return xiiGALGraphicsAdapterVendor::AMD;
    case 0x010DE: // NVIDIA
      return xiiGALGraphicsAdapterVendor::Nvidia;
    case 0x08086: // Intel
      return xiiGALGraphicsAdapterVendor::Intel;
    case 0x013B5: // ARM
      return xiiGALGraphicsAdapterVendor::ARM;
    case 0x05143: // Qualcomm
      return xiiGALGraphicsAdapterVendor::Qualcomm;
    case 0x01010: // Imagination Technologies
      return xiiGALGraphicsAdapterVendor::ImaginationTechnologies;
    case 0x01414: // Microsoft
      return xiiGALGraphicsAdapterVendor::Microsoft;
    case 0x0106B: // Apple
      return xiiGALGraphicsAdapterVendor::Apple;
    case 0x10005: // Mesa
      return xiiGALGraphicsAdapterVendor::Mesa;
    case 0x014E4: // Broadcom
      return xiiGALGraphicsAdapterVendor::Broadcom;
    default:
      return xiiGALGraphicsAdapterVendor::Unknown;
  }
}

xiiSharedPtr<xiiGALBuffer> xiiGALDeviceUtilities::CreateVertexBuffer(xiiGALDevice* pDevice, xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, xiiArrayPtr<xiiUInt8> pInitialData, bool bDataIsMutable)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Invalid device provided.");

  const bool bIsImmutable = (!pInitialData.IsEmpty() && !bDataIsMutable);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::VertexBuffer;
  bufferDescription.m_uiElementByteStride = uiVertexSize;
  bufferDescription.m_uiSize              = uiVertexSize * xiiMath::Max(1U, uiVertexCount);
  bufferDescription.m_Usage               = bIsImmutable ? xiiGALResourceUsage::Immutable : xiiGALResourceUsage::Dynamic;
  bufferDescription.m_CPUAccessFlags      = bIsImmutable ? xiiGALCPUAccessFlag::None : xiiGALCPUAccessFlag::Write;

  xiiGALBufferData initialData;
  initialData.m_pData      = pInitialData.GetPtr();
  initialData.m_uiDataSize = pInitialData.GetCount();

  return pDevice->CreateBuffer(bufferDescription, &initialData);
}

xiiSharedPtr<xiiGALBuffer> xiiGALDeviceUtilities::CreateIndexBuffer(xiiGALDevice* pDevice, IndexType indexType, xiiUInt32 uiIndexCount, xiiArrayPtr<xiiUInt8> pInitialData, bool bDataIsMutable)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Invalid device provided.");

  const bool bIsImmutable = (!pInitialData.IsEmpty() && !bDataIsMutable);

  xiiUInt32 uiIndexSize = 0;
  if (indexType == IndexType::UShort)
    uiIndexSize = sizeof(xiiUInt16);
  else if (indexType == IndexType::UInt)
    uiIndexSize = sizeof(xiiUInt32);

  XII_ASSERT_DEV(uiIndexCount != 0U, "Implementation Error: Unexpected index size.");

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::IndexBuffer;
  bufferDescription.m_uiElementByteStride = uiIndexSize;
  bufferDescription.m_uiSize              = uiIndexSize * xiiMath::Max(1U, uiIndexCount);
  bufferDescription.m_Usage               = bIsImmutable ? xiiGALResourceUsage::Immutable : xiiGALResourceUsage::Dynamic;
  bufferDescription.m_CPUAccessFlags      = bIsImmutable ? xiiGALCPUAccessFlag::None : xiiGALCPUAccessFlag::Write;

  xiiGALBufferData initialData;
  initialData.m_pData      = pInitialData.GetPtr();
  initialData.m_uiDataSize = pInitialData.GetCount();

  return pDevice->CreateBuffer(bufferDescription, &initialData);
}

xiiSharedPtr<xiiGALBuffer> xiiGALDeviceUtilities::CreateConstantBuffer(xiiGALDevice* pDevice, xiiUInt32 uiBufferSize, xiiStringView sDebugName /*= {}*/)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Invalid device provided.");

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  bufferDescription.m_uiElementByteStride = 0U;
  bufferDescription.m_uiSize              = uiBufferSize;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

  if (xiiSharedPtr<xiiGALBuffer> pConstantBuffer = pDevice->CreateBuffer(bufferDescription))
  {
    if (!sDebugName.IsEmpty())
    {
      pConstantBuffer->SetDebugName(sDebugName);
    }
    return pConstantBuffer;
  }
  return nullptr;
}

xiiSharedPtr<xiiGALBuffer> xiiGALDeviceUtilities::CreateStagingBuffer(xiiGALDevice* pDevice, xiiUInt32 uiBufferSize, xiiStringView sDebugName)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Invalid device provided.");

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::None;
  bufferDescription.m_uiElementByteStride = 0U;
  bufferDescription.m_uiSize              = uiBufferSize;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Staging;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

  if (xiiSharedPtr<xiiGALBuffer> pStagingBuffer = pDevice->CreateBuffer(bufferDescription))
  {
    if (!sDebugName.IsEmpty())
    {
      pStagingBuffer->SetDebugName(sDebugName);
    }
    return pStagingBuffer;
  }
  return nullptr;
}

xiiGALTextureCreationDescription xiiGALDeviceUtilities::CreateRenderTargetDescription(xiiSizeU32 size, xiiGALResourceFormat::Enum format, xiiUInt32 uiSampleCount)
{
  return xiiGALTextureCreationDescription{
    .m_Type               = xiiGALResourceDimension::Texture2D,
    .m_Size               = size,
    .m_uiArraySizeOrDepth = 1U,
    .m_Format             = format,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = uiSampleCount,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource | (xiiGALResourceFormat::IsDepthFormat(format) ? xiiGALBindFlags::DepthStencil : xiiGALBindFlags::RenderTarget),
    .m_Usage              = xiiGALResourceUsage::Mutable,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };
}

xiiResult xiiGALDeviceUtilities::MapAndUpdateBuffer(xiiGALCommandList* pCommandList, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags /*= xiiGALMapFlags::Discard*/)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "Invalid command list.");

  xiiGALDevice* pDevice                   = pCommandList->GetDevice();
  const auto&   graphicsAdapterProperties = pDevice->GetGraphicsDeviceAdapterProperties();
  const auto&   bufferDescription         = pBuffer->GetDescription();

  XII_IGNORE_UNUSED(graphicsAdapterProperties);

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_CHECK_ALIGNMENT(pSourceData.GetPtr(), graphicsAdapterProperties.m_BufferProperties.m_uiConstantBufferAlignment);
  }
  if (bufferDescription.m_Mode == xiiGALBufferMode::Structured)
  {
    XII_ASSERT_DEV((uiDestinationOffset % graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment) == 0, "Offset must be aligned to {} bytes.", graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment);
  }

  void* pMappedData = nullptr;
  XII_SUCCEED_OR_RETURN(pCommandList->MapBuffer(pBuffer, xiiGALMapType::Write, mapFlags, pMappedData));

  memcpy(xiiMemoryUtils::AddByteOffset(pMappedData, uiDestinationOffset), pSourceData.GetPtr(), pSourceData.GetCount());

  pCommandList->UnmapBuffer(pBuffer, xiiGALMapType::Write).AssertSuccess("Failed to unmap buffer.");

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Utilities_Implementation_DeviceUtilities);
