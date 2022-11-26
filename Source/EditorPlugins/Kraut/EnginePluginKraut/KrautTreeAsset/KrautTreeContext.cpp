#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Components/SkyBoxComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeContext, 1, xiiRTTIDefaultAllocator<xiiKrautTreeContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Kraut Tree"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiKrautTreeContext::xiiKrautTreeContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMainObject = nullptr;
}

void xiiKrautTreeContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "UpdateTree" && !m_hKrautComponent.IsInvalidated())
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      if (pMsg->m_sPayload == "DisplayRandomSeed")
      {
        m_uiDisplayRandomSeed = static_cast<xiiUInt32>(pMsg->m_fPayload);

        pTree->SetCustomRandomSeed(m_uiDisplayRandomSeed);
      }
    }

    return;
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void xiiKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());


  xiiKrautTreeComponent* pTree;

  // Preview Mesh
  {
    xiiGameObjectDesc obj;
    obj.m_sName.Assign("KrautTreePreview");
    // TODO: making the object dynamic is a workaround!
    // without it, shadows keep disappearing when switching between tree documents
    // triggering resource reload will also trigger xiiKrautTreeComponent::OnMsgExtractRenderData,
    // which fixes the shadows for a while, but not caching the render-data (xiiRenderData::Caching::IfStatic)
    // 'solves' the issue in the preview
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    const xiiTag& tagCastShadows = xiiTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMainObject->SetTag(tagCastShadows);

    m_hKrautComponent = xiiKrautTreeComponent::CreateComponent(m_pMainObject, pTree);
    xiiStringBuilder sMeshGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMainResource = xiiResourceManager::LoadResource<xiiKrautGeneratorResource>(sMeshGuid);
    pTree->SetVariationIndex(0xFFFF); // takes the 'display seed'
    pTree->SetKrautGeneratorResource(m_hMainResource);
  }


  // Wind
  {
    xiiGameObjectDesc obj;
    obj.m_sName.Assign("Wind");

    xiiGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    xiiSimpleWindComponent* pWind = nullptr;
    xiiSimpleWindComponent::CreateComponent(pObj, pWind);

    pWind->m_Deviation       = xiiAngle::Degree(180);
    pWind->m_MinWindStrength = xiiWindStrength::Calm;
    pWind->m_MaxWindStrength = xiiWindStrength::ModerateBrexiie;
  }

  // ground
  {
    const char* szMeshName = "KrautPreviewGroundMesh";
    m_hPreviewMeshResource = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshName);

    if (!m_hPreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "KrautPreviewGroundMeshBuffer";

      xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        xiiGeometry::GeoOptions opt;
        opt.m_Transform.SetTranslationMatrix(xiiVec3(0, 0, -0.05f));

        xiiGeometry geom;
        geom.AddCylinder(8.0f, 7.9f, 0.05f, 0.05f, true, true, 32, opt);
        geom.TriangulatePolygons();
        geom.ComputeTangents();

        xiiMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

        hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }
      {
        xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

        xiiMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.xiiMaterialAsset
        md.ComputeBounds();

        m_hPreviewMeshResource = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }

    // Ground Mesh Component
    {
      xiiGameObjectDesc obj;
      obj.m_sName.Assign("KrautGround");

      xiiGameObject* pObj;
      pWorld->CreateObject(obj, pObj);

      xiiMeshComponent* pMesh;
      xiiMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
    }
  }
}

xiiEngineProcessViewContext* xiiKrautTreeContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiKrautTreeViewContext, this);
}

void xiiKrautTreeContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiKrautTreeContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
  }

  xiiBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  // undo the artificial bounds scale to get a tight bbox for better thumbnails
  const float fAdditionalZoom = 1.5f;
  bounds.m_fSphereRadius /= xiiKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;
  bounds.m_vBoxHalfExtends /= xiiKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;

  xiiKrautTreeViewContext* pMeshViewContext = static_cast<xiiKrautTreeViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void xiiKrautTreeContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pMainObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    auto b = m_pMainObject->GetGlobalBounds();

    if (b.IsValid())
    {
      b.m_fSphereRadius /= xiiKrautTreeComponent::s_iLocalBoundsScale;
      b.m_vBoxHalfExtends /= (float)xiiKrautTreeComponent::s_iLocalBoundsScale;

      bounds.ExpandToInclude(b);
    }
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
