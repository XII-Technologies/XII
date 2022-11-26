#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVertexColorRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcVertexColorRenderer, 1, xiiRTTIDefaultAllocator<xiiProcVertexColorRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiProcVertexColorRenderer::xiiProcVertexColorRenderer()  = default;
xiiProcVertexColorRenderer::~xiiProcVertexColorRenderer() = default;

void xiiProcVertexColorRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const
{
  types.PushBack(xiiGetStaticRTTI<xiiProcVertexColorRenderData>());
}

void xiiProcVertexColorRenderer::SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const
{
  SUPER::SetAdditionalData(renderViewContext, pRenderData);

  xiiGALDevice*     pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pProcVertexColorRenderData = static_cast<const xiiProcVertexColorRenderData*>(pRenderData);

  pContext->BindBuffer("perInstanceVertexColors", pDevice->GetDefaultResourceView(pProcVertexColorRenderData->m_hVertexColorBuffer));
}

void xiiProcVertexColorRenderer::FillPerInstanceData(
  xiiArrayPtr<xiiPerInstanceData> instanceData,
  const xiiRenderDataBatch&       batch,
  xiiUInt32                       uiStartIndex,
  xiiUInt32&                      out_uiFilteredCount) const
{
  xiiUInt32 uiCount        = xiiMath::Min<xiiUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  xiiUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<xiiProcVertexColorRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    auto& perInstanceData = instanceData[uiCurrentIndex];

    xiiInternal::FillPerInstanceData(perInstanceData, it);
    perInstanceData.VertexColorAccessData = it->m_uiBufferAccessData;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}
