#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoRenderer, 1, xiiRTTIDefaultAllocator<xiiGizmoRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float xiiGizmoRenderer::s_fGizmoScale = 1.0f;

xiiGizmoRenderer::xiiGizmoRenderer()  = default;
xiiGizmoRenderer::~xiiGizmoRenderer() = default;

void xiiGizmoRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& inout_types) const
{
  inout_types.PushBack(xiiGetStaticRTTI<xiiGizmoRenderData>());
}

void xiiGizmoRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& inout_categories) const
{
  inout_categories.PushBack(xiiDefaultRenderDataCategories::SimpleOpaque);
  inout_categories.PushBack(xiiDefaultRenderDataCategories::SimpleForeground);
}

void xiiGizmoRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  // special Windows specific hack:
  // When ALT is down, the editor shouldn't show any gizmos, because this is used for the orbit camera mode
  // in general the ALT key is problematic and shouldn't be used as a modifier in gizmos
  // so to indicate to users that ALT has a very different effect, and to discourage programmers from using ALT as a modifier,
  // we just hide all gizmos when ALT is down
  // however, detecting the ALT key is only possible with a direct OS check, since Qt doesn't report this as an individual key press
  // and the xii input system is not active at all times
  if (GetKeyState(VK_MENU) & 0x8000)
    return;
#endif

  bool bOnlyPickable = false;

  if (auto pPickingRenderPass = xiiDynamicCast<const xiiPickingRenderPass*>(pPass))
  {
    // gizmos only exist for 'selected' objects, so ignore all gizmo rendering, if we don't want to pick selected objects
    if (!pPickingRenderPass->m_bPickSelected)
      return;

    bOnlyPickable = true;
  }

  const xiiGizmoRenderData* pRenderData = batch.GetFirstData<xiiGizmoRenderData>();

  const xiiMeshResourceHandle&     hMesh          = pRenderData->m_hMesh;
  const xiiMaterialResourceHandle& hMaterial      = pRenderData->m_hMaterial;
  xiiUInt32                        uiSubMeshIndex = pRenderData->m_uiSubMeshIndex;

  xiiResourceLock<xiiMeshResource> pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiSubMeshIndex)
    return;

  const xiiMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiSubMeshIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);

  xiiConstantBufferStorage<xiiGizmoConstants>* pGizmoConstantBuffer;
  xiiConstantBufferStorageHandle               hGizmoConstantBuffer = xiiRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);
  XII_SCOPE_EXIT(xiiRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer));

  renderViewContext.m_pRenderContext->BindConstantBuffer("xiiGizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<xiiGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    if (bOnlyPickable && !pRenderData->m_bIsPickable)
      continue;

    XII_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    XII_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    XII_ASSERT_DEV(pRenderData->m_uiSubMeshIndex == uiSubMeshIndex, "Invalid batching (part)");

    xiiGizmoConstants& cb  = pGizmoConstantBuffer->GetDataForWriting();
    xiiMat4            m   = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.ObjectToWorldMatrix = m;
    m.Invert(0.001f).IgnoreResult(); // this can fail, if scale is 0 (which happens), doesn't matter in those cases
    cb.WorldToObjectMatrix = m;
    cb.GizmoColor          = pRenderData->m_GizmoColor;
    cb.GizmoScale          = fGizmoScale;
    cb.GameObjectID        = pRenderData->m_uiUniqueID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
      // draw bounding box instead
      if (pRenderData->m_GlobalBounds.IsValid())
      {
        xiiDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), xiiColor::Magenta);
      }
    }
  }
}
