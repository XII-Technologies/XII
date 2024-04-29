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

  xiiGALBufferCreationDescription desc;
  desc.m_uiSize         = uiSizeInBytes;
  desc.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  desc.m_ResourceUsage  = xiiGALResourceUsage::Dynamic;
  desc.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  m_hGALConstantBuffer  = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
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
#if 0
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  xiiUInt32 uiNewHash = xiiHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
    m_uiLastHash = uiNewHash;
  }
#else
  m_bHasBeenModified = false;
  pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
#endif
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ConstantBufferStorage);
