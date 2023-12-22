#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshContext, 1, xiiRTTIDefaultAllocator<xiiAnimatedMeshContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Animated Mesh"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimatedMeshContext::xiiAnimatedMeshContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pAnimatedMeshObject = nullptr;
}

void xiiAnimatedMeshContext::HandleMessage(const xiiEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = xiiDynamicCast<const xiiEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    xiiAnimatedMeshComponent* pAnimatedMesh;
    if (m_pAnimatedMeshObject && m_pAnimatedMeshObject->TryGetComponentOfBaseType(pAnimatedMesh))
    {
      for (xiiUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        xiiMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(pMsg->m_Materials[i]);
        }

        pAnimatedMesh->SetMaterial(i, hMat);
      }
    }

    return;
  }

  if (auto* pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void xiiAnimatedMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiAnimatedMeshComponent* pAnimatedMesh;

  // Preview AnimatedMesh
  {
    xiiGameObjectDesc obj;
    obj.m_bDynamic = true;
    obj.m_sName.Assign("AnimatedMeshPreview");
    pWorld->CreateObject(obj, m_pAnimatedMeshObject);

    const xiiTag& tagCastShadows = xiiTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pAnimatedMeshObject->SetTag(tagCastShadows);

    xiiAnimatedMeshComponent::CreateComponent(m_pAnimatedMeshObject, pAnimatedMesh);
    xiiStringBuilder sAnimatedMeshGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sAnimatedMeshGuid);
    m_hAnimatedMesh = xiiResourceManager::LoadResource<xiiMeshResource>(sAnimatedMeshGuid);
    pAnimatedMesh->SetMesh(m_hAnimatedMesh);
  }
}

xiiEngineProcessViewContext* xiiAnimatedMeshContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiAnimatedMeshViewContext, this);
}

void xiiAnimatedMeshContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiAnimatedMeshContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiAnimatedMeshViewContext* pAnimatedMeshViewContext = static_cast<xiiAnimatedMeshViewContext*>(pThumbnailViewContext);
  return pAnimatedMeshViewContext->UpdateThumbnailCamera(bounds);
}

void xiiAnimatedMeshContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pAnimatedMeshObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pAnimatedMeshObject->UpdateLocalBounds();
    m_pAnimatedMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pAnimatedMeshObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const xiiQuerySelectionBBoxMsgToEngine* msg = static_cast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg);

  xiiQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID     = msg->m_uiViewID;
  res.m_iPurpose     = msg->m_iPurpose;
  res.m_vCenter      = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
