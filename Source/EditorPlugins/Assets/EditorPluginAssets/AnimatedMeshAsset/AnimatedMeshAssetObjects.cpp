/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetProperties, 2, xiiRTTIDefaultAllocator<xiiAnimatedMeshAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::MeshesWithAnimations)),
    XII_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    XII_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    XII_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("NormalPrecision", xiiMeshNormalPrecision, m_NormalPrecision),
    XII_ENUM_MEMBER_PROPERTY("TexCoordPrecision", xiiMeshTexCoordPrecision, m_TexCoordPrecision),
    XII_ENUM_MEMBER_PROPERTY("VertexColorConversion", xiiMeshVertexColorConversion, m_VertexColorConversion),
    XII_ENUM_MEMBER_PROPERTY("BoneWeightPrecision", xiiMeshBoneWeigthPrecision, m_BoneWeightPrecision),
    XII_MEMBER_PROPERTY("NormalizeWeights", m_bNormalizeWeights)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new xiiContainerAttribute(false, true, true)),
    XII_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    XII_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new xiiDefaultValueAttribute(50), new xiiClampValueAttribute(1, 100)),
    XII_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new xiiDefaultValueAttribute(5), new xiiClampValueAttribute(1, 100)),
    XII_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimatedMeshAssetProperties::xiiAnimatedMeshAssetProperties()  = default;
xiiAnimatedMeshAssetProperties::~xiiAnimatedMeshAssetProperties() = default;

void xiiAnimatedMeshAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiAnimatedMeshAssetProperties>())
  {
    const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();

    auto& props = *e.m_pPropertyStates;

    props["MeshSimplification"].m_Visibility       = bSimplify ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility   = bSimplify ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  }
}
