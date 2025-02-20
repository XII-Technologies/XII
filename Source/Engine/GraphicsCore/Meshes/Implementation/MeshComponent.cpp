#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Meshes/CpuMeshResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMeshComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
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

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), xiiResourceManager::LoadResource<xiiCpuMeshResource>(GetMesh().GetResourceID()));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshComponent);
