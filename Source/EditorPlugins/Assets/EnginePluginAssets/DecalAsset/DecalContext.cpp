#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <GraphicsCore/Decals/DecalComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalContext, 1, xiiRTTIDefaultAllocator<xiiDecalContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Decal"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDecalContext::xiiDecalContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiDecalContext::OnInitialize()
{
  const char* szMeshName = "DefaultDecalPreviewMesh";
  m_hPreviewMeshResource = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultDecalPreviewMeshBuffer";

    xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      xiiGeometry             geom;
      xiiGeometry::GeoOptions opt;

      geom.AddBox(xiiVec3(0.5f, 1.0f, 1.0f), true);

      xiiMat4 t, r;
      t               = xiiMat4::MakeTranslation(xiiVec3(0, 1.5f, 0));
      r               = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddStackedSphere(0.5f, 64, 64, opt);

      t.SetTranslationVector(xiiVec3(0, -1.5f, 0));
      r               = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddTorus(0.1f, 0.5f, 32, 64, true, opt);

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
      md.SetMaterial(0, "Materials/Common/TestBricks.xiiMaterial");
      md.ComputeBounds();

      m_hPreviewMeshResource = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc obj;
  xiiGameObject*    pObj;

  // Preview Mesh that the decals get projected onto
  {
    obj.m_sName.Assign("DecalPreview");
    pWorld->CreateObject(obj, pObj);

    xiiMeshComponent* pMesh;
    xiiMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
  }

  // decals
  {
    xiiStringBuilder sDecalGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sDecalGuid);

    // box
    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalPosition.Set(-0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalPosition.Set(-0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalPosition.Set(-0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }


    // box
    {
      obj.m_sName.Assign("Decal4");
      obj.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(xiiVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal5");
      obj.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(xiiVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal6");
      obj.m_LocalRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      xiiDecalComponent* pDecal;
      xiiDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(xiiVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }
  }
}

xiiEngineProcessViewContext* xiiDecalContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiDecalViewContext, this);
}

void xiiDecalContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}
