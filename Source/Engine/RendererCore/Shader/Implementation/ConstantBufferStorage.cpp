#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

xiiConstantBufferStorageBase::xiiConstantBufferStorageBase(xiiUInt32 uiSizeInBytes, const char* szName) :
  m_bHasBeenModified(false), m_uiLastHash(0)
{
  m_Data = xiiMakeArrayPtr(static_cast<xiiUInt8*>(xiiFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  xiiMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = xiiGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes, szName);
}

xiiConstantBufferStorageBase::~xiiConstantBufferStorageBase()
{
  xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);

  xiiFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Clear();
}

xiiArrayPtr<xiiUInt8> xiiConstantBufferStorageBase::GetRawDataForWriting()
{
  m_bHasBeenModified = true;
  return m_Data;
}

xiiArrayPtr<const xiiUInt8> xiiConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void xiiConstantBufferStorageBase::UploadData(xiiGALCommandEncoder* pCommandEncoder)
{
  if (pCommandEncoder->GetDevice().GetCapabilities().m_DeviceType == xiiGraphicsDeviceType::D3D11)
  {
    if (!m_bHasBeenModified)
      return;

    xiiUInt32 uiNewHash = xiiHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
    if (m_uiLastHash != uiNewHash)
    {
      pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
      m_uiLastHash = uiNewHash;
    }
  }
  else
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
  }

  m_bHasBeenModified = false;
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ConstantBufferStorage);
