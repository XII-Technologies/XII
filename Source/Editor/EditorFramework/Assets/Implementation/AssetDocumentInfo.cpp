#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocumentInfo, 2, xiiRTTIDefaultAllocator<xiiAssetDocumentInfo>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_SET_MEMBER_PROPERTY("Dependencies", m_TransformDependencies),
    XII_SET_MEMBER_PROPERTY("References", m_ThumbnailDependencies),
    XII_SET_MEMBER_PROPERTY("PackageDeps", m_PackageDependencies),
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
  m_uiSettingsHash          = rhs.m_uiSettingsHash;
  m_TransformDependencies   = rhs.m_TransformDependencies;
  m_ThumbnailDependencies   = rhs.m_ThumbnailDependencies;
  m_PackageDependencies     = rhs.m_PackageDependencies;
  m_Outputs                 = rhs.m_Outputs;
  m_sAssetsDocumentTypeName = rhs.m_sAssetsDocumentTypeName;
  m_MetaInfo                = std::move(rhs.m_MetaInfo);
}

void xiiAssetDocumentInfo::CreateShallowClone(xiiAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash          = m_uiSettingsHash;
  rhs.m_TransformDependencies   = m_TransformDependencies;
  rhs.m_ThumbnailDependencies   = m_ThumbnailDependencies;
  rhs.m_PackageDependencies     = m_PackageDependencies;
  rhs.m_Outputs                 = m_Outputs;
  rhs.m_sAssetsDocumentTypeName = m_sAssetsDocumentTypeName;
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

void xiiAssetDocumentInfo::SetAssetsDocumentTypeName(const char* szSz)
{
  m_sAssetsDocumentTypeName.Assign(szSz);
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
