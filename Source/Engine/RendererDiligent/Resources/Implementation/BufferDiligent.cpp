#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererDiligent/Resources/BufferDiligent.h>

xiiGALBufferDiligent::xiiGALBufferDiligent(const xiiGALBufferCreationDescription& Description) :
  xiiGALBuffer(Description), m_pBuffer(nullptr), m_IndexFormat(Diligent::VT_UNDEFINED)
{
}

xiiGALBufferDiligent::~xiiGALBufferDiligent() {}

xiiResult xiiGALBufferDiligent::InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  Diligent::BufferDesc BufferDesc;
  BufferDesc.Name = m_Description.m_szName;

  switch (m_Description.m_BufferType)
  {
    case xiiGALBufferType::ConstantBuffer:
      BufferDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
      break;

    case xiiGALBufferType::IndexBuffer:
      BufferDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
      m_IndexFormat        = m_Description.m_uiStructSize == 2 ? Diligent::VT_UINT16 : Diligent::VT_UINT32;
      break;

    case xiiGALBufferType::VertexBuffer:
      BufferDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
      break;

    case xiiGALBufferType::Generic:
      BufferDesc.BindFlags = Diligent::BIND_NONE;
      break;

    default:
      xiiLog::Error("Unknown buffer type supplied to CreateBuffer()!");
      return XII_FAILURE;
  }

  if (m_Description.m_bAllowShaderResourceView)
    BufferDesc.BindFlags |= Diligent::BIND_SHADER_RESOURCE;

  if (m_Description.m_bAllowUAV)
    BufferDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

  if (m_Description.m_bStreamOutputTarget)
    BufferDesc.BindFlags |= Diligent::BIND_STREAM_OUTPUT;

  BufferDesc.Size           = m_Description.m_uiTotalSize;
  BufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
  BufferDesc.MiscFlags      = Diligent::MISC_BUFFER_FLAG_NONE;

  if (m_Description.m_bUseForIndirectArguments)
    BufferDesc.BindFlags |= Diligent::BIND_INDIRECT_DRAW_ARGS;

  BufferDesc.Mode = Diligent::BUFFER_MODE_UNDEFINED;

  if (m_Description.m_bAllowRawViews)
    BufferDesc.Mode = Diligent::BUFFER_MODE_RAW;

  if (m_Description.m_bUseAsStructuredBuffer)
    BufferDesc.Mode = Diligent::BUFFER_MODE_STRUCTURED;

  if (m_Description.m_bUseAsFormattedBuffer)
    BufferDesc.Mode = Diligent::BUFFER_MODE_FORMATTED;

  BufferDesc.ElementByteStride = m_Description.m_uiStructSize;

  if (m_Description.m_BufferType == xiiGALBufferType::ConstantBuffer)
  {
    BufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    BufferDesc.Usage          = Diligent::USAGE_DYNAMIC;

    // If constant buffer: Patch size to be aligned to 64 bytes for easier usability
    BufferDesc.Size = xiiMemoryUtils::AlignSize(static_cast<xiiUInt32>(BufferDesc.Size), 64u);
  }
  else
  {
    if (m_Description.m_ResourceAccess.IsImmutable())
    {
      BufferDesc.Usage          = Diligent::USAGE_IMMUTABLE;
      BufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
    }
    else
    {
      if (m_Description.m_bAllowUAV) // UAVs allow writing from the GPU which cannot be combined with CPU write access.
      {
        BufferDesc.Usage = Diligent::USAGE_DEFAULT;
      }
      else
      {
        BufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        BufferDesc.Usage          = Diligent::USAGE_DYNAMIC;
      }
    }
  }

  Diligent::BufferData initialData = {};
  initialData.pData    = pInitialData.GetPtr();
  initialData.DataSize = pInitialData.GetCount();
  pDeviceDiligent->GetDevice()->CreateBuffer(BufferDesc, pInitialData.IsEmpty() ? nullptr : &initialData, &m_pBuffer);

  if (m_pBuffer == nullptr)
  {
    xiiLog::Error("Failed to create buffer for graphics device!");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBufferDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pBuffer);

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_BufferDiligent);
