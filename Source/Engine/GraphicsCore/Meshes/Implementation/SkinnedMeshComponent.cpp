#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Shader/Types.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkinningState::xiiSkinningState() = default;

xiiSkinningState::~xiiSkinningState()
{
  Clear();
}

void xiiSkinningState::Clear()
{
  m_pGpuBuffer.Clear();
  m_Transforms.Clear();
}

void xiiSkinningState::TransformsChanged()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_pGpuBuffer)
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
    m_pGpuBuffer          = pDevice->CreateBuffer(bufferDescription, &initData);

    m_pGpuBuffer->SetDebugName("xiiSkinningState");
  }
  else
  {
    // \todo Implement updating buffer for next frame.
    // xiiGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_hGpuBuffer, m_Transforms.GetByteArrayPtr());

    XII_IGNORE_UNUSED(pDevice);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshComponent);
