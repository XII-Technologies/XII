#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/Types.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSkinnedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hSkinningTransforms.GetInternalID().m_Data);
}

xiiSkinningState::xiiSkinningState() = default;

xiiSkinningState::~xiiSkinningState()
{
  Clear();
}

void xiiSkinningState::Clear()
{
  if (!m_hGpuBuffer.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGpuBuffer);
    m_hGpuBuffer.Invalidate();
  }

  m_bTransformsUpdated[0] = nullptr;
  m_bTransformsUpdated[1] = nullptr;
  m_Transforms.Clear();
}

void xiiSkinningState::TransformsChanged()
{
  if (m_hGpuBuffer.IsInvalidated())
  {
    if (m_Transforms.GetCount() == 0)
      return;

    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_uiElementByteStride = sizeof(xiiShaderTransform);
    bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * m_Transforms.GetCount();
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_ResourceUsage       = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    xiiGALBufferData initData;
    initData.m_pData      = m_Transforms.GetData();
    initData.m_uiDataSize = m_Transforms.GetArrayPtr().ToByteArray().GetCount();
    m_hGpuBuffer          = xiiGALDevice::GetDefaultDevice()->CreateBuffer(bufferDescription, &initData);

    m_bTransformsUpdated[0] = std::make_shared<bool>(true);
    m_bTransformsUpdated[1] = std::make_shared<bool>(true);
  }
  else
  {
    const xiiUInt32 uiRenIdx        = xiiRenderWorld::GetDataIndexForExtraction();
    *m_bTransformsUpdated[uiRenIdx] = false;
  }
}

void xiiSkinningState::FillSkinnedMeshRenderData(xiiSkinnedMeshRenderData& ref_renderData) const
{
  ref_renderData.m_hSkinningTransforms = m_hGpuBuffer;

  const xiiUInt32 uiExIdx = xiiRenderWorld::GetDataIndexForExtraction();

  if (m_bTransformsUpdated[uiExIdx] && *m_bTransformsUpdated[uiExIdx] == false)
  {
    auto pSkinningMatrices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiShaderTransform, m_Transforms.GetCount());
    pSkinningMatrices.CopyFrom(m_Transforms);

    ref_renderData.m_pNewSkinningTransformData = pSkinningMatrices.ToByteArray();
    ref_renderData.m_bTransformsUpdated        = m_bTransformsUpdated[uiExIdx];
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_SkinnedMeshComponent);
