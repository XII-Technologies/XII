#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

xiiConstantBufferStorageBase::xiiConstantBufferStorageBase(xiiUInt32 uiSizeInBytes)
{
  m_Data = xiiMakeArrayPtr(static_cast<xiiUInt8*>(xiiFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  xiiMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(xiiGALDevice::GetDefaultDevice(), uiSizeInBytes);
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

void xiiConstantBufferStorageBase::UploadData(xiiGALCommandList* pCommandList)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  xiiUInt32 uiNewHash = xiiHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    XII_ASSERT_DEV(m_Data.GetCount() <= xiiGALDevice::GetDefaultDevice()->GetBuffer(m_hGALConstantBuffer)->GetDescription().m_uiSize, "The size of the constant buffer storage is greater than the available storage!");

    pCommandList->UpdateBufferExtended(m_hGALConstantBuffer, 0, m_Data);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ConstantBufferStorage);
