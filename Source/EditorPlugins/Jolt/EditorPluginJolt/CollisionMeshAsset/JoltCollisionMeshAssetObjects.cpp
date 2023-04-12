#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiJoltSurfaceResourceSlot, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiJoltSurfaceResourceSlot>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
    XII_MEMBER_PROPERTY("Exclude", m_bExclude),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltCollisionMeshType, 2)
  XII_ENUM_CONSTANT(xiiJoltCollisionMeshType::ConvexHull),
  XII_ENUM_CONSTANT(xiiJoltCollisionMeshType::TriangleMesh),
  XII_ENUM_CONSTANT(xiiJoltCollisionMeshType::Cylinder),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltConvexCollisionMeshType, 1)
  XII_ENUM_CONSTANT(xiiJoltConvexCollisionMeshType::ConvexHull),
  XII_ENUM_CONSTANT(xiiJoltConvexCollisionMeshType::Cylinder),
  XII_ENUM_CONSTANT(xiiJoltConvexCollisionMeshType::ConvexDecomposition),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetProperties, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("RightDir", xiiBasisAxis, m_RightDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveX)),
    XII_ENUM_MEMBER_PROPERTY("UpDir", xiiBasisAxis, m_UpDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    XII_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("IsConvexMesh", m_bIsConvexMesh)->AddAttributes(new xiiHiddenAttribute()),
    XII_ENUM_MEMBER_PROPERTY("ConvexMeshType", xiiJoltConvexCollisionMeshType, m_ConvexMeshType),
    XII_MEMBER_PROPERTY("MaxConvexPieces", m_uiMaxConvexPieces)->AddAttributes(new xiiDefaultValueAttribute(5)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(0, 32)),
    XII_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::Meshes)),
    XII_ARRAY_MEMBER_PROPERTY("Surfaces", m_Slots)->AddAttributes(new xiiContainerAttribute(false, false, true)),
    XII_MEMBER_PROPERTY("Surface", m_sConvexMeshSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltCollisionMeshAssetProperties::xiiJoltCollisionMeshAssetProperties()  = default;
xiiJoltCollisionMeshAssetProperties::~xiiJoltCollisionMeshAssetProperties() = default;

void xiiJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != xiiGetStaticRTTI<xiiJoltCollisionMeshAssetProperties>())
    return;

  const bool     isConvex = e.m_pObject->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  const xiiInt64 meshType = e.m_pObject->GetTypeAccessor().GetValue("ConvexMeshType").ConvertTo<xiiInt64>();

  auto& props = *e.m_pPropertyStates;

  props["Radius"].m_Visibility          = xiiPropertyUiState::Invisible;
  props["Radius2"].m_Visibility         = xiiPropertyUiState::Invisible;
  props["Height"].m_Visibility          = xiiPropertyUiState::Invisible;
  props["Detail"].m_Visibility          = xiiPropertyUiState::Invisible;
  props["MeshFile"].m_Visibility        = xiiPropertyUiState::Invisible;
  props["ConvexMeshType"].m_Visibility  = xiiPropertyUiState::Invisible;
  props["MaxConvexPieces"].m_Visibility = xiiPropertyUiState::Invisible;
  props["Surfaces"].m_Visibility        = isConvex ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
  props["Surface"].m_Visibility         = isConvex ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;

  if (!isConvex)
  {
    props["MeshFile"].m_Visibility = xiiPropertyUiState::Default;
  }
  else
  {
    props["ConvexMeshType"].m_Visibility = xiiPropertyUiState::Default;

    switch (meshType)
    {
      case xiiJoltConvexCollisionMeshType::ConvexDecomposition:
        props["MaxConvexPieces"].m_Visibility = xiiPropertyUiState::Default;
        [[fallthrough]];

      case xiiJoltConvexCollisionMeshType::ConvexHull:
        props["MeshFile"].m_Visibility = xiiPropertyUiState::Default;
        break;

      case xiiJoltConvexCollisionMeshType::Cylinder:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Radius2"].m_Visibility = xiiPropertyUiState::Default;
        props["Height"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        break;
    }
  }
}
