/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
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
  }
  xiiVariantArray types;
  for (auto it = xiiDocumentManager::GetAllDocumentDescriptors().GetIterator(); it.IsValid() && types.GetCount() < 128; ++it)
  {
    types.PushBack(it.Key());
  }
  out_result["documentTypes"] = std::move(types);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterProject()
{
  const auto read   = xiiMCPPermission::Read;
  const auto sEmpty = R"({"type":"object","additionalProperties":false})";
  if (m_Registry.RegisterTool("editor.project", "Describe the current XII project, manifest, active asset profile and engine-process readiness.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::Project, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
