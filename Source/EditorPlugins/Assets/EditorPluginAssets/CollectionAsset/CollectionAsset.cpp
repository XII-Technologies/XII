#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
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

static void InsertEntry(xiiStringView sID, xiiStringView sLookupName, xiiMap<xiiString, xiiCollectionEntry>& inout_Found)
{
  auto it = inout_Found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return;
  }

  xiiStringBuilder                   tmp;
  xiiAssetCurator::xiiLockedSubAsset pInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    xiiLog::Warning("Asset in Collection is unknown: '{0}'", sID);
    return;
  }

  // Insert item itself
  {
    xiiCollectionEntry& entry       = inout_Found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID             = sID;
    entry.m_sAssetTypeName          = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // Insert dependencies
  {
    const xiiAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const xiiString& doc : pDocInfo->m_AssetTransformDependencies)
    {
      InsertEntry(doc, {}, inout_Found);
    }

    for (const xiiString& doc : pDocInfo->m_RuntimeDependencies)
    {
      InsertEntry(doc, {}, inout_Found);
    }
  }
}

void xiiCollectionAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  // TODO: why are collections not marked as needs-transform, out of the box, when a dependency changes ?

  SUPER::UpdateAssetDocumentInfo(pInfo);

  const xiiCollectionAssetData* pProp = GetProperties();

  xiiMap<xiiString, xiiCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries);
  }

  for (auto it : entries)
  {
    pInfo->m_AssetTransformDependencies.Insert(it.Value().m_sResourceID);
  }
}

xiiTransformStatus xiiCollectionAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiCollectionAssetData* pProp = GetProperties();

  xiiMap<xiiString, xiiCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries);
  }

  xiiCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return xiiStatus(XII_SUCCESS);
}
