/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <SharedPluginAssets/DecalAsset/DecalAtlasBuilder.h>
#include <SharedPluginAssets/TextureAsset/TextureAssetProfile.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

namespace
{
  xiiDecalAtlasBuildSettings GetBuildSettings(const xiiPlatformProfile* profile)
  {
    xiiDecalAtlasBuildSettings settings;
    if (const auto* textures = profile->GetTypeConfig<xiiTextureAssetProfileConfig>())
      settings.m_uiMaxEntrySize = xiiMath::Min(settings.m_uiMaxEntrySize, xiiUInt32(textures->m_uiMaxResolution));
    return settings;
  }

  xiiUInt16 GetAtlasAssetVersion()
  {
    return (xiiGetStaticRTTI<xiiDecalAssetDocument>()->GetTypeVersion() & 0xFF) |
      ((xiiGetStaticRTTI<xiiDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8);
  }
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiDecalAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDecalAssetDocumentManager::xiiDecalAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Decal";
  m_DocTypeDesc.m_sFileExtension    = "xiiDecalAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Decal.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Effects";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiDecalAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Decal");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinDecal";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiDecalAssetDocumentManager::~xiiDecalAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiDecalAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiDecalAssetDocumentManager::AddEntriesToAssetTable(xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile, xiiDelegate<void(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)> addEntry) const
{
  xiiStringBuilder projectDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  projectDir.MakeCleanPath();
  projectDir.Append("/");

  if (projectDir.StartsWith_NoCase(sDataDirectory))
  {
    addEntry("{ ProjectDecalAtlas }", GetDecalTexturePath(pAssetProfile), "Decal Atlas");
  }
}

void xiiDecalAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiDecalAssetDocument>())
      {
        new xiiQtDecalAssetDocumentWindow(static_cast<xiiDecalAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiDecalAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiDecalAssetDocument(sPath);
}

void xiiDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

xiiUInt64 xiiDecalAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const
{
  const auto            settings = GetBuildSettings(DetermineFinalTargetProfile(pAssetProfile));
  xiiHashStreamWriter64 hash(2); // Native packed atlas descriptor version.
  hash << settings.m_uiMaxEntrySize;
  hash << settings.m_uiMaxAtlasSize;
  hash << settings.m_uiMaxSourceSize;
  hash << settings.m_uiPadding;
  hash << settings.m_bGenerateMipMaps;
  return hash.GetHashValue();
}

xiiStatus xiiDecalAssetDocumentManager::GenerateDecalTexture(const xiiPlatformProfile* pAssetProfile)
{
  pAssetProfile = DetermineFinalTargetProfile(pAssetProfile);
  auto* curator = xiiAssetCurator::GetSingleton();
  struct Candidate
  {
    xiiUuid   m_Guid;
    xiiString m_sPath;
  };
  xiiDynamicArray<Candidate> candidates;
  {
    // Release the curator's inventory lock before opening documents or querying dependencies.
    const auto assets = curator->GetKnownSubAssets();
    for (auto it = assets->GetIterator(); it.IsValid(); ++it)
      if (it.Value().m_pAssetInfo->GetManager() == this)
        candidates.PushBack({it.Key(), it.Value().m_pAssetInfo->m_Path.GetAbsolutePath()});
  }
  candidates.Sort([](const Candidate& a, const Candidate& b) { return a.m_Guid < b.m_Guid; });
  xiiHashStreamWriter64 hash(ComputeAssetProfileHashImpl(pAssetProfile));
  for (const auto& candidate : candidates)
  {
    const auto dependencyHash = curator->GetAssetDependencyHash(candidate.m_Guid);
    if (dependencyHash == 0)
      return xiiStatus(xiiFmt("Cannot compute decal dependencies for '{}'.", candidate.m_sPath));
    hash << candidate.m_Guid;
    hash << dependencyHash;
  }
  xiiStringBuilder outputPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  outputPath.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));
  if (IsDecalTextureUpToDate(outputPath, hash.GetHashValue()))
    return XII_SUCCESS;

  xiiDynamicArray<xiiDecalAtlasSource> sources;
  auto*                                editor = xiiQtEditorApp::GetSingleton();
  for (const auto& candidate : candidates)
  {
    xiiDocument* document = GetDocumentByPath(candidate.m_sPath);
    const bool   wasOpen  = document != nullptr;
    if (!document)
      document = editor->OpenDocument(candidate.m_sPath, xiiDocumentFlags::None);
    if (!document)
      return xiiStatus(xiiFmt("Cannot open decal document '{}'.", candidate.m_sPath));
    XII_SCOPE_EXIT(if (!wasOpen && !document->HasWindowBeenRequested()) document->GetDocumentManager()->CloseDocument(document));
    auto*       decal      = static_cast<xiiDecalAssetDocument*>(document);
    const auto* properties = decal->GetProperties();
    if (properties->m_Mode < xiiDecalMode::BaseColor || properties->m_Mode > xiiDecalMode::BaseColorEmissive)
      return xiiStatus(xiiFmt("Decal '{}' uses an invalid mode.", candidate.m_sPath));
    if (properties->m_bBlendModeColorize)
      return xiiStatus(xiiFmt("Decal '{}' uses legacy colorize blending, which has no native rendering path. Its authored setting is preserved.", candidate.m_sPath));
    auto&            source = sources.ExpandAndGetRef();
    xiiStringBuilder id;
    xiiConversionUtils::ToString(decal->GetGuid(), id);
    source.m_sDecalId.Assign(id);
    auto resolve = [&](const xiiString& path, xiiString& absolute, bool required) -> xiiStatus {
      if (path.IsEmpty())
        return required ? xiiStatus(xiiFmt("Decal '{}' is missing a required image.", candidate.m_sPath)) : xiiStatus(XII_SUCCESS);
      xiiStringBuilder resolved = path;
      if (!editor->MakeDataDirectoryRelativePathAbsolute(resolved))
        return xiiStatus(xiiFmt("Cannot resolve decal image '{}'.", path));
      absolute = resolved;
      return XII_SUCCESS;
    };
    XII_SUCCEED_OR_RETURN(resolve(properties->m_sBaseColor, source.m_sAlbedo, true));
    XII_SUCCEED_OR_RETURN(resolve(properties->m_sAlphaMask, source.m_sAlphaMask, false));
    if (properties->NeedsNormal())
    {
      source.m_ChannelMask.Add(xiiDecalChannelMask::Normal);
      XII_SUCCEED_OR_RETURN(resolve(properties->m_sNormal, source.m_sNormal, true));
    }
    if (properties->NeedsORM())
    {
      source.m_ChannelMask.Add(xiiDecalChannelMask::Material);
      XII_SUCCEED_OR_RETURN(resolve(properties->m_sORM, source.m_sORM, true));
    }
    if (properties->NeedsEmissive())
    {
      source.m_ChannelMask.Add(xiiDecalChannelMask::Emissive);
      XII_SUCCEED_OR_RETURN(resolve(properties->m_sEmissive, source.m_sEmissive, true));
    }
  }
  xiiDecalAtlasResourceDescriptor atlas;
  XII_SUCCEED_OR_RETURN(xiiDecalAtlasBuilder::Build(sources, GetBuildSettings(pAssetProfile), atlas));
  xiiDeferredFileWriter writer;
  writer.SetOutput(outputPath, true);
  bool published = false;
  XII_SCOPE_EXIT(if (!published) writer.Discard());
  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(hash.GetHashValue(), GetAtlasAssetVersion());
  header.SetGenerator("Native Decal Atlas");
  XII_SUCCEED_OR_RETURN(header.Write(writer));
  XII_SUCCEED_OR_RETURN(atlas.Serialize(writer));
  XII_SUCCEED_OR_RETURN(writer.Close());
  published = true;
  // Individual decals now have their own asset table entries; reload the shared
  // manager resource explicitly when its active-profile pixels change.
  if (pAssetProfile == curator->GetActiveAssetProfile())
    if (auto* connection = xiiEditorEngineProcessConnection::GetSingleton())
    {
      xiiReloadResourceMsgToEngine reload;
      reload.m_sResourceID   = "{ ProjectDecalAtlas }";
      reload.m_sResourceType = "Decal Atlas";
      connection->SendMessage(&reload);
    }
  return XII_SUCCESS;
}

bool xiiDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, xiiUInt64 uiAssetHash) const
{
  xiiFileReader file;
  if (file.Open(szDecalFile).Failed() || file.GetFileSize() < 25)
    return false;
  char      tag[8]        = {};
  xiiUInt8  headerVersion = 0;
  xiiUInt64 hash          = 0;
  xiiUInt16 assetVersion = 0, descriptorVersion = 0;
  xiiUInt32 generatorLength = 0;
  if (file.ReadBytes(tag, sizeof(tag)) != sizeof(tag) || !xiiMemoryUtils::IsEqual(tag, "xiiAsset", sizeof(tag)) ||
      file.ReadBytes(&headerVersion, 1) != 1 || headerVersion != 3 ||
      file.ReadQWordValue(&hash).Failed() || file.ReadWordValue(&assetVersion).Failed() ||
      file.ReadDWordValue(&generatorLength).Failed() || generatorLength > file.GetFileSize() - 25 ||
      file.SkipBytes(generatorLength) != generatorLength || file.ReadWordValue(&descriptorVersion).Failed())
    return false;
  if (hash != uiAssetHash || assetVersion != GetAtlasAssetVersion() || descriptorVersion != 2)
    return false;
  // A failed previous disk write may have left a valid header and partial pixels.
  // Seek to the format's final marker instead of accepting that truncated cache.
  xiiOSFile raw;
  if (raw.Open(szDecalFile, xiiFileOpenMode::Read).Failed() || raw.GetFileSize() < 4)
    return false;
  raw.SetFilePosition(-4, xiiFileSeekMode::FromEnd);
  xiiUInt8 footer[4] = {};
  if (raw.Read(footer, sizeof(footer)) != sizeof(footer))
    return false;
  xiiRawMemoryStreamReader footerReader(footer, sizeof(footer));
  xiiUInt32                marker = 0;
  return footerReader.ReadDWordValue(&marker).Succeeded() && marker == 0xDECADA7A;
}

xiiString xiiDecalAssetDocumentManager::GetDecalTexturePath(const xiiPlatformProfile* pAssetProfile0) const
{
  const xiiPlatformProfile* pAssetProfile = xiiAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);
  xiiStringBuilder          result        = "Decals";
  GenerateOutputFilename(result, pAssetProfile, "xiiBinTextureAtlas", true);

  return result;
}
