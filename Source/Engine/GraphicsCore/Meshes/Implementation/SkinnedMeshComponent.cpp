#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Shader/Types.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkinningState::xiiSkinningState() = default;

xiiSkinningState::~xiiSkinningState()
{
  Clear();
}

void xiiSkinningState::Clear()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_hGpuBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(m_hGpuBuffer);
    m_hGpuBuffer.Invalidate();
  }

  m_Transforms.Clear();
}

void xiiSkinningState::TransformsChanged()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (m_hGpuBuffer.IsInvalidated())
  {
    if (m_Transforms.GetCount() == 0)
      return;

    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_uiElementByteStride = sizeof(xiiShaderTransform);
    bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * m_Transforms.GetCount();
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Staging;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    xiiGALBufferData initData;
    initData.m_pData      = m_Transforms.GetData();
    initData.m_uiDataSize = m_Transforms.GetArrayPtr().ToByteArray().GetCount();
    m_hGpuBuffer          = pDevice->CreateBuffer(bufferDescription, &initData);

    pDevice->GetBuffer(m_hGpuBuffer)->SetDebugName("xiiSkinningState");
  }
  else
  {
    // \todo Implement updating buffer for next frame.
    // xiiGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_hGpuBuffer, m_Transforms.GetByteArrayPtr());

    XII_IGNORE_UNUSED(pDevice);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshComponent);
