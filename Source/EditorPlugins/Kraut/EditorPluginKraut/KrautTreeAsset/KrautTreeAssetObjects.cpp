#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiKrautAssetMaterial, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiKrautAssetMaterial>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeAssetProperties, 1, xiiRTTIDefaultAllocator<xiiKrautTreeAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new xiiFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    XII_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new xiiDefaultValueAttribute(0.4f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(1.0f, 10000.0f)),
    XII_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
    XII_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new xiiContainerAttribute(false, false, false)),
    XII_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    XII_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiKrautTreeAssetProperties::xiiKrautTreeAssetProperties()  = default;
xiiKrautTreeAssetProperties::~xiiKrautTreeAssetProperties() = default;
