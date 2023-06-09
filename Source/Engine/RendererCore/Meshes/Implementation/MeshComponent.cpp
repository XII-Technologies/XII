#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMeshComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiMeshComponent::xiiMeshComponent()  = default;
xiiMeshComponent::~xiiMeshComponent() = default;

void xiiMeshComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != xiiWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    xiiMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    xiiResourceLock<xiiMeshResource> pRenderMesh(hRenderMesh, xiiResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(xiiResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), xiiResourceManager::LoadResource<xiiCpuMeshResource>(GetMeshFile()));
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
