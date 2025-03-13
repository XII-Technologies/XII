#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureContext, 1, xiiRTTIDefaultAllocator<xiiTextureContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target;Substance Package"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static void CreatePreviewRect(xiiGeometry& ref_geom)
{
  const xiiMat4  mTransform = xiiMat4::MakeIdentity();
  const xiiVec2  size(1.0f);
  const xiiColor color = xiiColor::White;

  const xiiVec2 halfSize = size * 0.5f;

  xiiUInt32 idx[4];

  idx[0] = ref_geom.AddVertex(mTransform, xiiVec3(-halfSize.x, 0, -halfSize.y), xiiVec3(-1, 0, 0), xiiVec2(-1, 2), color);
  idx[1] = ref_geom.AddVertex(mTransform, xiiVec3(halfSize.x, 0, -halfSize.y), xiiVec3(-1, 0, 0), xiiVec2(2, 2), color);
  idx[2] = ref_geom.AddVertex(mTransform, xiiVec3(halfSize.x, 0, halfSize.y), xiiVec3(-1, 0, 0), xiiVec2(2, -1), color);
  idx[3] = ref_geom.AddVertex(mTransform, xiiVec3(-halfSize.x, 0, halfSize.y), xiiVec3(-1, 0, 0), xiiVec2(-1, -1), color);

  ref_geom.AddPolygon(idx, false);
}

xiiTextureContext::xiiTextureContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiTextureContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentConfigMsgToEngine>() && m_hMaterial.IsValid())
  {
    const xiiDocumentConfigMsgToEngine* pMsg2 = static_cast<const xiiDocumentConfigMsgToEngine*>(pMsg);

    xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
    if (pMsg2->m_sWhatToDo == "SetChannelMode")
    {
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("AlphaThreshold", pMsg2->m_fValue);
    }
    else if (pMsg2->m_sWhatToDo == "SetLodLevel")
    {
      if (pMsg2->m_iValue != m_iLodLevel)
      {
        pMaterial->SetParameter("LodLevel", pMsg2->m_iValue);
        m_iLodLevel = pMsg2->m_iValue;
      }
    }
    else if (pMsg2->m_sWhatToDo == "SetTexture" && pMsg2->m_sValue.IsEmpty() == false)
    {
      SetTexture(pMsg2->m_sValue);
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiTextureContext::OnInitialize()
{
  xiiStringBuilder sTextureGuid;
  xiiConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const xiiStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture Preview");

  m_hMaterial = xiiResourceManager::GetExistingResource<xiiMaterialResource>(sMaterialResource);

  // Preview Mesh
  const char* szMeshName = "DefaultTexturePreviewMesh";
  m_hPreviewMeshResource = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

    xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      xiiGeometry geom;
      CreatePreviewRect(geom);
      geom.ComputeTangents();

      xiiMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

      hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

      xiiMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "");
      md.ComputeBounds();

      m_hPreviewMeshResource = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    xiiMaterialResourceDescriptor md;
    md.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Editor/Materials/TexturePreview.xiiMaterial");

    m_hMaterial = xiiResourceManager::GetOrCreateResource<xiiMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    xiiGameObjectDesc obj;
    xiiGameObject*    pObj;

    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));
    m_hPreviewObject    = m_pWorld->CreateObject(obj, pObj);

    xiiMeshComponent* pMesh;
    m_hPreviewMesh2D = xiiMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }

  SetTexture(sTextureGuid);
}

xiiEngineProcessViewContext* xiiTextureContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiTextureViewContext, this);
}

void xiiTextureContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

void xiiTextureContext::SetTexture(xiiStringView sTextureFile)
{
  if (m_hTexture.IsValid() && m_hTexture.GetResourceID() == sTextureFile)
    return;

  m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sTextureFile);
  xiiResourceLock<xiiTexture2DResource> pTexture(m_hTexture, xiiResourceAcquireMode::PointerOnly);
  pTexture->m_ResourceEvents.AddEventHandler(xiiMakeDelegate(&xiiTextureContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);

  xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::BlockTillLoaded);
  pMaterial->SetTexture2DBinding("BaseTexture", m_hTexture);
  pMaterial->SetParameter("IsLinear", !xiiGALResourceFormat::IsSrgb(pTexture->GetFormat()));
}

void xiiTextureContext::OnResourceEvent(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUpdated)
  {
    const xiiTexture2DResource* pTexture = static_cast<const xiiTexture2DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != xiiGALResourceFormat::Unknown)
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !xiiGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
