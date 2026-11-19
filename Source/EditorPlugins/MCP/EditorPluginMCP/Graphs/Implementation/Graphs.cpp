/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <Foundation/Types/ScopeExit.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

namespace
{
  const xiiDocumentNodeManager* GetNodeManager(const xiiDocument& document)
  {
    // Node command accessors require their owning document to use a node manager.
    const auto* pAccessor = document.GetObjectAccessor();
    return pAccessor != nullptr && pAccessor->GetDynamicRTTI()->IsDerivedFrom<xiiNodeCommandAccessor>() ?
      static_cast<const xiiDocumentNodeManager*>(document.GetObjectManager()) :
      nullptr;
  }

  xiiVariantArray DescribePins(const xiiDocumentNodeManager& manager, xiiArrayPtr<const xiiUniquePtr<const xiiPin>> pins)
  {
    xiiVariantArray rows;
    for (const auto& pPin : pins)
    {
      if (rows.GetCount() == 64)
      {
        break;
      }
      xiiVariantDictionary row;
      row["name"]            = xiiString(pPin->GetName());
      row["displayName"]     = xiiString(pPin->GetDisplayName());
      row["connectionCount"] = manager.GetConnections(*pPin).GetCount();
      if (const auto* pProperties = manager.GetPinPropertyObject(*pPin))
      {
        row["propertyObject"] = xiiEditorMCPArguments::FormatGuid(pProperties->GetGuid());
      }
      rows.PushBack(row);
    }
    return rows;
  }

  xiiStatus CreateNode(const xiiMCPCallContext& context, const xiiDocumentNodeManager& manager, xiiObjectAccessorBase& ref_accessor,
                       const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto                               sType     = xiiMCPJson::GetString(arguments, "type");
    const auto                               sTemplate = xiiMCPJson::GetString(arguments, "template");
    xiiDynamicArray<xiiNodeCreationTemplate> templates;
    manager.GetNodeCreationTemplates(templates);
    const xiiNodeCreationTemplate* pMatch = nullptr;
    for (const auto& creation : templates)
    {
      if (creation.m_pType == nullptr || creation.m_pType->GetTypeName() != sType || (!sTemplate.IsEmpty() && creation.m_sTypeName != sTemplate))
      {
        continue;
      }
      if (pMatch != nullptr)
      {
        return xiiStatus("Select a unique node template by type and name.");
      }
      pMatch = &creation;
    }
    if (pMatch == nullptr || context.HasExpired())
    {
      return xiiStatus("Choose a current creation template before the deadline.");
    }

    ref_accessor.StartTransaction("Create graph node through MCP");
    bool bCommitted = false;
    XII_SCOPE_EXIT(if (!bCommitted) { ref_accessor.CancelTransaction(); });
    xiiUuid id;
    XII_SUCCEED_OR_RETURN(ref_accessor.AddObjectByName(nullptr, "", xiiInt32(-1), pMatch->m_pType, id));
    const auto* pObject = ref_accessor.GetObject(id);
    if (pObject == nullptr || !manager.IsNode(pObject))
    {
      return xiiStatus("The template did not create a node in this document.");
    }
    for (const auto& property : pMatch->m_PropertyValues)
    {
      XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pObject, property.m_sPropertyName.GetView(), property.m_Value));
    }
    if (context.HasExpired())
    {
      return xiiStatus("The node creation deadline expired before commit.");
    }
    ref_accessor.FinishTransaction();
    bCommitted           = true;
    out_result["object"] = xiiEditorMCPArguments::FormatGuid(id);
    return XII_SUCCESS;
  }
} // namespace

xiiStatus xiiEditorMCP::Graph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  auto*       pDocument = FindDocument(arguments);
  const auto* pManager  = pDocument != nullptr ? GetNodeManager(*pDocument) : nullptr;
  xiiUInt64   uiOffset  = 0;
  xiiUInt64   uiLimit   = 0;
  if (pManager == nullptr || !xiiEditorMCPArguments::ReadPage(arguments, uiOffset, uiLimit) || uiOffset > xiiMath::MaxValue<xiiUInt32>())
  {
    return xiiStatus("Select an open document with a node command accessor and a valid page range.");
  }

  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  xiiVariantArray entries;
  if (xiiMCPJson::GetString(arguments, "view") == "templates")
  {
    xiiDynamicArray<xiiNodeCreationTemplate> templates;
    pManager->GetNodeCreationTemplates(templates);
    const auto uiEnd = xiiMath::Min(uiOffset + uiLimit, static_cast<xiiUInt64>(templates.GetCount()));
    for (xiiUInt64 uiIndex = uiOffset; uiIndex < uiEnd && !context.HasExpired(); ++uiIndex)
    {
      const auto& creation = templates[static_cast<xiiUInt32>(uiIndex)];
      if (creation.m_pType == nullptr)
      {
        continue;
      }
      xiiVariantDictionary entry;
      entry["type"]                       = xiiString(creation.m_pType->GetTypeName());
      entry["name"]                       = xiiString(creation.m_sTypeName);
      entry["category"]                   = xiiString(creation.m_sCategory.GetView());
      entry["requiresTemplateProperties"] = !creation.m_PropertyValues.IsEmpty();
      entries.PushBack(entry);
    }
    if (uiEnd < templates.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
  }
  else
  {
    const auto children = pManager->GetRootObject()->GetChildren();
    const auto uiEnd    = xiiMath::Min(uiOffset + uiLimit, static_cast<xiiUInt64>(children.GetCount()));
    for (xiiUInt64 uiIndex = uiOffset; uiIndex < uiEnd && !context.HasExpired(); ++uiIndex)
    {
      const auto*          pObject = children[static_cast<xiiUInt32>(uiIndex)];
      xiiVariantDictionary entry;
      entry["id"]   = xiiEditorMCPArguments::FormatGuid(pObject->GetGuid());
      entry["type"] = xiiString(pObject->GetType()->GetTypeName());
      if (pManager->IsNode(pObject))
      {
        entry["kind"]    = xiiString("node");
        entry["inputs"]  = DescribePins(*pManager, pManager->GetInputPins(pObject));
        entry["outputs"] = DescribePins(*pManager, pManager->GetOutputPins(pObject));
        if (xiiVariant position; xiiMCPReflection::ConvertValue(pManager->GetNodePos(pObject), position))
        {
          entry["position"] = position;
        }
      }
      else if (pManager->IsConnection(pObject))
      {
        entry["kind"] = xiiString("connection");
        if (const auto* pConnection = pManager->GetConnectionIfExists(pObject))
        {
          const auto& source = pConnection->GetSourcePin();
          const auto& target = pConnection->GetTargetPin();
          entry["source"]    = xiiEditorMCPArguments::FormatGuid(source.GetParent()->GetGuid());
          entry["sourcePin"] = xiiString(source.GetName());
          entry["target"]    = xiiEditorMCPArguments::FormatGuid(target.GetParent()->GetGuid());
          entry["targetPin"] = xiiString(target.GetName());
        }
      }
      else
      {
        continue;
      }
      entries.PushBack(entry);
    }
    if (uiEnd < children.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
  }
  if (context.HasExpired())
  {
    return xiiStatus("The graph inspection deadline expired.");
  }
  out_result["entries"] = entries;
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::EditGraph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument = nullptr;
  XII_SUCCEED_OR_RETURN(ValidateEdit(arguments, pDocument));
  const auto* pManager = GetNodeManager(*pDocument);
  if (pManager == nullptr || context.HasExpired())
  {
    return xiiStatus("The document does not support node commands or the deadline expired.");
  }

  const auto sAction  = xiiMCPJson::GetString(arguments, "action");
  auto&      accessor = *pDocument->GetObjectAccessor();
  auto*      pHistory = pDocument->GetCommandHistory();
  if (sAction == "create")
  {
    XII_SUCCEED_OR_RETURN(CreateNode(context, *pManager, accessor, arguments, out_result));
  }
  else if (sAction == "connect")
  {
    const auto* pSource = accessor.GetObject(xiiEditorMCPArguments::ReadGuid(arguments, "source"));
    const auto* pTarget = accessor.GetObject(xiiEditorMCPArguments::ReadGuid(arguments, "target"));
    if (pSource == nullptr || pTarget == nullptr || !pManager->IsNode(pSource) || !pManager->IsNode(pTarget))
    {
      return xiiStatus("Both endpoints must be current nodes in this document.");
    }
    const auto* pOut = pManager->GetOutputPinByName(pSource, xiiMCPJson::GetString(arguments, "sourcePin"));
    const auto* pIn  = pManager->GetInputPinByName(pTarget, xiiMCPJson::GetString(arguments, "targetPin"));
    if (pOut == nullptr || pIn == nullptr)
    {
      return xiiStatus("Use current pin names from editor.graph.");
    }
    xiiDocumentNodeManager::CanConnectResult capacity;
    XII_SUCCEED_OR_RETURN(pManager->CanConnect(pManager->GetConnectionType(), *pOut, *pIn, capacity));
    using Capacity = xiiDocumentNodeManager::CanConnectResult;
    if (((capacity == Capacity::Connect1to1 || capacity == Capacity::Connect1toN) && pManager->HasConnections(*pOut)) ||
        ((capacity == Capacity::Connect1to1 || capacity == Capacity::ConnectNto1) && pManager->HasConnections(*pIn)))
    {
      return xiiStatus("Disconnect the existing link before connecting a single-link pin.");
    }
    XII_SUCCEED_OR_RETURN(xiiNodeCommands::AddAndConnectCommand(pHistory, pManager->GetConnectionType(), *pOut, *pIn));
  }
  else
  {
    const auto  id      = xiiEditorMCPArguments::ReadGuid(arguments, "object");
    const auto* pObject = id.IsValid() ? accessor.GetObject(id) : nullptr;
    if (pObject == nullptr)
    {
      return xiiStatus("The requested node or connection no longer exists.");
    }
    if (sAction == "disconnect" && pManager->IsConnection(pObject) && pManager->GetConnectionIfExists(pObject) != nullptr)
    {
      XII_SUCCEED_OR_RETURN(xiiNodeCommands::DisconnectAndRemoveCommand(pHistory, id));
    }
    else if (sAction == "remove" && pManager->IsNode(pObject))
    {
      accessor.StartTransaction("Remove graph node through MCP");
      const auto status = accessor.RemoveObject(pObject);
      if (status.Failed())
      {
        accessor.CancelTransaction();
        return status;
      }
      accessor.FinishTransaction();
    }
    else
    {
      return xiiStatus("The requested graph operation is unsupported for this object.");
    }
  }

  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterGraphs()
{
  XII_SUCCEED_OR_RETURN(m_Registry.RegisterTool("editor.graph", "Page nodes, pins and connections or list creation templates for an open node document.",
                                                R"({"type":"object","required":["document"],"properties":{"document":{"type":"string"},"view":{"enum":["graph","templates"]},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", xiiMCPPermission::Read, xiiMakeDelegate(&xiiEditorMCP::Graph, this)));
  return m_Registry.RegisterTool("editor.edit_graph", "Create a template node, connect pins, disconnect a link or remove a node through the document command history.",
                                 R"({"type":"object","required":["document","session","revision","generation","action"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"},"action":{"enum":["create","connect","disconnect","remove"]},"type":{"type":"string"},"template":{"type":"string"},"source":{"type":"string"},"sourcePin":{"type":"string"},"target":{"type":"string"},"targetPin":{"type":"string"},"object":{"type":"string"}}})", xiiMCPPermission::Edit, xiiMakeDelegate(&xiiEditorMCP::EditGraph, this));
}
