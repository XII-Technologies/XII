/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/DocumentTasks.h>

xiiSaveDocumentTask::xiiSaveDocumentTask()
{
  ConfigureTask("xiiSaveDocumentTask", xiiTaskNesting::Maybe);
}

xiiSaveDocumentTask::~xiiSaveDocumentTask() = default;

void xiiSaveDocumentTask::Execute()
{
  xiiAbstractGraphDdlSerializer::WriteDocument(file, &headerGraph, &objectGraph, &typesGraph, false);

  if (file.Close() == XII_FAILURE)
  {
    m_Result = xiiStatus(xiiFmt("Unable to write document file '{0}'!", m_sDocumentPath));
  }
  else
  {
    m_Result = xiiStatus(XII_SUCCESS);
  }
}

xiiAfterSaveDocumentTask::xiiAfterSaveDocumentTask()
{
  ConfigureTask("xiiAfterSaveDocumentTask", xiiTaskNesting::Maybe);
}

xiiAfterSaveDocumentTask::~xiiAfterSaveDocumentTask() = default;

void xiiAfterSaveDocumentTask::Execute()
{
  const auto result = m_pSaveTask->m_Result;
  if (result.Succeeded())
  {
    // Only the captured revision reached disk. Later transactions (including undo/redo)
    // must stay dirty, and event handlers may create further edits after this check.
    if (m_document->m_uiModificationRevision == m_uiModificationRevision)
      m_document->SetModified(false);

    xiiDocumentEvent e;
    e.m_pDocument = m_document;
    e.m_Type      = xiiDocumentEvent::Type::DocumentSaved;
    m_document->m_EventsOne.Broadcast(e);
    m_document->s_EventsAny.Broadcast(e);

    m_document->InternalAfterSaveDocument();
  }

  // An earlier completion must not forget a later queued save. Clear this task before
  // invoking the callback, which may enqueue another save; do not touch the document afterwards.
  if (m_document->m_ActiveSaveTask == m_TaskGroup)
    m_document->m_ActiveSaveTask.Invalidate();
  if (m_callback.IsValid())
  {
    m_callback(m_document, result);
  }
}
