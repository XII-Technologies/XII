/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <ToolsFoundation/Project/ToolsProject.h>

using namespace xiiEditorMCPArguments;

xiiStatus xiiEditorMCP::Project(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
{
  const auto* pProject     = xiiToolsProject::GetSingleton();
  out_result["ready"]      = xiiToolsProject::IsProjectOpen();
  out_result["generation"] = xiiToolsProject::GetProjectGeneration();
  if (pProject != nullptr)
  {
    const auto& manifest                = pProject->GetManifest();
    out_result["id"]                    = FormatGuid(manifest.m_ProjectID);
    out_result["name"]                  = manifest.m_sName;
    out_result["description"]           = manifest.m_sDescription;
    out_result["projectFile"]           = pProject->GetProjectFile();
    out_result["projectDirectory"]      = pProject->GetProjectDirectory();
    out_result["dataDirectoriesConfig"] = manifest.m_sDataDirectories;
    out_result["runtimePluginsConfig"]  = manifest.m_sRuntimePlugins;
    out_result["assetProfilesConfig"]   = manifest.m_sAssetProfiles;
    out_result["defaultScene"]          = manifest.m_sDefaultScene;
  }
  if (const auto* pEngine = xiiEditorEngineProcessConnection::GetSingleton())
  {
    out_result["engineProcessReady"] = pEngine->IsEngineSetup() && !pEngine->IsProcessCrashed();
  }
  if (const auto* pCurator = xiiAssetCurator::GetSingleton(); pCurator != nullptr && pCurator->IsReady())
  {
    if (const auto* pProfile = pCurator->GetActiveAssetProfile())
    {
      out_result["assetProfile"] = xiiString(pProfile->GetConfigName());
    }
    xiiVariantArray profiles;
    for (xiiUInt32 uiIndex = 0; uiIndex < pCurator->GetNumAssetProfiles() && uiIndex < 64; ++uiIndex)
    {
      if (const auto* pProfile = pCurator->GetAssetProfile(uiIndex))
      {
        xiiVariantDictionary profile;
        profile["name"]     = xiiString(pProfile->GetConfigName());
        profile["platform"] = xiiString(pProfile->GetTargetPlatform());
        profiles.PushBack(profile);
      }
    }
    out_result["assetProfiles"] = profiles;
  }
  if (const auto* pEditor = xiiQtEditorApp::GetSingleton(); pEditor != nullptr && xiiToolsProject::IsProjectOpen())
  {
    xiiVariantArray bundles;
    for (const auto& bundle : pEditor->GetPluginBundles().m_Plugins)
    {
      if (bundles.GetCount() == 128)
      {
        out_result["pluginBundlesTruncated"] = true;
        break;
      }
      xiiVariantDictionary row;
      row["id"]        = bundle.Key();
      row["name"]      = bundle.Value().m_sDisplayName;
      row["selected"]  = bundle.Value().m_bSelected;
      row["mandatory"] = bundle.Value().m_bMandatory;
      row["missing"]   = bundle.Value().m_bMissing;
      bundles.PushBack(row);
    }
    out_result["pluginBundles"] = bundles;
  }
  xiiVariantArray types;
  xiiVariantArray creatable;
  for (auto it = xiiDocumentManager::GetAllDocumentDescriptors().GetIterator(); it.IsValid() && types.GetCount() < 128; ++it)
  {
    types.PushBack(it.Key());
    if (const auto* pType = it.Value(); pType->m_bCanCreate)
    {
      xiiVariantDictionary row;
      row["type"]      = pType->m_sDocumentTypeName;
      row["extension"] = pType->m_sFileExtension;
      creatable.PushBack(std::move(row));
    }
  }
  out_result["documentTypes"]          = std::move(types);
  out_result["creatableDocumentTypes"] = std::move(creatable);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::CppProject(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
{
  if (!xiiToolsProject::IsProjectOpen())
  {
    return xiiStatus("Open a project before inspecting its C++ integration.");
  }
  out_result["hasBuildFiles"]           = xiiCppProject::ExistsProjectCMakeListsTxt();
  out_result["sdkCompiler"]             = xiiString(xiiCppProject::CompilerToString(xiiCppProject::GetSdkCompiler()));
  out_result["sdkCompilerMajorVersion"] = xiiCppProject::GetSdkCompilerMajorVersion();
  xiiCppSettings settings;
  if (settings.Load().Failed() || !xiiCppSettings::IsValidPluginName(settings.m_sPluginName))
  {
    out_result["configured"] = false;
    return XII_SUCCESS;
  }
  out_result["configured"]      = true;
  out_result["plugin"]          = settings.m_sPluginName;
  out_result["sourceDirectory"] = xiiCppProject::GetPluginSourceDir(settings);
  out_result["buildDirectory"]  = xiiCppProject::GetBuildDir(settings);
  out_result["outputDirectory"] = xiiCppProject::GetPluginOutputDir();
  out_result["generator"]       = xiiCppProject::GetCMakeGeneratorName(settings);
  out_result["solutionExists"]  = xiiCppProject::ExistsSolution(settings);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterProject()
{
  const auto read   = xiiMCPPermission::Read;
  const auto sEmpty = R"({"type":"object","additionalProperties":false})";
  if (m_Registry.RegisterTool("editor.project", "Describe the current project, available asset profiles, plugin configuration and engine-process readiness.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::Project, this)).Failed() ||
      m_Registry.RegisterTool("editor.cpp_project", "Inspect configured C++ plugin source, generated solution, compiler and build directories.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::CppProject, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
