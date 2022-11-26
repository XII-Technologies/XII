#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocumentInfo, 2, xiiRTTIDefaultAllocator<xiiAssetDocumentInfo>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_SET_MEMBER_PROPERTY("Dependencies", m_AssetTransformDependencies),
    XII_SET_MEMBER_PROPERTY("References", m_RuntimeDependencies),
    XII_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    XII_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    XII_ACCESSOR_PROPERTY("AssetType", GetAssetsDocumentTypeName, SetAssetsDocumentTypeName),
    XII_ARRAY_MEMBER_PROPERTY("MetaInfo", m_MetaInfo)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAssetDocumentInfo::xiiAssetDocumentInfo()
{
  m_uiSettingsHash = 0;
}

xiiAssetDocumentInfo::~xiiAssetDocumentInfo()
{
  ClearMetaData();
}

xiiAssetDocumentInfo::xiiAssetDocumentInfo(xiiAssetDocumentInfo&& rhs)
{
  (*this) = std::move(rhs);
}

void xiiAssetDocumentInfo::operator=(xiiAssetDocumentInfo&& rhs)
{
  m_uiSettingsHash             = rhs.m_uiSettingsHash;
  m_AssetTransformDependencies = rhs.m_AssetTransformDependencies;
  m_RuntimeDependencies        = rhs.m_RuntimeDependencies;
  m_Outputs                    = rhs.m_Outputs;
  m_sAssetsDocumentTypeName    = rhs.m_sAssetsDocumentTypeName;
  m_MetaInfo                   = std::move(rhs.m_MetaInfo);
}

void xiiAssetDocumentInfo::CreateShallowClone(xiiAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash             = m_uiSettingsHash;
  rhs.m_AssetTransformDependencies = m_AssetTransformDependencies;
  rhs.m_RuntimeDependencies        = m_RuntimeDependencies;
  rhs.m_Outputs                    = m_Outputs;
  rhs.m_sAssetsDocumentTypeName    = m_sAssetsDocumentTypeName;
  rhs.m_MetaInfo.Clear();
}

void xiiAssetDocumentInfo::ClearMetaData()
{
  for (auto* pObj : m_MetaInfo)
  {
    pObj->GetDynamicRTTI()->GetAllocator()->Deallocate(pObj);
  }
  m_MetaInfo.Clear();
}

const char* xiiAssetDocumentInfo::GetAssetsDocumentTypeName() const
{
  return m_sAssetsDocumentTypeName.GetData();
}

void xiiAssetDocumentInfo::SetAssetsDocumentTypeName(const char* sz)
{
  m_sAssetsDocumentTypeName.Assign(sz);
}

const xiiReflectedClass* xiiAssetDocumentInfo::GetMetaInfo(const xiiRTTI* pType) const
{
  for (auto* pObj : m_MetaInfo)
  {
    if (pObj->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pObj;
  }
  return nullptr;
}
