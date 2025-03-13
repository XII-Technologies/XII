#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshContext, 1, xiiRTTIDefaultAllocator<xiiMeshContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMeshContext::xiiMeshContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void xiiMeshContext::HandleMessage(const xiiEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = xiiDynamicCast<const xiiEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    xiiMeshComponent* pMesh;
    if (m_pMeshObject && m_pMeshObject->TryGetComponentOfBaseType(pMesh))
    {
      for (xiiUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        xiiMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(pMsg->m_Materials[i]);
        }

        pMesh->SetMaterial(i, hMat);
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
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
        return;
      }
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void xiiMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc obj;
  xiiMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const xiiTag& tagCastShadows = xiiTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    xiiMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    xiiStringBuilder sMeshGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);

    {
      xiiResourceLock<xiiMeshResource> pMeshRes(m_hMesh, xiiResourceAcquireMode::PointerOnly);
      pMeshRes->m_ResourceEvents.AddEventHandler(xiiMakeDelegate(&xiiMeshContext::OnResourceEvent, this), m_MeshResourceEventSubscriber);
    }
  }
}

xiiEngineProcessViewContext* xiiMeshContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiMeshViewContext, this);
}

void xiiMeshContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiMeshContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  if (m_bBoundsDirty)
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
  }
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiMeshViewContext* pMeshViewContext = static_cast<xiiMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void xiiMeshContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds = xiiBoundingBoxSphere::MakeInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
    const auto& b  = m_pMeshObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const xiiQuerySelectionBBoxMsgToEngine* msg = static_cast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg);

  xiiQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID     = msg->m_uiViewID;
  res.m_iPurpose     = msg->m_iPurpose;
  res.m_vCenter      = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void xiiMeshContext::OnResourceEvent(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUpdated)
  {
    m_bBoundsDirty = true;
  }
}
