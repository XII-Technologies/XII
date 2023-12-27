#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetProperties, 2, xiiRTTIDefaultAllocator<xiiAnimatedMeshAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::SkeletalMeshes)),
    XII_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    XII_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    XII_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("NormalPrecision", xiiMeshNormalPrecision, m_NormalPrecision),
    XII_ENUM_MEMBER_PROPERTY("TexCoordPrecision", xiiMeshTexCoordPrecision, m_TexCoordPrecision),
    XII_ENUM_MEMBER_PROPERTY("BoneWeightPrecision", xiiMeshBoneWeigthPrecision, m_BoneWeightPrecision),
    XII_MEMBER_PROPERTY("NormalizeWeights", m_bNormalizeWeights)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new xiiContainerAttribute(false, true, true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimatedMeshAssetProperties::xiiAnimatedMeshAssetProperties()  = default;
xiiAnimatedMeshAssetProperties::~xiiAnimatedMeshAssetProperties() = default;
