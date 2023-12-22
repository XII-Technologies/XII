#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeContext, 1, xiiRTTIDefaultAllocator<xiiTextureCubeContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Cube"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTextureCubeContext::xiiTextureCubeContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiTextureCubeContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentConfigMsgToEngine>())
  {
    const xiiDocumentConfigMsgToEngine* pMsg2 = static_cast<const xiiDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "ChannelMode" && m_hMaterial.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("LodLevel", pMsg2->m_fValue);
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiTextureCubeContext::OnInitialize()
{
  const char*      szMeshName = "DefaultTextureCubePreviewMesh";
  xiiStringBuilder sTextureGuid;
  xiiConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const xiiStringBuilder sMaterialResource(sTextureGuid.GetData(), " - TextureCube Preview");

  m_hPreviewMeshResource = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshName);
  m_hMaterial            = xiiResourceManager::GetExistingResource<xiiMaterialResource>(sMaterialResource);

  m_hTexture                               = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sTextureGuid);
  xiiGALTextureFormat::Enum textureFormat = xiiGALTextureFormat::Invalid;
  {
    xiiResourceLock<xiiTextureCubeResource> pTexture(m_hTexture, xiiResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(xiiMakeDelegate(&xiiTextureCubeContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTextureCubePreviewMeshBuffer";

    xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      xiiGeometry geom;
      geom.AddSphere(0.5f, 64, 64);
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
    md.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Editor/Materials/TextureCubePreview.xiiMaterial");

    auto& tb = md.m_TextureCubeBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = textureFormat != xiiGALTextureFormat::Invalid ? !xiiGALTextureFormat::IsSrgb(textureFormat) : false;

    m_hMaterial = xiiResourceManager::GetOrCreateResource<xiiMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    xiiGameObjectDesc obj;
    xiiGameObject*    pObj;

    obj.m_sName.Assign("TextureCubePreview");
    obj.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(90));
    m_hPreviewObject    = m_pWorld->CreateObject(obj, pObj);

    xiiMeshComponent* pMesh;
    m_hPreviewMesh2D = xiiMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

xiiEngineProcessViewContext* xiiTextureCubeContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiTextureCubeViewContext, this);
}

void xiiTextureCubeContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

void xiiTextureCubeContext::OnResourceEvent(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUpdated)
  {
    const xiiTextureCubeResource* pTexture = static_cast<const xiiTextureCubeResource*>(e.m_pResource);
    if (pTexture->GetFormat() != xiiGALTextureFormat::Invalid)
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !xiiGALTextureFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
