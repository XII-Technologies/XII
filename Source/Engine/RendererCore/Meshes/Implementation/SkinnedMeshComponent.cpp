#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/Types.h>
#include <RendererFoundation/Device/Device.h>

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

    xiiGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize                = sizeof(xiiShaderTransform);
    BufferDesc.m_uiTotalSize                 = BufferDesc.m_uiStructSize * m_Transforms.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer      = true;
    BufferDesc.m_bAllowShaderResourceView    = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hGpuBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, m_Transforms.GetArrayPtr().ToByteArray());

    m_bTransformsUpdated[0] = std::make_shared<bool>(true);
    m_bTransformsUpdated[1] = std::make_shared<bool>(true);
  }
  else
  {
    const xiiUInt32 uiRenIdx        = xiiRenderWorld::GetDataIndexForExtraction();
    *m_bTransformsUpdated[uiRenIdx] = false;
  }
}

void xiiSkinningState::FillSkinnedMeshRenderData(xiiSkinnedMeshRenderData& renderData) const
{
  renderData.m_hSkinningTransforms = m_hGpuBuffer;

  const xiiUInt32 uiExIdx = xiiRenderWorld::GetDataIndexForExtraction();

  if (m_bTransformsUpdated[uiExIdx] && *m_bTransformsUpdated[uiExIdx] == false)
  {
    auto pSkinningMatrices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiShaderTransform, m_Transforms.GetCount());
    pSkinningMatrices.CopyFrom(m_Transforms);

    renderData.m_pNewSkinningTransformData = pSkinningMatrices.ToByteArray();
    renderData.m_bTransformsUpdated        = m_bTransformsUpdated[uiExIdx];
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
