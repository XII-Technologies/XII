#include <GraphicsCore/GraphicsCorePCH.h>

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

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshImportTransform, 1)
  XII_ENUM_CONSTANT(xiiMeshImportTransform::Blender_YUp),
  XII_ENUM_CONSTANT(xiiMeshImportTransform::Blender_ZUp),
  XII_ENUM_CONSTANT(xiiMeshImportTransform::Custom),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiBasisAxis::Enum xiiMeshImportTransform::GetRightDir(xiiMeshImportTransform::Enum transform, xiiBasisAxis::Enum dir)
{
  switch (transform)
  {
    case xiiMeshImportTransform::Blender_YUp:
      return xiiBasisAxis::NegativeX;
    case xiiMeshImportTransform::Blender_ZUp:
      return xiiBasisAxis::NegativeX;
    case xiiMeshImportTransform::Custom:
      return dir;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

xiiBasisAxis::Enum xiiMeshImportTransform::GetUpDir(xiiMeshImportTransform::Enum transform, xiiBasisAxis::Enum dir)
{
  switch (transform)
  {
    case xiiMeshImportTransform::Blender_YUp:
      return xiiBasisAxis::PositiveY;
    case xiiMeshImportTransform::Blender_ZUp:
      return xiiBasisAxis::PositiveZ;
    case xiiMeshImportTransform::Custom:
      return dir;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

bool xiiMeshImportTransform::GetFlipForward(xiiMeshImportTransform::Enum transform, bool bFlip)
{
  switch (transform)
  {
    case xiiMeshImportTransform::Blender_YUp:
      return false;
    case xiiMeshImportTransform::Blender_ZUp:
      return false;
    case xiiMeshImportTransform::Custom:
      return bFlip;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return bFlip;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshComponent);
