#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionAssetEntry, 1, xiiRTTIDefaultAllocator<xiiCollectionAssetEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sLookupName),
    XII_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new xiiAssetBrowserAttribute(""))
  }
    XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionAssetData, 1, xiiRTTIDefaultAllocator<xiiCollectionAssetData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCollectionAssetDocument::xiiCollectionAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiCollectionAssetData>(szDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiCollectionAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiCollectionAssetData* pProp = GetProperties();

  xiiCollectionResourceDescriptor desc;
  xiiCollectionEntry              entry;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    xiiAssetCurator::xiiLockedSubAsset pInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(e.m_sRedirectionAsset);

    if (pInfo == nullptr)
    {
      xiiLog::Warning("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset);
      continue;
    }

    entry.m_sOptionalNiceLookupName = e.m_sLookupName;
    entry.m_sResourceID             = e.m_sRedirectionAsset;
    entry.m_sAssetTypeName          = pInfo->m_Data.m_sSubAssetsDocumentTypeName;

    desc.m_Resources.PushBack(entry);
  }

  desc.Save(stream);

  return xiiStatus(XII_SUCCESS);
}
