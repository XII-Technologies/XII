#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

XII_IMPLEMENT_SINGLETON(xiiVisualizerManager);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualizerManager)

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVisualizerManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiVisualizerManager::GetSingleton())
    {
      auto ptr = xiiVisualizerManager::GetSingleton();
      XII_DEFAULT_DELETE(ptr);
    }
  }


XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVisualizerManager::xiiVisualizerManager() :
  m_SingletonRegistrar(this)
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualizerManager::DocumentManagerEventHandler, this));
}

xiiVisualizerManager::~xiiVisualizerManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerManager::DocumentManagerEventHandler, this));
}

void xiiVisualizerManager::SetVisualizersActive(const xiiDocument* pDoc, bool bActive)
{
  if (m_DocsSubscribed[pDoc].m_bActivated == bActive)
    return;

  m_DocsSubscribed[pDoc].m_bActivated = bActive;

  SendEventToRecreateVisualizers(pDoc);
}

bool xiiVisualizerManager::GetVisualizersActive(const xiiDocument* pDoc)
{
  return m_DocsSubscribed[pDoc].m_bActivated;
}

void xiiVisualizerManager::SelectionEventHandler(const xiiSelectionManagerEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  SendEventToRecreateVisualizers(event.m_pDocument);
}

void xiiVisualizerManager::SendEventToRecreateVisualizers(const xiiDocument* pDoc)
{
  if (m_DocsSubscribed[pDoc].m_bActivated)
  {
    const auto& sel = pDoc->GetSelectionManager()->GetSelection();

    xiiVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument  = pDoc;
    m_Events.Broadcast(e);
  }
  else
  {
    xiiDeque<const xiiDocumentObject*> sel;

    xiiVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument  = pDoc;

    m_Events.Broadcast(e);
  }
}

void xiiVisualizerManager::DocumentManagerEventHandler(const xiiDocumentManager::Event& e)
{
  if (e.m_Type == xiiDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiVisualizerManager::StructureEventHandler, this));
  }

  if (e.m_Type == xiiDocumentManager::Event::Type::DocumentClosing)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerManager::StructureEventHandler, this));

    SetVisualizersActive(e.m_pDocument, false);
  }
}

void xiiVisualizerManager::StructureEventHandler(const xiiDocumentObjectStructureEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  if (!event.m_pDocument->GetSelectionManager()->IsSelectionEmpty() &&
      (event.m_EventType == xiiDocumentObjectStructureEvent::Type::AfterObjectAdded ||
       event.m_EventType == xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved))
  {
    SendEventToRecreateVisualizers(event.m_pDocument);
  }
}
