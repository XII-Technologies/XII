#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>

xiiConstantBufferStorageBase::xiiConstantBufferStorageBase(xiiUInt32 uiSizeInBytes)

{
  m_Data = xiiMakeArrayPtr(static_cast<xiiUInt8*>(xiiFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  xiiMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = xiiGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
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
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  xiiUInt32 uiNewHash = xiiHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
    m_uiLastHash = uiNewHash;
  }
}



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ConstantBufferStorage);
