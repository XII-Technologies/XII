#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTypeScriptAssetDocument::xiiTypeScriptAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiTypeScriptAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::None)
{
}

void xiiTypeScriptAssetDocument::EditScript()
{
  xiiStringBuilder sTsPath(GetProperties()->m_sScriptFile);

  if (GetProperties()->m_sScriptFile.IsEmpty())
    return;

  if (!xiiFileSystem::ExistsFile(sTsPath))
  {
    CreateComponentFile(sTsPath);
  }

  xiiStringBuilder sTsFileAbsPath;
  if (xiiFileSystem::ResolvePath(sTsPath, &sTsFileAbsPath, nullptr).Failed())
    return;

  static_cast<xiiTypeScriptAssetDocumentManager*>(GetDocumentManager())->SetupProjectForTypeScript(false);

  CreateTsConfigFiles();

  {
    QStringList args;

    for (const auto& dd : xiiQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
    {
      xiiStringBuilder path;
      xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

      args.append(QString::fromUtf8(path, path.GetElementCount()));
    }

    args.append(sTsFileAbsPath.GetData());

    if (xiiQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      xiiQtUiServices::OpenFileInDefaultProgram(sTsFileAbsPath);
    }
  }

  {
    xiiTypeScriptAssetDocumentEvent e;
    e.m_Type      = xiiTypeScriptAssetDocumentEvent::Type::ScriptOpened;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void xiiTypeScriptAssetDocument::CreateComponentFile(const char* szFile)
{
  xiiStringBuilder sScriptFile = szFile;

  {
    xiiDataDirectoryType* pDataDir = nullptr;
    if (xiiFileSystem::ResolvePath(GetDocumentPath(), nullptr, nullptr, &pDataDir).Failed())
      return;

    sScriptFile.Prepend(pDataDir->GetRedirectedDataDirectoryPath(), "/");
  }

  const xiiStringBuilder sComponentName = xiiPathUtils::GetFileName(GetDocumentPath());

  if (sComponentName.IsEmpty())
    return;

  xiiStringBuilder sContent;

  {
    xiiFileReader fileIn;
    if (fileIn.Open(":plugins/TypeScript/NewComponent.ts").Succeeded())
    {
      sContent.ReadAll(fileIn);
      sContent.ReplaceAll("NewComponent", sComponentName.GetView());
      sContent.ReplaceAll("<PATH-TO-XII-TS>", "TypeScript/xii");
    }
  }

  {
    xiiFileWriter file;
    if (file.Open(sScriptFile).Succeeded())
    {
      file.WriteBytes(sContent.GetData(), sContent.GetElementCount()).IgnoreResult();
    }
  }

  {
    xiiTypeScriptAssetDocumentEvent e;
    e.m_Type      = xiiTypeScriptAssetDocumentEvent::Type::ScriptCreated;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void xiiTypeScriptAssetDocument::CreateTsConfigFiles()
{
  for (const auto& dd : xiiQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    if (dd.m_sRootName.IsEqual_NoCase("BASE"))
      continue;

    xiiStringBuilder path;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();
    path.MakeCleanPath();

    CreateTsConfigFile(path).IgnoreResult();
  }
}

xiiResult xiiTypeScriptAssetDocument::CreateTsConfigFile(const char* szDirectory)
{
  xiiStringBuilder sTsConfig;
  xiiStringBuilder sTmp;

  for (xiiUInt32 iPlus1 = xiiQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs.GetCount(); iPlus1 > 0; --iPlus1)
  {
    const auto& dd = xiiQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs[iPlus1 - 1];

    xiiStringBuilder path;
    XII_SUCCEED_OR_RETURN(xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path));
    path.MakeCleanPath();
    path.AppendPath("*");

    sTmp.AppendWithSeparator(", ", "\"", path, "\"");
  }

  sTsConfig.Format(
    R"({
  "compilerOptions": {
    "target": "es5",
    "baseUrl": "",
    "paths": {
      "*": [{0}]
    }    
  }
}
)",
    sTmp);


  {
    sTmp = szDirectory;
    sTmp.AppendPath("tsconfig.json");

    xiiFileWriter file;
    XII_SUCCEED_OR_RETURN(file.Open(sTmp));
    XII_SUCCEED_OR_RETURN(file.WriteBytes(sTsConfig.GetData(), sTsConfig.GetElementCount()));
  }

  {
    sTmp = szDirectory;
    sTmp.AppendPath(".gitignore");

    xiiQtUiServices::AddToGitIgnore(sTmp, "tsconfig.json").IgnoreResult();
  }

  return XII_SUCCESS;
}

void xiiTypeScriptAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);

  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

xiiTransformStatus xiiTypeScriptAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_SUCCEED_OR_RETURN(ValidateScriptCode());
  XII_SUCCEED_OR_RETURN(AutoGenerateVariablesCode());

  xiiStringBuilder sTypeName = xiiPathUtils::GetFileName(GetDocumentPath());
  stream << sTypeName;

  const xiiUuid& docGuid = GetGuid();
  stream << docGuid;

  {
    xiiTypeScriptAssetDocumentEvent e;
    e.m_Type      = xiiTypeScriptAssetDocumentEvent::Type::ScriptTransformed;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }

  xiiTypeScriptAssetDocumentManager* pAssMan = static_cast<xiiTypeScriptAssetDocumentManager*>(GetAssetDocumentManager());
  XII_SUCCEED_OR_RETURN(pAssMan->GenerateScriptCompendium(transformFlags));

  return xiiTransformStatus();
}

xiiStatus xiiTypeScriptAssetDocument::ValidateScriptCode()
{
  xiiStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  xiiStringBuilder content;

  // read typescript file content
  {
    xiiFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return xiiStatus(xiiFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  // validate that the class with the correct name exists
  {
    xiiStringBuilder sClass;
    sClass = "class ";
    sClass.Append(sTsDocPath.GetFileName());
    sClass.Append(" extends");

    if (content.FindSubString(sClass) == nullptr)
    {
      return xiiStatus(xiiFmt("Sub-string '{}' not found. Class name may be incorrect.", sClass));
    }
  }

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiTypeScriptAssetDocument::AutoGenerateVariablesCode()
{
  xiiStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  xiiStringBuilder content;

  // read typescript file content
  {
    xiiFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return xiiStatus(xiiFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  const char* szTagBegin = "/* BEGIN AUTO-GENERATED: VARIABLES */";
  const char* szTagEnd   = "/* END AUTO-GENERATED: VARIABLES */";

  const char* szBeginAG = content.FindSubString(szTagBegin);

  if (szBeginAG == nullptr)
  {
    return xiiStatus(xiiFmt("'{}' tag is missing or corrupted.", szTagBegin));
  }

  const char* szEndAG = content.FindSubString(szTagEnd, szBeginAG);


  if (szEndAG == nullptr)
  {
    return xiiStatus(xiiFmt("'{}' tag is missing or corrupted.", szTagEnd));
  }

  xiiStringBuilder sAutoGen;

  // create code for exposed parameters
  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      sAutoGen.AppendFormat("    {}: number = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      sAutoGen.AppendFormat("    {}: boolean = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_StringParameters)
    {
      sAutoGen.AppendFormat("    {}: string = \"{}\";\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      sAutoGen.AppendFormat("    {}: xii.Vec3 = new xii.Vec3({}, {}, {});\n", p.m_sName, p.m_DefaultValue.x, p.m_DefaultValue.y, p.m_DefaultValue.z);
    }
    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      sAutoGen.AppendFormat("    {}: xii.Color = new xii.Color({}, {}, {}, {});\n", p.m_sName, p.m_DefaultValue.r, p.m_DefaultValue.g, p.m_DefaultValue.b, p.m_DefaultValue.a);
    }
  }

  // write back the modified file
  {
    sAutoGen.Prepend(szTagBegin, "\n");
    sAutoGen.Append("    ", szTagEnd);

    content.ReplaceSubString(szBeginAG, szEndAG + xiiStringUtils::GetStringElementCount(szTagEnd), sAutoGen);

    xiiFileWriter tsWriteBack;
    if (tsWriteBack.Open(sTsDocPath).Failed())
    {
      return xiiStatus(xiiFmt("Could not update .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    XII_SUCCEED_OR_RETURN(tsWriteBack.WriteBytes(content.GetData(), content.GetElementCount()));
  }

  return xiiStatus(XII_SUCCESS);
}

void xiiTypeScriptAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (bFirstTimeCreation)
  {
    auto history = GetCommandHistory();
    history->StartTransaction("Initial Setup");

    if (GetProperties()->m_sScriptFile.IsEmpty())
    {
      xiiStringBuilder sDefaultFile = GetDocumentPath();
      sDefaultFile.ChangeFileExtension("ts");
      xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sDefaultFile);

      xiiSetObjectPropertyCommand propCmd;
      propCmd.m_Object    = GetPropertyObject()->GetGuid();
      propCmd.m_sProperty = "ScriptFile";
      propCmd.m_NewValue  = xiiString(sDefaultFile);
      XII_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    history->FinishTransaction();

    const xiiString& sTsPath = GetProperties()->m_sScriptFile;

    if (!xiiFileSystem::ExistsFile(sTsPath))
    {
      CreateComponentFile(sTsPath);
    }
  }
}
