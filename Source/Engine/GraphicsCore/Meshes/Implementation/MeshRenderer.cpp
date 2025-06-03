#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/Implementation/MeshRendererUtils.h>
#include <GraphicsCore/Meshes/InstancedMeshComponent.h>
#include <GraphicsCore/Meshes/MeshRenderer.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiMeshRenderer::xiiMeshRenderer()  = default;
xiiMeshRenderer::~xiiMeshRenderer() = default;

void xiiMeshRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiMeshRenderData>());
  ref_types.PushBack(xiiGetStaticRTTI<xiiInstancedMeshRenderData>());
}

void xiiMeshRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Sky);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitForeground);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleForeground);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Selection);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::GUI);
}

void xiiMeshRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  const xiiMeshRenderData* pRenderData = batch.GetFirstData<xiiMeshRenderData>();

  const xiiMeshResourceHandle&     hMesh                    = pRenderData->m_hMesh;
  const xiiMaterialResourceHandle& hMaterial                = pRenderData->m_hMaterial;
  const xiiUInt32                  uiPartIndex              = pRenderData->m_uiSubMeshIndex;
  const bool                       bHasExplicitInstanceData = pRenderData->IsInstanceOf<xiiInstancedMeshRenderData>();

  xiiResourceLock<xiiMeshResource> pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
    return;

  xiiInstanceData* pInstanceData = bHasExplicitInstanceData ? static_cast<const xiiInstancedMeshRenderData*>(pRenderData)->m_pExplicitInstanceData : pPass->GetPipeline()->GetFrameDataProvider<xiiInstanceDataProvider>()->GetData(renderViewContext);

  if (pRenderData->m_uiFlipWinding)
  {
    renderViewContext.SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    renderViewContext.SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  SetAdditionalData(renderViewContext, pCommandList, pRenderData);

  pInstanceData->BindResources(pContext);

  if (!bHasExplicitInstanceData)
  {
    xiiUInt32 uiStartIndex = 0;
    while (uiStartIndex < batch.GetCount())
    {
      const xiiUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

      xiiUInt32                       uiInstanceDataOffset = 0;
      xiiArrayPtr<xiiPerInstanceData> instanceData         = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

      xiiUInt32 uiFilteredCount = 0;
      FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

      if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
      {
        pInstanceData->UpdateInstanceData(pCommandList, uiFilteredCount);

        const xiiMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

        if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiFilteredCount).Failed())
        {
          for (auto it = batch.GetIterator<xiiMeshRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
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
  else
  {
    xiiUInt32 uiInstanceCount = static_cast<const xiiInstancedMeshRenderData*>(pRenderData)->m_uiExplicitInstanceCount;

    const xiiMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

    pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiInstanceCount).IgnoreResult();
  }
}

void xiiMeshRenderer::SetAdditionalData(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiMeshRenderData* pRenderData) const
{
  renderViewContext.SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  XII_IGNORE_UNUSED(pCommandList);
  XII_IGNORE_UNUSED(pRenderData);
}

void xiiMeshRenderer::FillPerInstanceData(xiiArrayPtr<xiiPerInstanceData> pInstanceData, const xiiRenderDataBatch& batch, xiiUInt32 uiStartIndex, xiiUInt32& out_uiFilteredCount) const
{
  xiiUInt32 uiCount        = xiiMath::Min<xiiUInt32>(pInstanceData.GetCount(), batch.GetCount() - uiStartIndex);
  xiiUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<xiiMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    xiiInternal::FillPerInstanceData(pInstanceData[uiCurrentIndex], it);

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshRenderer);
