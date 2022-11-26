#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct xiiSelectionManagerEvent;
class xiiDocumentObject;
class xiiVisualizerAttribute;

struct XII_GUIFOUNDATION_DLL xiiVisualizerManagerEvent
{
  const xiiDocument*                        m_pDocument;
  const xiiDeque<const xiiDocumentObject*>* m_pSelection;
};

class XII_GUIFOUNDATION_DLL xiiVisualizerManager
{
  XII_DECLARE_SINGLETON(xiiVisualizerManager);

public:
  xiiVisualizerManager();
  ~xiiVisualizerManager();

  void SetVisualizersActive(const xiiDocument* pDoc, bool bActive);
  bool GetVisualizersActive(const xiiDocument* pDoc);

  xiiEvent<const xiiVisualizerManagerEvent&> m_Events;

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void SendEventToRecreateVisualizers(const xiiDocument* pDoc);

  struct DocData
  {
    bool m_bActivated;

    DocData() { m_bActivated = true; }
  };

  xiiMap<const xiiDocument*, DocData> m_DocsSubscribed;
};
