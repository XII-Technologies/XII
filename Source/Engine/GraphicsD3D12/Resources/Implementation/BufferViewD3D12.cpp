/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferViewD3D12::xiiGALBufferViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(std::move(pDeviceD3D12), std::move(pBuffer), creationDescription)
{
}

xiiGALBufferViewD3D12::~xiiGALBufferViewD3D12() = default;

xiiResult xiiGALBufferViewD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALBufferD3D12> pBufferD3D12 = m_pBuffer.Downcast<xiiGALBufferD3D12>();

  if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to initialize D3D12 buffer view '{}': backing buffer is invalid.", GetDebugName());
    return XII_FAILURE;
  }

  const xiiGALBufferCreationDescription& bufferDescription = pBufferD3D12->GetDescription();

  if (m_Description.m_ViewType == xiiGALBufferViewType::ShaderResource && !bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    xiiLog::Error("Failed to initialize D3D12 buffer view '{}': the parent buffer is missing xiiGALBindFlags::ShaderResource.", GetDebugName());
    return XII_FAILURE;
  }
  if (m_Description.m_ViewType == xiiGALBufferViewType::UnorderedAccess && !bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    xiiLog::Error("Failed to initialize D3D12 buffer view '{}': the parent buffer is missing xiiGALBindFlags::UnorderedAccess.", GetDebugName());
    return XII_FAILURE;
  }

  m_ViewMetadata.m_uiByteOffset = m_Description.m_uiByteOffset;
  m_ViewMetadata.m_uiByteWidth  = m_Description.m_uiByteWidth;
  m_ViewMetadata.m_Format       = m_Description.m_Format;
  m_ViewMetadata.m_bRawView     = false;

  switch (bufferDescription.m_Mode)
  {
    case xiiGALBufferMode::Formatted:
    {
      const xiiGALResourceFormatDescription& formatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);
      const xiiUInt64                        uiElementSize     = formatDescription.GetElementSize();

      m_ViewMetadata.m_uiFirstElement = static_cast<xiiUInt32>(m_Description.m_uiByteOffset / uiElementSize);
      m_ViewMetadata.m_uiElementCount = static_cast<xiiUInt32>(m_Description.m_uiByteWidth / uiElementSize);
    }
    break;

    case xiiGALBufferMode::Structured:
    {
      const xiiUInt64 uiStride = bufferDescription.m_uiElementByteStride;
      m_ViewMetadata.m_uiStructureByteStride = static_cast<xiiUInt32>(uiStride);
      m_ViewMetadata.m_uiFirstElement        = static_cast<xiiUInt32>(m_Description.m_uiByteOffset / uiStride);
      m_ViewMetadata.m_uiElementCount        = static_cast<xiiUInt32>(m_Description.m_uiByteWidth / uiStride);
    }
    break;

    case xiiGALBufferMode::Raw:
    {
      if (m_Description.m_Format == xiiGALResourceFormat::Unknown)
      {
        m_ViewMetadata.m_bRawView       = true;
        m_ViewMetadata.m_uiFirstElement = static_cast<xiiUInt32>(m_Description.m_uiByteOffset / sizeof(xiiUInt32));
        m_ViewMetadata.m_uiElementCount = static_cast<xiiUInt32>(m_Description.m_uiByteWidth / sizeof(xiiUInt32));
      }
      else
      {
        const xiiGALResourceFormatDescription& formatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);
        const xiiUInt64                        uiElementSize     = formatDescription.GetElementSize();

        m_ViewMetadata.m_uiFirstElement = static_cast<xiiUInt32>(m_Description.m_uiByteOffset / uiElementSize);
        m_ViewMetadata.m_uiElementCount = static_cast<xiiUInt32>(m_Description.m_uiByteWidth / uiElementSize);
      }
    }
    break;

    default:
      xiiLog::Error("Failed to initialize D3D12 buffer view '{}': unsupported buffer mode '{}'.", GetDebugName(), xiiArgEnum(bufferDescription.m_Mode));
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALBufferViewD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferViewD3D12);
