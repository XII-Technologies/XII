#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiTypeScriptAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTypeScriptAssetDocumentManager::xiiTypeScriptAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "TypeScript";
  m_DocTypeDesc.m_sFileExtension    = "xiiTypeScriptAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/TypeScript.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiTypeScriptAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Code_TypeScript");

  // Typescript doesn't fully work with the new scripting infrastructure yet. Uncomment at your own risk.
  // m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiTypeScriptRes";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::None;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("TypeScript", QPixmap(":/AssetIcons/TypeScript.png"));

  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  xiiGameObjectDocument::s_GameObjectDocumentEvents.AddEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));

  // make sure the preferences exist
  xiiPreferences::QueryPreferences<xiiTypeScriptPreferences>();
}

xiiTypeScriptAssetDocumentManager::~xiiTypeScriptAssetDocumentManager()
{
  ShutdownTranspiler();

  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  xiiGameObjectDocument::s_GameObjectDocumentEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));
}

void xiiTypeScriptAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTypeScriptAssetDocument>())
      {
        xiiQtTypeScriptAssetDocumentWindow* pDocWnd = new xiiQtTypeScriptAssetDocumentWindow(static_cast<xiiTypeScriptAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiTypeScriptAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiTypeScriptAssetDocument(szPath);
}

void xiiTypeScriptAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void xiiTypeScriptAssetDocumentManager::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectOpened)
  {
    InitializeTranspiler();
  }

  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectClosing)
  {
    ShutdownTranspiler();
  }
}

void xiiTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler(const xiiGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      if (xiiPreferences::QueryPreferences<xiiTypeScriptPreferences>()->m_bAutoUpdateScriptsForSimulation)
      {
        GenerateScriptCompendium(xiiTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    case xiiGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case xiiGameObjectDocumentEvent::Type::GameMode_StartingExternal:
    {
      if (xiiPreferences::QueryPreferences<xiiTypeScriptPreferences>()->m_bAutoUpdateScriptsForPlayTheGame)
      {
        GenerateScriptCompendium(xiiTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    default:
      break;
  }
}

void xiiTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation(xiiStringBuilder& content)
{
  const char* szTagBegin = "XII_DECLARE_MESSAGE_TYPE;";

  xiiUInt32        uiContinueAfterOffset = 0;
  xiiStringBuilder sAutoGen;

  while (true)
  {

    const char* szBeginAG = content.FindSubString(szTagBegin, content.GetData() + uiContinueAfterOffset);

    if (szBeginAG == nullptr)
    {
      break;
    }

    const char* szClassAG = content.FindLastSubString("class", szBeginAG);
    if (szClassAG == nullptr)
    {
      // return xiiStatus(xiiFmt("'{}' tag is incorrectly placed.", szBeginAG));
      return;
    }

    xiiUInt32 uiTypeNameHash = 0;

    {
      xiiStringView classNameView(szClassAG + 5, szBeginAG);
      classNameView.Trim(" \t\n\r");

      xiiStringBuilder sClassName;

      xiiStringIterator classNameIt = classNameView.GetIteratorFront();
      while (classNameIt.IsValid() && !xiiStringUtils::IsIdentifierDelimiter_C_Code(classNameIt.GetCharacter()))
      {
        sClassName.Append(classNameIt.GetCharacter());
        ++classNameIt;
      }

      if (sClassName.IsEmpty())
      {
        // return xiiStatus("Message class name not found.");
        return;
      }

      uiTypeNameHash = xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sClassName.GetData()));
    }

    sAutoGen.Format("public static GetTypeNameHash(): number { return {0}; }\nconstructor() { super(); this.TypeNameHash = {0}; }\n", uiTypeNameHash);

    uiContinueAfterOffset = szBeginAG - content.GetData();

    content.ReplaceSubString(szBeginAG, szBeginAG + xiiStringUtils::GetStringElementCount(szTagBegin), sAutoGen);
  }
}

void xiiTypeScriptAssetDocumentManager::InitializeTranspiler()
{
  if (m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = true;

  if (xiiFileSystem::FindDataDirectoryWithRoot("TypeScript") == nullptr)
  {
    xiiFileSystem::AddDataDirectory(">sdk/Data/Tools/xiiEditor/Typescript", "TypeScript", "TypeScript").IgnoreResult();
  }

  m_Transpiler.SetOutputFolder(":project/AssetCache/Temp");
  m_Transpiler.StartLoadTranspiler();
  m_Transpiler.SetModifyTsBeforeTranspilationCallback(&xiiTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation);
}

void xiiTypeScriptAssetDocumentManager::ShutdownTranspiler()
{
  if (!m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = false;

  m_Transpiler.FinishLoadTranspiler();
}

void xiiTypeScriptAssetDocumentManager::SetupProjectForTypeScript(bool bForce)
{
  if (m_bProjectSetUp && !bForce)
    return;

  m_bProjectSetUp = true;

  if (xiiTypeScriptBinding::SetupProjectCode().Failed())
  {
    xiiLog::Error("Could not setup Typescript data in project directory");
    return;
  }
}

xiiResult xiiTypeScriptAssetDocumentManager::GenerateScriptCompendium(xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_LOG_BLOCK("Generating Script Compendium");

  xiiHybridArray<xiiAssetInfo*, 256> allTsAssets;

  // keep this locked until the end of the function
  xiiAssetCurator::xiiLockedSubAssetTable   AllAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiHashTable<xiiUuid, xiiSubAsset>& AllAssets       = *AllAssetsLocked;

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const xiiSubAsset* pSub = &it.Value();

    if (pSub->m_pAssetInfo->GetManager() == this)
    {
      allTsAssets.PushBack(pSub->m_pAssetInfo);
    }
  }

  if (allTsAssets.IsEmpty())
  {
    xiiLog::Debug("Skipping script compendium creation - no TypeScript assets in project.");
    return XII_SUCCESS;
  }


  SetupProjectForTypeScript(false);

  // read m_CheckedTsFiles cache
  if (m_CheckedTsFiles.IsEmpty())
  {
    xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    xiiFileReader file;
    if (file.Open(sFile).Succeeded())
    {
      file.ReadMap(m_CheckedTsFiles).IgnoreResult();
    }
  }

  xiiMap<xiiString, xiiUInt32> relPathToDataDirIdx;

  xiiScriptCompendiumResourceDesc compendium;
  bool                            bAnythingNew = false;

  for (xiiUInt32 ddIdx = 0; ddIdx < xiiFileSystem::GetNumDataDirectories(); ++ddIdx)
  {
    xiiStringBuilder sDataDirPath = xiiFileSystem::GetDataDirectory(ddIdx)->GetRedirectedDataDirectoryPath();
    sDataDirPath.MakeCleanPath();

    if (!sDataDirPath.IsAbsolutePath())
      continue;

    xiiFileSystemIterator fsIt;
    fsIt.StartSearch(sDataDirPath, xiiFileSystemIteratorFlags::ReportFilesRecursive);

    xiiStringBuilder sTsFilePath;
    for (; fsIt.IsValid(); fsIt.Next())
    {
      if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".ts"))
        continue;

      fsIt.GetStats().GetFullPath(sTsFilePath);

      sTsFilePath.MakeRelativeTo(sDataDirPath).IgnoreResult();

      relPathToDataDirIdx[sTsFilePath] = ddIdx;

      compendium.m_PathToSource.Insert(sTsFilePath, xiiString());

      xiiTimestamp& lastModification = m_CheckedTsFiles[sTsFilePath];

      if (!lastModification.Compare(fsIt.GetStats().m_LastModificationTime, xiiTimestamp::CompareMode::FileTimeEqual))
      {
        bAnythingNew     = true;
        lastModification = fsIt.GetStats().m_LastModificationTime;
      }
    }
  }

  xiiStringBuilder sOutFile(":project/AssetCache/Common/Scripts.xiiScriptCompendium");

  if (!transformFlags.IsSet(xiiTransformFlags::ForceTransform))
  {
    if (bAnythingNew == false && xiiFileSystem::ExistsFile(sOutFile))
      return XII_SUCCESS;
  }

  xiiMap<xiiString, xiiString> filenameToSourceTsPath;

  xiiProgressRange progress("Transpiling Scripts", compendium.m_PathToSource.GetCount(), true);

  // remove the output file, so that if anything fails from here on out, it will be re-generated next time
  xiiFileSystem::DeleteFile(sOutFile);

  xiiStringBuilder sFilename;

  // TODO: could multi-thread this, if we had multiple transpilers loaded
  {
    xiiStringBuilder sOutputFolder;

    xiiStringBuilder sTranspiledJs;
    for (auto it : compendium.m_PathToSource)
    {
      if (!progress.BeginNextStep(it.Key()))
        return XII_FAILURE;

      sOutputFolder = xiiFileSystem::GetDataDirectory(relPathToDataDirIdx[it.Key()])->GetRedirectedDataDirectoryPath();
      sOutputFolder.MakeCleanPath();
      sOutputFolder.AppendPath("AssetCache/Temp");
      m_Transpiler.SetOutputFolder(sOutputFolder);

      if (m_Transpiler.TranspileFileAndStoreJS(it.Key(), sTranspiledJs).Failed())
      {
        xiiLog::Error("Failed to transpile '{}'", it.Key());
        return XII_FAILURE;
      }

      it.Value() = sTranspiledJs;

      sFilename                         = xiiPathUtils::GetFileName(it.Key());
      filenameToSourceTsPath[sFilename] = it.Key();
    }
  }

  // at runtime we need to be able to load a typescript component
  // at edit time, the xiiTypeScriptComponent should present the component type as a reference to an asset document
  // thus at edit time, this reference should look like a path to a document
  // however, at runtime we only need the name of the component type to instantiate (for the call to 'new' in Duktape/JS)
  // and the relative path to the source ts/js file (for the call to 'require' in Duktape/JS to 'load' the module)
  // just for these two strings we do not want to load an entire resource, as we would typically do with other asset types
  // therefore we extract the required data (component name and path) here and store it in the compendium
  // now all we need is the GUID of the TypeScript asset to look up this information at runtime
  // thus the xiiTypeScriptComponent does not need to store the asset document reference as a full string (path), but can just
  // store it as the GUID
  // at runtime this 'path' is not used as a xiiResource path/id, as would be common, but is used to look up the information
  // directly from the compendium
  {
    for (auto pAssetInfo : allTsAssets)
    {
      const xiiString& docPath = pAssetInfo->m_sDataDirParentRelativePath;
      const xiiUuid&   docGuid = pAssetInfo->m_Info->m_DocumentID;

      sFilename = xiiPathUtils::GetFileName(docPath);

      // TODO: handle filenameToSourceTsPath[sFilename] == "" case (log error)
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentTypeName = sFilename;
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentFilePath = filenameToSourceTsPath[sFilename];
    }
  }

  {
    xiiDeferredFileWriter file;
    file.SetOutput(sOutFile);

    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(1, 1);

    XII_SUCCEED_OR_RETURN(header.Write(file));

    XII_SUCCEED_OR_RETURN(compendium.Serialize(file));

    XII_SUCCEED_OR_RETURN(file.Close());
  }

  // write m_CheckedTsFiles cache
  {
    xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    xiiFileWriter file;
    if (file.Open(sFile).Succeeded())
    {
      XII_SUCCEED_OR_RETURN(file.WriteMap(m_CheckedTsFiles));
    }
  }

  return XII_SUCCESS;
}

xiiStatus xiiTypeScriptAssetDocumentManager::GetAdditionalOutputs(xiiDynamicArray<xiiString>& files)
{
  if (GenerateScriptCompendium(xiiTransformFlags::Default).Failed())
    return xiiStatus("Failed to build TypeScript compendium.");

  files.PushBack("AssetCache/Common/Scripts.xiiScriptCompendium");

  return xiiStatus(XII_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptPreferences, 1, xiiRTTIDefaultAllocator<xiiTypeScriptPreferences>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CompileForSimulation", m_bAutoUpdateScriptsForSimulation),
    XII_MEMBER_PROPERTY("CompileForPlayTheGame", m_bAutoUpdateScriptsForPlayTheGame),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTypeScriptPreferences::xiiTypeScriptPreferences() :
  xiiPreferences(xiiPreferences::Domain::Application, "TypeScript")
{
}
