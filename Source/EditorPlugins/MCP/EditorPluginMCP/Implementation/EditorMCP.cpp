/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Core/World/World.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QCoreApplication>

namespace
{
  xiiEditorMCP* s_pEditorMCP = nullptr;

  xiiString FormatGuid(const xiiUuid& guid)
  {
    xiiStringBuilder sGuid;
    xiiConversionUtils::ToString(guid, sGuid);
    return xiiString(sGuid);
  }

  xiiUuid ReadGuid(const xiiVariantDictionary& arguments, xiiStringView sKey)
  {
    const auto sValue = xiiMCPJson::GetString(arguments, sKey);
    return xiiConversionUtils::IsStringUuid(sValue) ? xiiConversionUtils::ConvertStringToUuid(sValue) : xiiUuid();
  }

  bool ReadUnsigned(const xiiVariantDictionary& arguments, xiiStringView sKey, xiiUInt64& out_uiValue)
  {
    const auto* pValue = arguments.GetValue(sKey);
    return pValue != nullptr && xiiMCPJson::ReadUInt(*pValue, out_uiValue);
  }

  bool ReadPage(const xiiVariantDictionary& arguments, xiiUInt64& out_uiOffset, xiiUInt64& out_uiLimit)
  {
    out_uiOffset = 0;
    out_uiLimit  = 32;
    return (!arguments.Contains("offset") || ReadUnsigned(arguments, "offset", out_uiOffset)) &&
      (!arguments.Contains("limit") || ReadUnsigned(arguments, "limit", out_uiLimit)) &&
      out_uiLimit > 0 && out_uiLimit <= 128;
  }

  bool IsScene(const xiiDocument& document)
  {
    const auto sType = document.GetDocumentTypeName();
    return sType == "Scene" || sType == "Prefab" || sType == "Layer";
  }

  void DescribeDocument(const xiiDocument& document, const xiiUuid& session, xiiVariantDictionary& out_result)
  {
    out_result["id"]         = FormatGuid(document.GetGuid());
    out_result["session"]    = FormatGuid(session);
    out_result["revision"]   = document.GetModificationRevision();
    out_result["generation"] = xiiToolsProject::GetProjectGeneration();
    out_result["type"]       = xiiString(document.GetDocumentTypeName());
    out_result["path"]       = xiiString(document.GetDocumentPath());
    out_result["dirty"]      = document.IsModified();
    out_result["readOnly"]   = document.IsReadOnly();
  }

  xiiVariantDictionary DescribeDocumentObject(const xiiDocumentObject& object)
  {
    xiiVariantDictionary row;
    row["id"]             = FormatGuid(object.GetGuid());
    row["type"]           = xiiString(object.GetType()->GetTypeName());
    row["parentProperty"] = xiiString(object.GetParentProperty());
    row["childCount"]     = object.GetChildren().GetCount();
    return row;
  }

  const xiiDocumentObject* FindDocumentObject(xiiObjectAccessorBase& ref_accessor, const xiiVariantDictionary& arguments, xiiStringView sKey)
  {
    const auto id = ReadGuid(arguments, sKey);
    return id.IsValid() ? ref_accessor.GetObject(id) : nullptr;
  }

  xiiStatus ApplySceneCommand(xiiObjectAccessorBase& ref_accessor, const xiiVariantDictionary& operation, xiiVariantDictionary& out_row)
  {
    const auto  sAction = xiiMCPJson::GetString(operation, "action");
    const auto* pObject = FindDocumentObject(ref_accessor, operation, "object");
    if (sAction == "addObject" || sAction == "addComponent" || sAction == "reparent")
    {
      const auto* pParent    = FindDocumentObject(ref_accessor, operation, "parent");
      const bool  bComponent = sAction == "addComponent";
      if ((operation.Contains("parent") && pParent == nullptr) ||
          (pParent != nullptr && !pParent->GetType()->IsDerivedFrom<xiiGameObject>()) ||
          (bComponent && pParent == nullptr))
      {
        return xiiStatus("The destination must be a valid game object. A component requires a parent.");
      }
      const auto sProperty = pParent == nullptr ? "" : bComponent ? "Components" :
                                                                    "Children";
      if (sAction == "reparent")
      {
        if (pObject == nullptr || !pObject->GetType()->IsDerivedFrom<xiiGameObject>())
        {
          return xiiStatus("Only a game object can be reparented.");
        }
        return ref_accessor.MoveObjectByName(pObject, pParent, sProperty, xiiInt32(-1));
      }

      const xiiRTTI* pType = xiiGetStaticRTTI<xiiGameObject>();
      if (bComponent)
      {
        pType = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(operation, "type"));
        if (pType == nullptr || !pType->IsDerivedFrom<xiiComponent>() || pType->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
        {
          return xiiStatus("Provide a concrete component type.");
        }
      }
      auto id = ReadGuid(operation, "newId");
      if (operation.Contains("newId") && (!id.IsValid() || ref_accessor.GetObject(id) != nullptr))
      {
        return xiiStatus("The requested new object UUID is invalid or already in use.");
      }
      const auto status = ref_accessor.AddObjectByName(pParent, sProperty, xiiInt32(-1), pType, id);
      if (status.Succeeded())
      {
        out_row["id"] = FormatGuid(id);
      }
      return status;
    }

    if (pObject == nullptr || (!pObject->GetType()->IsDerivedFrom<xiiGameObject>() && !pObject->GetType()->IsDerivedFrom<xiiComponent>()))
    {
      return xiiStatus("Select a game object or component in the open scene.");
    }
    if (sAction == "remove")
    {
      return ref_accessor.RemoveObject(pObject);
    }
    if (sAction == "set")
    {
      const auto  sProperty = xiiMCPJson::GetString(operation, "property");
      const auto* pProperty = pObject->GetType()->FindPropertyByName(sProperty);
      const auto* pValue    = operation.GetValue("value");
      xiiVariant  converted;
      if (pProperty == nullptr || pValue == nullptr || !xiiMCPReflection::ConvertArgument(*pProperty, *pValue, converted))
      {
        return xiiStatus("The property is unavailable, read-only, or the JSON value does not match its supported type.");
      }
      return ref_accessor.SetValue(pObject, pProperty, converted);
    }
    return xiiStatus("Unknown scene edit action.");
  }

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

xiiEditorMCP::xiiEditorMCP() :
  m_Protocol(m_Registry),
  m_Http(m_Protocol)
{
}

xiiEditorMCP::~xiiEditorMCP()
{
  m_Timer.stop();
  if (m_DocumentEvents != 0)
  {
    xiiDocumentManager::s_Events.RemoveEventHandler(m_DocumentEvents);
  }
}

xiiStatus xiiEditorMCP::Start()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  m_DocumentEvents     = xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiEditorMCP::DocumentEvent, this));

  const auto sProject = pOptions->GetStringOption("-MCPProject");
  if (!sProject.IsEmpty())
  {
    if (!xiiConversionUtils::IsStringUuid(sProject))
    {
      return xiiStatus("Invalid -MCPProject UUID.");
    }
    m_AllowedProject = xiiConversionUtils::ConvertStringToUuid(sProject);
  }
  xiiUInt8 uiPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  if (m_AllowedProject.IsValid() && pOptions->GetBoolOption("-MCPAllowEdit"))
  {
    uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
  }
  if (m_AllowedProject.IsValid() && pOptions->GetBoolOption("-MCPAllowExecute"))
  {
    uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
  }

  const auto read      = xiiMCPPermission::Read;
  const auto edit      = xiiMCPPermission::Edit;
  const auto execute   = xiiMCPPermission::Execute;
  const auto sEmpty    = R"({"type":"object","additionalProperties":false})";
  const auto sDocument = R"({"type":"object","required":["document"],"properties":{"document":{"type":"string"},"object":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})";
  const auto sRevision = R"({"type":"object","required":["document","session","revision","generation"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"}}})";
  if (m_Registry.RegisterTool("editor.project", "Describe the current XII project, manifest, active asset profile and engine-process readiness.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::Project, this)).Failed() ||
      m_Registry.RegisterTool("editor.documents", "List open Editor documents, selection and revision tokens. Scene authoring requires a matching document session and revision.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::Documents, this)).Failed() ||
      m_Registry.RegisterTool("editor.scene", "Browse children of a scene, prefab or layer object and inspect its reflected members. Omit object for the root.", sDocument, read, xiiMakeDelegate(&xiiEditorMCP::Scene, this)).Failed() ||
      m_Registry.RegisterTool("editor.edit_scene", "Apply a bounded batch in one undoable transaction. Set, addObject, addComponent, remove and reparent are supported.", R"({"type":"object","required":["document","session","revision","generation","operations"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"},"operations":{"type":"array","minItems":1,"maxItems":24,"items":{"type":"object","required":["action"],"properties":{"action":{"enum":["addObject","addComponent","set","remove","reparent"]},"object":{"type":"string"},"newId":{"type":"string"},"parent":{"type":"string"},"type":{"type":"string"},"property":{"type":"string"},"value":{}}}}}})", edit, xiiMakeDelegate(&xiiEditorMCP::EditScene, this)).Failed() ||
      m_Registry.RegisterTool("editor.save", "Queue an asynchronous save of the specified open scene revision. Inspect the dirty flag to confirm completion.", sRevision, edit, xiiMakeDelegate(&xiiEditorMCP::Save, this)).Failed() ||
      m_Registry.RegisterTool("editor.assets", "Search indexed assets by data-relative path, type and tags with bounded pagination.", R"({"type":"object","properties":{"query":{"type":"string"},"type":{"type":"string"},"tag":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", read, xiiMakeDelegate(&xiiEditorMCP::Assets, this)).Failed() ||
      m_Registry.RegisterTool("editor.asset", "Inspect one indexed asset's transform state, dependencies, outputs and recent transform messages.", R"({"type":"object","required":["asset"],"properties":{"asset":{"type":"string"}}})", read, xiiMakeDelegate(&xiiEditorMCP::Asset, this)).Failed() ||
      m_Registry.RegisterTool("editor.transform_asset", "Queue an indexed asset for transformation by the Editor's background asset processor.", R"({"type":"object","required":["asset","generation"],"properties":{"asset":{"type":"string"},"generation":{"type":"integer"}}})", execute, xiiMakeDelegate(&xiiEditorMCP::Transform, this)).Failed())
  {
    return xiiStatus("Could not register Editor MCP capabilities.");
  }

  if (pOptions->GetBoolOption("-MCP"))
  {
    const auto status = m_Bridge.StartServer(m_Registry, "XII Editor", pOptions->GetStringOption("-MCPLocal"), uiPermissions, !pOptions->GetBoolOption("-MCPNoDiscovery"));
    if (status.Failed())
    {
      return status;
    }
  }

  const auto sListen = pOptions->GetStringOption("-MCPListen");
  if (!sListen.IsEmpty())
  {
    xiiMCPHttpConfiguration http;
    http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
    http.m_uiPermissions = uiPermissions;
    if (xiiNetworkEndpoint::Parse(sListen, http.m_Endpoint).Failed())
    {
      return xiiStatus("Invalid -MCPListen endpoint.");
    }
    const auto status = m_Http.Start(http);
    if (status.Failed())
    {
      return status;
    }
  }

  QObject::connect(&m_Timer, &QTimer::timeout, [this] {
    if (m_bUpdating)
    {
      return;
    }
    m_bUpdating = true;
    XII_SCOPE_EXIT(m_bUpdating = false);
    m_Bridge.Update();
    m_Http.Update();
  });
  m_Timer.start(10);
  return XII_SUCCESS;
}

void xiiEditorMCP::DocumentEvent(const xiiDocumentManager::Event& event)
{
  if (event.m_Type == xiiDocumentManager::Event::Type::DocumentClosing && event.m_pDocument != nullptr)
  {
    m_DocumentSessions.Remove(event.m_pDocument->GetGuid());
  }
}

xiiUuid xiiEditorMCP::GetDocumentSession(const xiiDocument& document)
{
  auto it = m_DocumentSessions.Find(document.GetGuid());
  if (!it.IsValid())
  {
    it = m_DocumentSessions.Insert(document.GetGuid(), xiiUuid::MakeUuid());
  }
  return it.Value();
}

xiiDocument* xiiEditorMCP::FindDocument(const xiiVariantDictionary& arguments)
{
  if (!xiiToolsProject::IsProjectOpen())
  {
    return nullptr;
  }

  const auto id        = ReadGuid(arguments, "document");
  auto*      pDocument = id.IsValid() ? xiiDocumentManager::GetDocumentByGuid(id) : nullptr;
  return pDocument != nullptr && xiiToolsProject::GetSingleton()->IsDocumentInAllowedRoot(pDocument->GetDocumentPath()) ? pDocument : nullptr;
}

xiiStatus xiiEditorMCP::ValidateEdit(const xiiVariantDictionary& arguments, xiiDocument*& out_pDocument)
{
  auto* pDocument = FindDocument(arguments);
  if (pDocument == nullptr || !IsScene(*pDocument) || pDocument->IsReadOnly() ||
      xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID != m_AllowedProject)
  {
    return xiiStatus("Editing requires an authorized ready project and an open writable scene, prefab or layer.");
  }
  xiiUInt64 uiGeneration = 0, uiRevision = 0;
  if (!ReadUnsigned(arguments, "generation", uiGeneration) || !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration) ||
      !ReadUnsigned(arguments, "revision", uiRevision) || uiRevision != pDocument->GetModificationRevision() ||
      ReadGuid(arguments, "session") != GetDocumentSession(*pDocument))
  {
    return xiiStatus("The document or project changed. Fetch its current session and revision before editing.");
  }
  if (pDocument->GetCommandHistory()->IsInTransaction() || pDocument->GetCommandHistory()->IsInUndoRedo())
  {
    return xiiStatus("The Editor is already running a document transaction.");
  }

  out_pDocument = pDocument;
  return XII_SUCCESS;
}

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

xiiStatus xiiEditorMCP::Documents(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
{
  xiiVariantArray documents;
  if (xiiToolsProject::IsProjectOpen())
  {
    const auto* pProject = xiiToolsProject::GetSingleton();
    for (const auto* pManager : xiiDocumentManager::GetAllDocumentManagers())
    {
      for (const auto* pDocument : pManager->GetAllOpenDocuments())
      {
        if (documents.GetCount() >= 128 || !pProject->IsDocumentInAllowedRoot(pDocument->GetDocumentPath()))
        {
          continue;
        }
        xiiVariantDictionary row;
        DescribeDocument(*pDocument, GetDocumentSession(*pDocument), row);
        xiiVariantArray selected;
        for (const auto* pObject : pDocument->GetSelectionManager()->GetSelection())
        {
          if (selected.GetCount() >= 32)
          {
            break;
          }
          selected.PushBack(FormatGuid(pObject->GetGuid()));
        }
        row["selection"] = std::move(selected);
        documents.PushBack(std::move(row));
      }
    }
  }
  out_result["documents"] = std::move(documents);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::Scene(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  auto* pDocument = FindDocument(arguments);
  if (pDocument == nullptr || !IsScene(*pDocument))
  {
    return xiiStatus("Select an open scene, prefab or layer in the active project.");
  }
  auto&       accessor = *pDocument->GetObjectAccessor();
  const auto* pObject  = arguments.Contains("object") ? FindDocumentObject(accessor, arguments, "object") : accessor.GetObjectManager()->GetRootObject();
  xiiUInt64   uiOffset = 0, uiLimit = 0;
  if (pObject == nullptr || !ReadPage(arguments, uiOffset, uiLimit))
  {
    return xiiStatus("The requested object or page is unavailable.");
  }

  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  out_result["object"]     = DescribeDocumentObject(*pObject);
  const auto&     children = pObject->GetChildren();
  xiiVariantArray rows;
  if (uiOffset < children.GetCount())
  {
    const auto uiEnd = xiiMath::Min(static_cast<xiiUInt64>(children.GetCount()), uiOffset + uiLimit);
    for (xiiUInt32 uiIndex = static_cast<xiiUInt32>(uiOffset); uiIndex < uiEnd; ++uiIndex)
    {
      rows.PushBack(DescribeDocumentObject(*children[uiIndex]));
    }
    if (uiEnd < children.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
  }
  out_result["children"] = std::move(rows);
  const auto& type       = *pObject->GetType();
  out_result["schema"]   = xiiMCPReflection::Describe(type);
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  type.GetAllProperties(properties);
  xiiVariantDictionary values;
  for (const auto* pProperty : properties)
  {
    if (pProperty->GetCategory() != xiiPropertyCategory::Member || pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer) ||
        pProperty->GetAttributeByType<xiiHiddenAttribute>() != nullptr || values.GetCount() >= 64)
    {
      continue;
    }
    xiiVariant value, json;
    if (accessor.GetValue(pObject, pProperty, value).Succeeded() && xiiMCPReflection::ConvertValue(value, json))
    {
      values[pProperty->GetPropertyName()] = std::move(json);
    }
  }
  out_result["properties"] = std::move(values);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::EditScene(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument  = nullptr;
  const auto   validation = ValidateEdit(arguments, pDocument);
  if (validation.Failed())
  {
    return validation;
  }
  const auto* pOperations = arguments.GetValue("operations");
  if (pOperations == nullptr || !pOperations->IsA<xiiVariantArray>())
  {
    return xiiStatus("Provide an array of scene edit operations.");
  }
  const auto& operations = pOperations->Get<xiiVariantArray>();
  if (operations.IsEmpty() || operations.GetCount() > 24)
  {
    return xiiStatus("Submit between 1 and 24 operations per transaction.");
  }

  auto& accessor = *pDocument->GetObjectAccessor();
  accessor.StartTransaction("Apply MCP scene edits");
  bool bCommitted = false;
  XII_SCOPE_EXIT(if (!bCommitted) { accessor.CancelTransaction(); });
  xiiVariantArray results;
  for (const auto& operation : operations)
  {
    if (context.HasExpired() || !operation.IsA<xiiVariantDictionary>())
    {
      return xiiStatus("The batch expired or contains an invalid operation.");
    }
    xiiVariantDictionary row;
    const auto           status = ApplySceneCommand(accessor, operation.Get<xiiVariantDictionary>(), row);
    if (status.Failed())
    {
      return xiiStatus(xiiFmt("Operation {0} failed: {1}", results.GetCount(), status.GetMessageString()));
    }
    results.PushBack(std::move(row));
  }
  if (context.HasExpired())
  {
    return xiiStatus("The batch deadline expired before commit.");
  }
  accessor.FinishTransaction();
  bCommitted = true;
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  out_result["operations"] = std::move(results);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::Save(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument  = nullptr;
  const auto   validation = ValidateEdit(arguments, pDocument);
  if (validation.Failed())
  {
    return validation;
  }
  const bool bDirty  = pDocument->IsModified();
  const bool bQueued = bDirty && pDocument->SaveDocumentAsync({}).IsValid();
  if (bDirty && !bQueued)
  {
    return xiiStatus("The Editor rejected the save request for this document.");
  }
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  out_result["queued"] = bQueued;
  return XII_SUCCESS;
}

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
  if (!xiiToolsProject::IsProjectOpen() || !ReadUnsigned(arguments, "generation", uiGeneration) || !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration) ||
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

XII_PLUGIN_DEPENDENCY(xiiMCPPlugin);

XII_PLUGIN_ON_LOADED()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  if (QCoreApplication::instance() == nullptr || (!pOptions->GetBoolOption("-MCP") && pOptions->GetStringOption("-MCPListen").IsEmpty()))
  {
    return;
  }
  s_pEditorMCP = XII_DEFAULT_NEW(xiiEditorMCP);
  if (s_pEditorMCP->Start().LogFailure())
  {
    XII_DEFAULT_DELETE(s_pEditorMCP);
  }
}

XII_PLUGIN_ON_UNLOADED()
{
  XII_DEFAULT_DELETE(s_pEditorMCP);
}

XII_STATICLINK_FILE(EditorPluginMCP, EditorPluginMCP_Implementation_EditorMCP);
