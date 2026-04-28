/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
    XII_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new xiiAssetBrowserAttribute("", "*", xiiDependencyFlags::Package))
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

xiiCollectionAssetDocument::xiiCollectionAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiCollectionAssetData>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

static bool InsertEntry(xiiStringView sID, xiiStringView sLookupName, xiiMap<xiiString, xiiCollectionEntry>& inout_found)
{
  auto it = inout_found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return true;
  }

  xiiStringBuilder                   tmp;
  xiiAssetCurator::xiiLockedSubAsset pInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    // this happens for non-asset types (e.g. 'xyz.color' and other non-asset file types)
    // these are benign and can just be skipped
    return false;
  }

  // insert item itself
  {
    xiiCollectionEntry& entry       = inout_found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID             = sID;
    entry.m_sAssetTypeName          = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // insert dependencies
  {
    const xiiAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const xiiString& doc : pDocInfo->m_PackageDependencies)
    {
      // ignore return value, we are only interested in top-level information
      InsertEntry(doc, {}, inout_found);
    }
  }

  return true;
}

xiiTransformStatus xiiCollectionAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiCollectionAssetData* pProp = GetProperties();

  xiiMap<xiiString, xiiCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    if (!InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries))
    {
      // this should be treated as an error for top-level references, since they are manually added (in contrast to the transitive dependencies)
      return xiiStatus(xiiFmt("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset));
    }
  }

  xiiCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return XII_SUCCESS;
}
