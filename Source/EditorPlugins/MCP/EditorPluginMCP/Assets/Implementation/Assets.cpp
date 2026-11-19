/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <ToolsFoundation/Project/ToolsProject.h>

using namespace xiiEditorMCPArguments;

namespace
{
  const char* AssetState(xiiAssetInfo::TransformState state)
  {
    constexpr const char* Names[] = {"unknown", "upToDate", "needsImport", "needsTransform", "needsThumbnail", "transformError", "missingTransformDependency",
                                     "missingThumbnailDependency", "missingPackageDependency", "circularDependency"};
    static_assert(XII_ARRAY_SIZE(Names) == xiiAssetInfo::TransformState::COUNT);
    return state < xiiAssetInfo::TransformState::COUNT ? Names[state] : "unknown";
  }

  xiiVariantArray BoundedStrings(const xiiSet<xiiString>& values)
  {
    xiiVariantArray result;
    for (const auto& sValue : values)
    {
      if (result.GetCount() == 64)
      {
        break;
      }
      result.PushBack(sValue);
    }
    return result;
  }

  void DescribeAsset(const xiiAssetInfo& asset, xiiVariantDictionary& out_row)
  {
    const auto& info          = *asset.m_Info;
    out_row["id"]             = FormatGuid(info.m_DocumentID);
    out_row["path"]           = asset.m_Path.IsValid() ? xiiString(asset.m_Path.GetDataDirRelativePath()) : asset.m_Path.GetAbsolutePath();
    out_row["type"]           = xiiString(info.m_sAssetsDocumentTypeName.GetView());
    out_row["tags"]           = info.m_sAssetsDocumentTags;
    out_row["transformState"] = xiiString(AssetState(asset.m_TransformState));
  }
} // namespace

xiiStatus xiiEditorMCP::Assets(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto* pCurator = xiiAssetCurator::GetSingleton();
  xiiUInt64   uiOffset = 0, uiLimit = 0;
  if (!xiiToolsProject::IsProjectOpen() || pCurator == nullptr || !pCurator->IsReady() || !ReadPage(arguments, uiOffset, uiLimit))
  {
    return xiiStatus("A ready asset curator and a valid page range are required.");
  }

  const auto      sQuery = xiiMCPJson::GetString(arguments, "query");
  const auto      sType  = xiiMCPJson::GetString(arguments, "type");
  const auto      sTag   = xiiMCPJson::GetString(arguments, "tag");
  xiiVariantArray rows;
  xiiUInt64       uiMatched = 0;
  const auto      assets    = pCurator->GetKnownAssets();
  for (auto it : *assets)
  {
    if (context.HasExpired())
    {
      return xiiStatus("Asset search exceeded its deadline. Narrow the search or lower the page limit.");
    }
    const auto& asset = *it.Value();
    const auto& info  = *asset.m_Info;
    if ((!sQuery.IsEmpty() && asset.m_Path.GetAbsolutePath().FindSubString_NoCase(sQuery) == nullptr) ||
        (!sType.IsEmpty() && info.m_sAssetsDocumentTypeName.GetView() != sType) ||
        (!sTag.IsEmpty() && info.m_sAssetsDocumentTags.FindSubString_NoCase(sTag) == nullptr))
    {
      continue;
    }
    if (uiMatched++ < uiOffset)
    {
      continue;
    }
    if (rows.GetCount() == uiLimit)
    {
      out_result["nextOffset"] = uiOffset + uiLimit;
      break;
    }
    xiiVariantDictionary row;
    DescribeAsset(asset, row);
    rows.PushBack(std::move(row));
  }
  out_result["assets"] = std::move(rows);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::Asset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto* pCurator = xiiAssetCurator::GetSingleton();
  const auto  id       = ReadGuid(arguments, "asset");
  if (!xiiToolsProject::IsProjectOpen() || pCurator == nullptr || !pCurator->IsReady() || !id.IsValid())
  {
    return xiiStatus("Provide an indexed asset UUID in a ready project.");
  }
  const auto sId   = FormatGuid(id);
  const auto asset = pCurator->FindSubAsset(sId);
  if (!asset.isValid() || asset->m_pAssetInfo == nullptr)
  {
    return xiiStatus("The asset is no longer indexed.");
  }
  const auto& info = *asset->m_pAssetInfo;
  DescribeAsset(info, out_result);
  out_result["transformDependencies"] = BoundedStrings(info.m_Info->m_TransformDependencies);
  out_result["thumbnailDependencies"] = BoundedStrings(info.m_Info->m_ThumbnailDependencies);
  out_result["packageDependencies"]   = BoundedStrings(info.m_Info->m_PackageDependencies);
  out_result["outputTags"]            = BoundedStrings(info.m_Info->m_Outputs);
  xiiVariantArray logs;
  for (const auto& log : info.m_LogEntries)
  {
    if (logs.GetCount() >= 32)
    {
      break;
    }
    logs.PushBack(xiiString(log.m_sMsg.GetView().GetSubString(0, 512)));
  }
  out_result["transformMessages"] = std::move(logs);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::Transform(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiUInt64  uiGeneration = 0;
  const auto id           = ReadGuid(arguments, "asset");
  auto*      pProcessor   = xiiAssetProcessor::GetSingleton();
  if (!m_bAllowAssetProcessing || !xiiToolsProject::IsProjectOpen() || !ReadUnsigned(arguments, "generation", uiGeneration) || !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration) ||
      xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID != m_AllowedProject || pProcessor == nullptr || !id.IsValid())
  {
    return xiiStatus("Asset transformation requires an authorized ready project, current project generation and indexed asset UUID.");
  }
  const auto status = pProcessor->QueueTransformAssets(xiiArrayPtr<const xiiUuid>(&id, 1));
  if (status.Succeeded())
  {
    out_result["asset"]  = FormatGuid(id);
    out_result["queued"] = true;
  }
  return status;
}

xiiStatus xiiEditorMCP::OpenAsset(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiUInt64   uiGeneration = 0;
  const auto* pProject     = xiiToolsProject::GetSingleton();
  const auto* pCurator     = xiiAssetCurator::GetSingleton();
  auto*       pEditor      = xiiQtEditorApp::GetSingleton();
  const auto  id           = ReadGuid(arguments, "asset");
  if (!xiiToolsProject::IsProjectOpen() || pProject == nullptr || pProject->GetManifest().m_ProjectID != m_AllowedProject ||
      !ReadUnsigned(arguments, "generation", uiGeneration) || !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration) ||
      pCurator == nullptr || !pCurator->IsReady() || pEditor == nullptr || !id.IsValid())
  {
    return xiiStatus("Use the current generation of an authorized ready project and an indexed asset ID.");
  }

  xiiString sPath;
  {
    const auto asset = pCurator->FindSubAsset(FormatGuid(id));
    if (asset.isValid() && asset->m_pAssetInfo != nullptr)
    {
      sPath = asset->m_pAssetInfo->m_Path.GetAbsolutePath();
    }
  }
  if (sPath.IsEmpty() || !pProject->IsDocumentInAllowedRoot(sPath) || context.HasExpired())
  {
    return xiiStatus("The indexed asset document is unavailable in the authorized project.");
  }
  const auto* pDocument = pEditor->OpenDocument(sPath, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);
  if (pDocument == nullptr || !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration))
  {
    return xiiStatus("The Editor could not open the document in the current project generation.");
  }
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterAssets()
{
  const auto read    = xiiMCPPermission::Read;
  const auto edit    = xiiMCPPermission::Edit;
  const auto execute = xiiMCPPermission::Execute;
  if (m_Registry.RegisterTool("editor.assets", "Search indexed assets by data-relative path, type and tags with bounded pagination.", R"({"type":"object","properties":{"query":{"type":"string"},"type":{"type":"string"},"tag":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", read, xiiMakeDelegate(&xiiEditorMCP::Assets, this)).Failed() ||
      m_Registry.RegisterTool("editor.asset", "Inspect one indexed asset's transform state, dependencies, outputs and recent transform messages.", R"({"type":"object","required":["asset"],"properties":{"asset":{"type":"string"}}})", read, xiiMakeDelegate(&xiiEditorMCP::Asset, this)).Failed() ||
      m_Registry.RegisterTool("editor.open_asset", "Open an indexed asset document through the Editor. Use its returned session and revision for authoring.", R"({"type":"object","required":["asset","generation"],"properties":{"asset":{"type":"string"},"generation":{"type":"integer"}}})", edit, xiiMakeDelegate(&xiiEditorMCP::OpenAsset, this)).Failed() ||
      m_Registry.RegisterTool("editor.transform_asset", "Queue an indexed asset for transformation by the Editor's background asset processor.", R"({"type":"object","required":["asset","generation"],"properties":{"asset":{"type":"string"},"generation":{"type":"integer"}}})", execute, xiiMakeDelegate(&xiiEditorMCP::Transform, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
