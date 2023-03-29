#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocumentManager, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAssetDocumentManager::xiiAssetDocumentManager()  = default;
xiiAssetDocumentManager::~xiiAssetDocumentManager() = default;

xiiStatus xiiAssetDocumentManager::CloneDocument(const char* szPath, const char* szClonePath, xiiUuid& inout_cloneGuid)
{
  xiiStatus res = SUPER::CloneDocument(szPath, szClonePath, inout_cloneGuid);
  if (res.Succeeded())
  {
    // Cloned documents are usually opened right after cloning. To make sure this does not fail we need to inform the asset curator of the newly added asset document.
    xiiAssetCurator::GetSingleton()->NotifyOfFileChange(szClonePath);
  }
  return res;
}

void xiiAssetDocumentManager::ComputeAssetProfileHash(const xiiPlatformProfile* pAssetProfile)
{
  m_uiAssetProfileHash = ComputeAssetProfileHashImpl(DetermineFinalTargetProfile(pAssetProfile));

  if (GeneratesProfileSpecificAssets())
  {
    XII_ASSERT_DEBUG(m_uiAssetProfileHash != 0, "Assets that generate a profile-specific output must compute a hash for the profile settings.");
  }
  else
  {
    XII_ASSERT_DEBUG(m_uiAssetProfileHash == 0, "Only assets that generate per-profile outputs may specify an asset profile hash.");
    m_uiAssetProfileHash = 0;
  }
}

xiiUInt64 xiiAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const
{
  return 0;
}

xiiStatus xiiAssetDocumentManager::ReadAssetDocumentInfo(xiiUniquePtr<xiiAssetDocumentInfo>& out_pInfo, xiiStreamReader& stream) const
{
  xiiAbstractObjectGraph graph;

  if (xiiAbstractGraphDdlSerializer::ReadHeader(stream, &graph).Failed())
    return xiiStatus("Failed to read asset document");

  xiiRttiConverterContext context;
  xiiRttiConverterReader  rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");

  if (pHeaderNode == nullptr)
    return xiiStatus("Document does not contain a 'Header'");

  xiiAssetDocumentInfo* pEntry = rttiConverter.CreateObjectFromNode(pHeaderNode).Cast<xiiAssetDocumentInfo>();
  XII_ASSERT_DEBUG(pEntry != nullptr, "Failed to deserialize xiiAssetDocumentInfo!");
  out_pInfo = xiiUniquePtr<xiiAssetDocumentInfo>(pEntry, xiiFoundation::GetDefaultAllocator());
  return xiiStatus(XII_SUCCESS);
}

xiiString xiiAssetDocumentManager::GenerateResourceThumbnailPath(const char* szDocumentPath)
{
  xiiStringBuilder sProjectDir = xiiAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  xiiStringBuilder sRelativePath = szDocumentPath;

  sRelativePath.MakeRelativeTo(sProjectDir).IgnoreResult();
  sRelativePath.Append(".jpg");

  xiiStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

bool xiiAssetDocumentManager::IsThumbnailUpToDate(const char* szDocumentPath, xiiUInt64 uiThumbnailHash, xiiUInt32 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  xiiString     sThumbPath = GenerateResourceThumbnailPath(szDocumentPath);
  xiiFileReader file;
  if (file.Open(sThumbPath, 256).Failed())
    return false;

  xiiAssetDocument::ThumbnailInfo thumbnailInfo;

  const xiiUInt64 uiHeaderSize = thumbnailInfo.GetSerializedSize();
  xiiUInt64       uiFileSize   = file.GetFileSize();

  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);

  if (thumbnailInfo.Deserialize(file).Failed())
  {
    return false;
  }

  return thumbnailInfo.IsThumbnailUpToDate(uiThumbnailHash, uiTypeVersion);
}

void xiiAssetDocumentManager::AddEntriesToAssetTable(const char* szDataDirectory, const xiiPlatformProfile* pAssetProfile, xiiDelegate<void(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)> addEntry) const {}

xiiString xiiAssetDocumentManager::GetAssetTableEntry(const xiiSubAsset* pSubAsset, const char* szDataDirectory, const xiiPlatformProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor, szDataDirectory, pSubAsset->m_pAssetInfo->m_sAbsolutePath, "", pAssetProfile);
}

xiiString xiiAssetDocumentManager::GetAbsoluteOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, const char* szDocumentPath, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile) const
{
  xiiStringBuilder sProjectDir = xiiAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  xiiString        sRelativePath = GetRelativeOutputFileName(pTypeDesc, sProjectDir, szDocumentPath, szOutputTag, pAssetProfile);
  xiiStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

xiiString xiiAssetDocumentManager::GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile) const
{
  const xiiPlatformProfile* pPlatform = xiiAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
  XII_ASSERT_DEBUG(xiiStringUtils::IsNullOrEmpty(szOutputTag), "The output tag '%s' for '%s' is not supported, override GetRelativeOutputFileName", szOutputTag, szDocumentPath);

  xiiStringBuilder sRelativePath(szDocumentPath);
  sRelativePath.MakeRelativeTo(szDataDirectory).IgnoreResult();
  GenerateOutputFilename(sRelativePath, pPlatform, pTypeDesc->m_sResourceFileExtension, GeneratesProfileSpecificAssets());

  return sRelativePath;
}

bool xiiAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const xiiDynamicArray<xiiString>& outputs, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  CURATOR_PROFILE(szDocumentPath);
  if (!IsOutputUpToDate(szDocumentPath, "", uiHash, pTypeDescriptor))
    return false;

  for (const xiiString& sOutput : outputs)
  {
    if (!IsOutputUpToDate(szDocumentPath, sOutput, uiHash, pTypeDescriptor))
      return false;
  }
  return true;
}

bool xiiAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  const xiiString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, szDocumentPath, szOutputTag);
  return xiiAssetDocumentManager::IsResourceUpToDate(sTargetFile, uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());
}

const xiiPlatformProfile* xiiAssetDocumentManager::DetermineFinalTargetProfile(const xiiPlatformProfile* pAssetProfile)
{
  if (pAssetProfile == nullptr)
  {
    return xiiAssetCurator::GetSingleton()->GetActiveAssetProfile();
  }

  return pAssetProfile;
}

xiiResult xiiAssetDocumentManager::TryOpenAssetDocument(const char* szPathOrGuid)
{
  xiiAssetCurator::xiiLockedSubAsset pSubAsset;

  if (xiiConversionUtils::IsStringUuid(szPathOrGuid))
  {
    xiiUuid matGuid;
    matGuid = xiiConversionUtils::ConvertStringToUuid(szPathOrGuid);

    pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way
    // either left as an exercise for whoever needs non-asset references
    pSubAsset = xiiAssetCurator::GetSingleton()->FindSubAsset(szPathOrGuid);
  }

  if (pSubAsset)
  {
    xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(pSubAsset->m_pAssetInfo->m_sAbsolutePath);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

bool xiiAssetDocumentManager::IsResourceUpToDate(const char* szResourceFile, xiiUInt64 uiHash, xiiUInt16 uiTypeVersion)
{
  CURATOR_PROFILE(szResourceFile);
  xiiFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  // this might happen if writing to the file failed
  if (file.GetFileSize() == 0)
    return false;

  xiiAssetFileHeader AssetHeader;
  AssetHeader.Read(file).IgnoreResult();

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

void xiiAssetDocumentManager::GenerateOutputFilename(xiiStringBuilder& inout_sRelativeDocumentPath, const xiiPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
  {
    const xiiPlatformProfile* pPlatform = xiiAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
    inout_sRelativeDocumentPath.Prepend(pPlatform->GetConfigName(), "/");
  }
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
