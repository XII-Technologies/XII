/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

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
  if (const auto* config = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pMsg))
  {
    if (config->m_sWhatToDo == "PreviewLighting")
    {
      if (config->m_PayloadValue == m_LastLightingMessage)
        return;
      xiiPreviewLightingSettings settings;
      if (xiiPreviewLightingSettings::FromVariant(config->m_PayloadValue, settings).Succeeded() && m_pPreviewLighting != nullptr && m_pPreviewLighting->SetSettings(settings).Succeeded())
      {
        m_LightingSettings    = settings;
        m_LastLightingMessage = config->m_PayloadValue;
      }
      else
        xiiLog::Warning("Rejected invalid material preview lighting settings.");
      return;
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiCreateThumbnailMsgToEngine>())
  {
    xiiResourceManager::RestoreResource(m_hMaterial);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentConfigMsgToEngine>())
  {
    const xiiDocumentConfigMsgToEngine* pMsg2 = static_cast<const xiiDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "PreviewScene" && xiiMath::IsFinite(pMsg2->m_fValue))
    {
      m_bPreviewGrid = pMsg2->m_iValue != 0;
      if (m_fPreviewOrientation != pMsg2->m_fValue)
      {
        XII_LOCK(m_pWorld->GetWriteMarker());
        xiiMeshComponent* mesh = nullptr;
        if (m_pWorld->TryGetComponent(m_hMeshComponent, mesh))
          mesh->GetOwner()->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(pMsg2->m_fValue)));
        m_fPreviewOrientation = pMsg2->m_fValue;
      }
    }
    else if (pMsg2->m_sWhatToDo == "InvalidateCache")
    {
      // make sure all scenes etc rebuild their render cache
      if (m_pWorld != nullptr)
      {
        XII_LOCK(m_pWorld->GetWriteMarker());
        m_pWorld->GetOrCreateModule<xiiRenderWorldModule>()->DeleteAllCachedRenderData();
      }
    }
    else if (pMsg2->m_sWhatToDo == "PreviewModel" && pMsg2->m_iValue >= 0 && pMsg2->m_iValue <= static_cast<int>(PreviewModel::Plane) && m_PreviewModel != (PreviewModel)pMsg2->m_iValue)
    {
      m_PreviewModel = (PreviewModel)pMsg2->m_iValue;

      auto pWorld = m_pWorld;
      XII_LOCK(pWorld->GetWriteMarker());

      xiiMeshComponent* pMesh;
      if (pWorld->TryGetComponent(m_hMeshComponent, pMesh))
      {
        switch (m_PreviewModel)
        {
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

  if (const auto* config = xiiDynamicCast<const xiiDocumentConfigMsgToEngine*>(pMsg))
  {
    if (config->m_sWhatToDo == "PreviewScene" || config->m_sWhatToDo == "PreviewModel" || config->m_sWhatToDo == "InvalidateCache")
      return;
  }
  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

xiiStatus xiiMaterialContext::OnInitialize()
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
        opt.m_Color     = xiiColor::White;
        opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));
        geom.AddStackedSphere(0.1f, 64, 64, opt);
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
        opt.m_Color = xiiColor::White;

        // Build geometry
        xiiGeometry geom;

        geom.AddBox(xiiVec3(0.12f), true, opt);
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
        opt.m_Color     = xiiColor::White;
        opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(-90));
        geom.AddRect(xiiVec2(0.2f), 64, 64, opt);
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

        m_hPlaneMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szPlaneMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDescription obj;
  xiiGameObject*           pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, pObj);

    xiiMeshComponent* pMesh;
    m_hMeshComponent = xiiMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hSphereMesh);
    xiiStringBuilder sMaterialGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(sMaterialGuid);

    // All three generated preview meshes have one material slot.
    pMesh->SetMaterialOverride(0, m_hMaterial);
  }
  m_pPreviewLighting = XII_DEFAULT_NEW(xiiScenePreviewLighting, *pWorld);
  // This world contains only transient preview state. The Editor tag is hidden
  // when rendering thumbnails or disabling gizmos, so it must not hide lighting.
  XII_SUCCEED_OR_RETURN(m_pPreviewLighting->EnsureCreated(false, true));
  XII_SUCCEED_OR_RETURN(m_pPreviewLighting->SetSettings(m_LightingSettings));
  return XII_SUCCESS;
}

void xiiMaterialContext::OnDeinitialize()
{
  m_pPreviewLighting.Clear();
  xiiEngineProcessDocumentContext::OnDeinitialize();
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
