#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialContext, 1, xiiRTTIDefaultAllocator<xiiMaterialContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Material"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMaterialContext::xiiMaterialContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiMaterialContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiCreateThumbnailMsgToEngine>())
  {
    xiiResourceManager::RestoreResource(m_hMaterial);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentConfigMsgToEngine>())
  {
    const xiiDocumentConfigMsgToEngine* pMsg2 = static_cast<const xiiDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "InvalidateCache")
    {
      // make sure all scenes etc rebuild their render cache
      xiiRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg2->m_sWhatToDo == "PreviewModel" && m_PreviewModel != (PreviewModel)pMsg2->m_iValue)
    {
      m_PreviewModel = (PreviewModel)pMsg2->m_iValue;

      auto pWorld = m_pWorld;
      XII_LOCK(pWorld->GetWriteMarker());

      xiiMeshComponent* pMesh;
      if (pWorld->TryGetComponent(m_hMeshComponent, pMesh))
      {
        switch (m_PreviewModel)
        {
          case PreviewModel::Ball:
            pMesh->SetMesh(m_hBallMesh);
            break;
          case PreviewModel::Sphere:
            pMesh->SetMesh(m_hSphereMesh);
            break;
          case PreviewModel::Box:
            pMesh->SetMesh(m_hBoxMesh);
            break;
          case PreviewModel::Plane:
            pMesh->SetMesh(m_hPlaneMesh);
            break;
        }
      }
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiMaterialContext::OnInitialize()
{
  {
    const char* szSphereMeshName = "SphereMaterialPreviewMesh";
    m_hSphereMesh                = xiiResourceManager::GetExistingResource<xiiMeshResource>(szSphereMeshName);

    if (!m_hSphereMesh.IsValid())
    {
      const char* szMeshBufferName = "SphereMaterialPreviewMeshBuffer";

      xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        xiiGeometry geom;

        xiiGeometry::GeoOptions opt;
        opt.m_Color     = xiiColor::Red;
        opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));
        geom.AddSphere(0.1f, 64, 64, opt);
        geom.ComputeTangents();

        xiiMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(xiiGALVertexAttributeSemantic::TexCoord1, xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::Default));
        desc.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(xiiGALVertexAttributeSemantic::Color1, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

        hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

        xiiMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hSphereMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szSphereMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szBoxMeshName = "BoxMaterialPreviewMesh";
    m_hBoxMesh                = xiiResourceManager::GetExistingResource<xiiMeshResource>(szBoxMeshName);

    if (!m_hBoxMesh.IsValid())
    {
      const char* szMeshBufferName = "BoxMaterialPreviewMeshBuffer";

      xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        xiiGeometry::GeoOptions opt;
        opt.m_Color = xiiColor::Red;

        // Build geometry
        xiiGeometry geom;

        geom.AddBox(xiiVec3(0.12f), true, opt);
        geom.ComputeTangents();

        xiiMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(xiiGALVertexAttributeSemantic::TexCoord1, xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::Default));
        desc.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(xiiGALVertexAttributeSemantic::Color1, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

        hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

        xiiMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hBoxMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szBoxMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szPlaneMeshName = "PlaneMaterialPreviewMesh";
    m_hPlaneMesh                = xiiResourceManager::GetExistingResource<xiiMeshResource>(szPlaneMeshName);

    if (!m_hPlaneMesh.IsValid())
    {
      const char* szMeshBufferName = "PlaneMaterialPreviewMeshBuffer";

      xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        xiiGeometry geom;

        xiiGeometry::GeoOptions opt;
        opt.m_Color     = xiiColor::Red;
        opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(-90));
        geom.AddRectXY(xiiVec2(0.2f), 64, 64, opt);
        geom.ComputeTangents();

        xiiMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(xiiGALVertexAttributeSemantic::TexCoord1, xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::Default));
        desc.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(xiiGALVertexAttributeSemantic::Color1, xiiGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

        hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

        xiiMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hPlaneMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szPlaneMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    m_hBallMesh = xiiResourceManager::LoadResource<xiiMeshResource>("Editor/Meshes/MaterialBall.xiiMesh");
  }

  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc obj;
  xiiGameObject*    pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    pWorld->CreateObject(obj, pObj);

    xiiMeshComponent* pMesh;
    m_hMeshComponent = xiiMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hBallMesh);
    xiiStringBuilder sMaterialGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(sMaterialGuid);

    // 20 material overrides should be enough for any mesh.
    for (xiiUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
  }
}

xiiEngineProcessViewContext* xiiMaterialContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiMaterialViewContext, this);
}

void xiiMaterialContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiMaterialContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  xiiMaterialViewContext* pMaterialViewContext = static_cast<xiiMaterialViewContext*>(pThumbnailViewContext);
  pMaterialViewContext->PositionThumbnailCamera();
  return true;
}
