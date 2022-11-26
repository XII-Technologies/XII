#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>

#include <JoltPlugin/Components/JoltVisColMeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshContext, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Jolt_Colmesh_Triangle;Jolt_Colmesh_Convex"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltCollisionMeshContext::xiiJoltCollisionMeshContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void xiiJoltCollisionMeshContext::HandleMessage(const xiiEditorEngineDocumentMsg* pDocMsg)
{
  if (auto pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pDocMsg))
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

void xiiJoltCollisionMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc           obj;
  xiiJoltVisColMeshComponent* pMesh = nullptr;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const xiiTag& tagCastShadows = xiiTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    xiiJoltVisColMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    xiiStringBuilder sMeshGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = xiiResourceManager::LoadResource<xiiJoltMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);
  }
}

xiiEngineProcessViewContext* xiiJoltCollisionMeshContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiJoltCollisionMeshViewContext, this);
}

void xiiJoltCollisionMeshContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiJoltCollisionMeshContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiJoltCollisionMeshViewContext* pMeshViewContext = static_cast<xiiJoltCollisionMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void xiiJoltCollisionMeshContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pMeshObject->GetGlobalBounds();

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
