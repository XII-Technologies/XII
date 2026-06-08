/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/LightData.h>

const char* ToCompressionMode(xiiTextureConverterCompressionMode::Enum mode);
const char* ToMipmapMode(xiiTextureConverterMipmapMode::Enum mode);

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
    addEntry("{ ProjectDecalAtlas }", "Default/Decals.xiiBinTextureAtlas", "Decal Atlas");
  }
}

xiiString xiiDecalAssetDocumentManager::GetAssetTableEntry(const xiiSubAsset* pSubAsset, xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile) const
{
  // means NO table entry will be written, because for decals we don't need a redirection
  return xiiString();
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
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

xiiStatus xiiDecalAssetDocumentManager::GenerateDecalTexture(const xiiPlatformProfile* pAssetProfile)
{
  xiiAssetCurator* pCurator  = xiiAssetCurator::GetSingleton();
  const auto&      allAssets = pCurator->GetKnownSubAssets();

  xiiUInt64 uiAssetHash = 1;

  for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
  {
    const auto& asset = it.Value();

    if (asset.m_pAssetInfo->GetManager() != this)
      continue;

    uiAssetHash += pCurator->GetAssetDependencyHash(it.Key());
  }

  xiiStringBuilder sDecalFile = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  sDecalFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));

  if (IsDecalTextureUpToDate(sDecalFile, uiAssetHash))
    return XII_SUCCESS;

  xiiTextureAtlasCreationDesc atlasDesc;

  // find all decal assets, extract their file information to pass it along to TextureConverter
  {
    atlasDesc.m_Layers.SetCount(3);
    atlasDesc.m_Layers[0].m_Usage         = xiiTextureConverterUsage::Color;
    atlasDesc.m_Layers[1].m_Usage         = xiiTextureConverterUsage::NormalMap;
    atlasDesc.m_Layers[2].m_Usage         = xiiTextureConverterUsage::Linear;
    atlasDesc.m_Layers[2].m_uiNumChannels = 3;

    atlasDesc.m_Items.Reserve(64);

    xiiQtEditorApp*  pEditorApp = xiiQtEditorApp::GetSingleton();
    xiiStringBuilder sAbsPath;

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->GetManager() != this)
        continue;

      XII_LOG_BLOCK("Decal", asset.m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

      // does the document already exist and is it open ?
      bool         bWasOpen = false;
      xiiDocument* pDoc     = GetDocumentByPath(asset.m_pAssetInfo->m_Path.GetAbsolutePath());
      if (pDoc)
        bWasOpen = true;
      else
        pDoc = pEditorApp->OpenDocument(asset.m_pAssetInfo->m_Path.GetAbsolutePath(), xiiDocumentFlags::None);

      if (pDoc == nullptr)
        return xiiStatus(xiiFmt("Could not open asset document '{0}'", asset.m_pAssetInfo->m_Path.GetDataDirParentRelativePath()));

      xiiDecalAssetDocument* pDecalAsset = static_cast<xiiDecalAssetDocument*>(pDoc);

      {
        auto& item = atlasDesc.m_Items.ExpandAndGetRef();

        // store the GUID as the decal identifier
        xiiConversionUtils::ToString(pDecalAsset->GetGuid(), sAbsPath);
        item.m_uiUniqueID = xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sAbsPath));

        auto pDecalProps = pDecalAsset->GetProperties();
        item.m_uiFlags   = 0;
        item.m_uiFlags |= pDecalProps->NeedsNormal() ? DECAL_USE_NORMAL : 0;
        item.m_uiFlags |= pDecalProps->NeedsORM() ? DECAL_USE_ORM : 0;
        item.m_uiFlags |= pDecalProps->NeedsEmissive() ? DECAL_USE_EMISSIVE : 0;
        item.m_uiFlags |= pDecalProps->m_bBlendModeColorize ? DECAL_BLEND_MODE_COLORIZE : 0;

        if (!pDecalProps->m_sAlphaMask.IsEmpty())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sAlphaMask;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return xiiStatus(xiiFmt("Invalid alpha mask texture path '{0}'", sAbsPath));
          }

          item.m_sAlphaInput = sAbsPath;
        }

        if (pDecalProps->NeedsBaseColor())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sBaseColor;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return xiiStatus(xiiFmt("Invalid base color texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[0] = sAbsPath;
        }

        if (pDecalProps->NeedsNormal())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sNormal;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return xiiStatus(xiiFmt("Invalid normal texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[1] = sAbsPath;
        }

        if (pDecalProps->NeedsORM())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sORM;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return xiiStatus(xiiFmt("Invalid ORM texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }

        if (pDecalProps->NeedsEmissive())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sEmissive;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return xiiStatus(xiiFmt("Invalid emissive texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }
      }


      if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }
  }

  xiiAssetFileHeader header;
  {
    xiiUInt16 uiVersion = xiiGetStaticRTTI<xiiDecalAssetDocument>()->GetTypeVersion() & 0xFF;
    uiVersion |= (xiiGetStaticRTTI<xiiDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8;

    header.SetFileHashAndVersion(uiAssetHash, uiVersion);
  }

  xiiStatus result(XII_SUCCESS);

  // Send information to TextureConverter to do all the work
  {
    xiiStringBuilder texGroupFile = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    texGroupFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));
    texGroupFile.ChangeFileExtension("xiiDecalAtlasDesc");

    if (atlasDesc.Save(texGroupFile).Failed())
      return xiiStatus(xiiFmt("Failed to save texture atlas descriptor file '{0}'", texGroupFile));

    result = RunTextureConverter(sDecalFile, texGroupFile, header);
  }

  xiiFileStats stat;
  if (xiiOSFile::GetFileStats(sDecalFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TextureConverter crashed or had an error
    xiiOSFile::DeleteFile(sDecalFile).IgnoreResult();

    result = xiiStatus(xiiFmt("File does not exist: '{}'.", sDecalFile));
  }

  return result;
}

bool xiiDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, xiiUInt64 uiAssetHash) const
{
  xiiFileReader file;
  if (file.Open(szDecalFile).Succeeded())
  {
    xiiAssetFileHeader header;
    header.Read(file).IgnoreResult();

    // file still valid
    if (header.GetFileHash() == uiAssetHash)
      return true;
  }

  return false;
}

xiiString xiiDecalAssetDocumentManager::GetDecalTexturePath(const xiiPlatformProfile* pAssetProfile0) const
{
  const xiiPlatformProfile* pAssetProfile = xiiAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);
  xiiStringBuilder          result        = "Decals";
  GenerateOutputFilename(result, pAssetProfile, "xiiBinTextureAtlas", true);

  return result;
}

xiiStatus xiiDecalAssetDocumentManager::RunTextureConverter(const char* szTargetFile, const char* szInputFile, const xiiAssetFileHeader& AssetHeader)
{
  QStringList      arguments;
  xiiStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << xiiConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const xiiUInt64 uiHash64     = AssetHeader.GetFileHash();
    const xiiUInt32 uiHashLow32  = uiHash64 & 0xFFFFFFFF;
    const xiiUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", xiiArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", xiiArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  arguments << "-type";
  arguments << "Atlas";

  arguments << "-compression";
  arguments << ToCompressionMode(xiiTextureConverterCompressionMode::High);

  arguments << "-mipmaps";
  arguments << ToMipmapMode(xiiTextureConverterMipmapMode::Linear);

  arguments << "-atlasDesc";
  arguments << QString(szInputFile);

  XII_SUCCEED_OR_RETURN(xiiQtEditorApp::GetSingleton()->ExecuteTool("xiiTextureConverter", arguments, 180, xiiLog::GetThreadLocalLogSystem()));

  return XII_SUCCESS;
}
