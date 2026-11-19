/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

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
      m_Registry.RegisterTool("editor.save", "Queue an asynchronous save of the specified open scene revision. Inspect the dirty flag to confirm completion.", sRevision, edit, xiiMakeDelegate(&xiiEditorMCP::Save, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
