/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QApplication>

using namespace xiiEditorMCPArguments;

void xiiEditorMCP::DescribeDocument(const xiiDocument& document, const xiiUuid& session, xiiVariantDictionary& out_result)
{
  out_result["id"]         = FormatGuid(document.GetGuid());
  out_result["session"]    = FormatGuid(session);
  out_result["revision"]   = document.GetModificationRevision();
  out_result["generation"] = xiiToolsProject::GetProjectGeneration();
  out_result["type"]       = xiiString(document.GetDocumentTypeName());
  out_result["path"]       = xiiString(document.GetDocumentPath());
  out_result["dirty"]      = document.IsModified();
  out_result["readOnly"]   = document.IsReadOnly();
  out_result["canUndo"]    = document.GetCommandHistory()->CanUndo();
  out_result["canRedo"]    = document.GetCommandHistory()->CanRedo();
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
  if (pDocument == nullptr || pDocument->IsReadOnly() ||
      xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID != m_AllowedProject)
  {
    return xiiStatus("Editing requires an authorized ready project and an open writable document.");
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
  for (auto* pWidget = QApplication::focusWidget(); pWidget != nullptr && xiiToolsProject::IsProjectOpen(); pWidget = pWidget->parentWidget())
  {
    if (const auto* pWindow = qobject_cast<const xiiQtDocumentWindow*>(pWidget))
    {
      if (const auto* pActive = pWindow->GetDocument();
          pActive != nullptr && xiiToolsProject::GetSingleton()->IsDocumentInAllowedRoot(pActive->GetDocumentPath()))
      {
        out_result["activeDocument"] = FormatGuid(pActive->GetGuid());
      }
      break;
    }
  }
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::Document(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  auto* pDocument = FindDocument(arguments);
  if (pDocument == nullptr)
  {
    return xiiStatus("The document is not open in the current project.");
  }
  return InspectDocumentObject(*pDocument, arguments, out_result);
}

xiiStatus xiiEditorMCP::SetDocumentProperty(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument = nullptr;
  XII_SUCCEED_OR_RETURN(ValidateEdit(arguments, pDocument));

  auto&       accessor  = *pDocument->GetObjectAccessor();
  const auto* pObject   = accessor.GetObject(ReadGuid(arguments, "object"));
  const auto* pProperty = pObject != nullptr ? pObject->GetType()->FindPropertyByName(xiiMCPJson::GetString(arguments, "property")) : nullptr;
  const auto* pValue    = arguments.GetValue("value");
  xiiVariant  value;
  if (pProperty == nullptr || pValue == nullptr || !xiiMCPReflection::ConvertArgument(*pProperty, *pValue, value) || context.HasExpired())
  {
    return xiiStatus("Provide a current object, a supported writable reflected property and a compatible value before the deadline.");
  }

  accessor.StartTransaction("Edit reflected property through MCP");
  const auto status = accessor.SetValue(pObject, pProperty, value);
  if (status.Failed() || context.HasExpired())
  {
    accessor.CancelTransaction();
    return status.Failed() ? status : xiiStatus("The edit deadline expired.");
  }
  accessor.FinishTransaction();
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::DocumentHistory(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument = nullptr;
  XII_SUCCEED_OR_RETURN(ValidateEdit(arguments, pDocument));

  auto&      history = *pDocument->GetCommandHistory();
  const auto sAction = xiiMCPJson::GetString(arguments, "action");
  if (sAction == "undo" && history.CanUndo())
  {
    XII_SUCCEED_OR_RETURN(history.Undo());
  }
  else if (sAction == "redo" && history.CanRedo())
  {
    XII_SUCCEED_OR_RETURN(history.Redo());
  }
  else
  {
    return xiiStatus("The requested undo or redo is unavailable for this document.");
  }
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
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

xiiResult xiiEditorMCP::RegisterDocuments()
{
  const auto read      = xiiMCPPermission::Read;
  const auto edit      = xiiMCPPermission::Edit;
  const auto sEmpty    = R"({"type":"object","additionalProperties":false})";
  const auto sRevision = R"({"type":"object","required":["document","session","revision","generation"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"}}})";
  if (m_Registry.RegisterTool("editor.documents", "List open Editor documents, selection and revision tokens. Scene authoring requires a matching document session and revision.", sEmpty, read, xiiMakeDelegate(&xiiEditorMCP::Documents, this)).Failed() ||
      m_Registry.RegisterTool("editor.document", "Inspect an open document's reflected object tree, properties and history availability.",
                              R"({"type":"object","required":["document"],"properties":{"document":{"type":"string"},"object":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", read, xiiMakeDelegate(&xiiEditorMCP::Document, this))
        .Failed() ||
      m_Registry.RegisterTool("editor.set_document_property", "Set a supported reflected member on an open document object using its command history.",
                              R"({"type":"object","required":["document","session","revision","generation","object","property","value"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"},"object":{"type":"string"},"property":{"type":"string"},"value":{}}})", edit, xiiMakeDelegate(&xiiEditorMCP::SetDocumentProperty, this))
        .Failed() ||
      m_Registry.RegisterTool("editor.document_history", "Undo or redo the most recent command for the specified open document revision.",
                              R"({"type":"object","required":["document","session","revision","generation","action"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"},"action":{"enum":["undo","redo"]}}})", edit, xiiMakeDelegate(&xiiEditorMCP::DocumentHistory, this))
        .Failed() ||
      m_Registry.RegisterTool("editor.save", "Queue an asynchronous save of the specified open document revision. Inspect the dirty flag to confirm completion.", sRevision, edit, xiiMakeDelegate(&xiiEditorMCP::Save, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
