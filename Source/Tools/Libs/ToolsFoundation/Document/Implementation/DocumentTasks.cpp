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
    m_document->m_LastSaveResult = xiiStatus(xiiFmt("Unable to open file '{0}' for writing!", m_document->m_sDocumentPath));
  }
  else
  {
    m_document->m_LastSaveResult = xiiStatus(XII_SUCCESS);
  }
}

xiiAfterSaveDocumentTask::xiiAfterSaveDocumentTask()
{
  ConfigureTask("xiiAfterSaveDocumentTask", xiiTaskNesting::Maybe);
}

xiiAfterSaveDocumentTask::~xiiAfterSaveDocumentTask() = default;

void xiiAfterSaveDocumentTask::Execute()
{
  if (m_document->m_LastSaveResult.Succeeded())
  {
    xiiDocumentEvent e;
    e.m_pDocument = m_document;
    e.m_Type      = xiiDocumentEvent::Type::DocumentSaved;
    m_document->m_EventsOne.Broadcast(e);
    m_document->s_EventsAny.Broadcast(e);

    m_document->SetModified(false);

    // after saving once, this information is pointless
    m_document->m_uiUnknownObjectTypeInstances = 0;
    m_document->m_UnknownObjectTypes.Clear();
  }

  if (m_document->m_LastSaveResult.Succeeded())
  {
    m_document->InternalAfterSaveDocument();
  }
  if (m_callback.IsValid())
  {
    m_callback(m_document, m_document->m_LastSaveResult);
  }
  m_document->m_ActiveSaveTask.Invalidate();
}
