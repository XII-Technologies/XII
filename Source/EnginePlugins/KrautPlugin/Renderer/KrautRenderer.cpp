#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderer.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautRenderData, 1, xiiRTTIDefaultAllocator<xiiKrautRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautRenderer, 1, xiiRTTIDefaultAllocator<xiiKrautRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiKrautRenderer::xiiKrautRenderer()  = default;
xiiKrautRenderer::~xiiKrautRenderer() = default;

void xiiKrautRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const
{
  types.PushBack(xiiGetStaticRTTI<xiiKrautRenderData>());
}

void xiiKrautRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& categories) const
{
  categories.PushBack(xiiDefaultRenderDataCategories::LitOpaque);
  categories.PushBack(xiiDefaultRenderDataCategories::Selection);
}

void xiiKrautRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  const xiiKrautRenderData* pRenderData = batch.GetFirstData<xiiKrautRenderData>();

  xiiResourceLock<xiiMeshResource> pMesh(pRenderData->m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer sub-meshes.
  if (pMesh->GetSubMeshes().GetCount() <= pRenderData->m_uiSubMeshIndex)
    return;

  TempTreeCB treeConstants(pRenderContext);

  const auto& subMesh = pMesh->GetSubMeshes()[pRenderData->m_uiSubMeshIndex];

  xiiInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<xiiInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  // inverted trees are not allowed
  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  // no skinning atm
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  pRenderContext->BindMaterial(pMesh->GetMaterials()[subMesh.m_uiMaterialIndex]);
  pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  treeConstants.SetTreeData(pRenderData->m_vLeafCenter, renderViewContext.m_pViewData->m_CameraUsageHint == xiiCameraUsageHint::Shadow ? 1.0f : 0.0f);

  const xiiVec3 vLodCamPos = renderViewContext.m_pLodCamera->GetPosition();

  const bool bIsShadowView = renderViewContext.m_pViewData->m_CameraUsageHint == xiiCameraUsageHint::Shadow;

  for (xiiUInt32 uiStartIndex = 0; uiStartIndex < batch.GetCount(); /**/)
  {
    const xiiUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    xiiUInt32                       uiInstanceDataOffset = 0;
    xiiArrayPtr<xiiPerInstanceData> instanceData         = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

    xiiUInt32 uiFilteredCount = 0;
    FillPerInstanceData(vLodCamPos, instanceData, batch, bIsShadowView, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pRenderContext, uiFilteredCount);

      if (pRenderContext->DrawMeshBuffer(subMesh.m_uiPrimitiveCount, subMesh.m_uiFirstPrimitive, uiFilteredCount).Failed())
      {
        for (auto it = batch.GetIterator<xiiKrautRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
        {
          pRenderData = it;

          // draw bounding box instead
          if (pRenderData->m_GlobalBounds.IsValid())
          {
            xiiDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), xiiColor::Magenta);
          }
        }
      }
    }

    uiStartIndex += instanceData.GetCount();
  }
}

void xiiKrautRenderer::FillPerInstanceData(const xiiVec3& vLodCamPos, xiiArrayPtr<xiiPerInstanceData> instanceData, const xiiRenderDataBatch& batch, bool bIsShadowView, xiiUInt32 uiStartIndex, xiiUInt32& out_uiFilteredCount) const
{
  xiiUInt32 uiCount        = xiiMath::Min<xiiUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  xiiUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<xiiKrautRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const xiiKrautRenderData* pRenderData = it;

    const float fDistanceSQR = (pRenderData->m_GlobalTransform.m_vPosition - vLodCamPos).GetLengthSquared();

    if (fDistanceSQR < pRenderData->m_fLodDistanceMinSQR || fDistanceSQR >= pRenderData->m_fLodDistanceMaxSQR)
      continue;

    if (bIsShadowView && !pRenderData->m_bCastShadows)
      continue;

    const xiiMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    auto& perInstanceData         = instanceData[uiCurrentIndex];
    perInstanceData.ObjectToWorld = objectToWorld;

    // always assumes uniform-scale only
    perInstanceData.ObjectToWorldNormal = objectToWorld;
    perInstanceData.GameObjectID        = pRenderData->m_uiUniqueID;
    perInstanceData.Color               = xiiColor(pRenderData->m_vWindTrunk.x, pRenderData->m_vWindTrunk.y, pRenderData->m_vWindTrunk.z, pRenderData->m_vWindTrunk.GetLength());

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

xiiKrautRenderer::TempTreeCB::TempTreeCB(xiiRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage(m_pConstants, XII_STRINGIZE(xiiKrautTreeConstants));

  pRenderContext->BindConstantBuffer(XII_STRINGIZE(xiiKrautTreeConstants), m_hConstantBuffer);
}

xiiKrautRenderer::TempTreeCB::~TempTreeCB()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiKrautRenderer::TempTreeCB::SetTreeData(const xiiVec3& vTreeCenter, float fLeafShadowOffset)
{
  xiiKrautTreeConstants& cb = m_pConstants->GetDataForWriting();
  cb.LeafCenter             = vTreeCenter;
  cb.LeafShadowOffset       = fLeafShadowOffset;
}
