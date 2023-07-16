#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class xiiManipulatorAttribute;
struct xiiPhantomRttiManagerEvent;
struct xiiSelectionManagerEvent;

struct XII_GUIFOUNDATION_DLL xiiManipulatorManagerEvent
{
  const xiiDocument*                             m_pDocument;
  const xiiManipulatorAttribute*                 m_pManipulator;
  const xiiHybridArray<xiiPropertySelection, 8>* m_pSelection;
  bool                                           m_bHideManipulators;
};

class XII_GUIFOUNDATION_DLL xiiManipulatorManager
{
  XII_DECLARE_SINGLETON(xiiManipulatorManager);

public:
  xiiManipulatorManager();
  ~xiiManipulatorManager();

  const xiiManipulatorAttribute* GetActiveManipulator(const xiiDocument* pDoc, const xiiHybridArray<xiiPropertySelection, 8>*& out_pSelection) const;

  void SetActiveManipulator(
    const xiiDocument*                             pDoc,
    const xiiManipulatorAttribute*                 pManipulator,
    const xiiHybridArray<xiiPropertySelection, 8>& selection);

  void ClearActiveManipulator(const xiiDocument* pDoc);

  xiiCopyOnBroadcastEvent<const xiiManipulatorManagerEvent&> m_Events;

  void HideActiveManipulator(const xiiDocument* pDoc, bool bHide);
  void ToggleHideActiveManipulator(const xiiDocument* pDoc);

private:
  struct Data
  {
    Data()
    {
      m_pAttribute        = nullptr;
      m_bHideManipulators = false;
    }

    const xiiManipulatorAttribute*          m_pAttribute;
    xiiHybridArray<xiiPropertySelection, 8> m_Selection;
    bool                                    m_bHideManipulators;
  };

  void InternalSetActiveManipulator(
    const xiiDocument*                             pDoc,
    const xiiManipulatorAttribute*                 pManipulator,
    const xiiHybridArray<xiiPropertySelection, 8>& selection,
    bool                                           bUnhide);

  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  void TransferToCurrentSelection(const xiiDocument* pDoc);

  void PhantomTypeManagerEventHandler(const xiiPhantomRttiManagerEvent& e);
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& e);

  xiiMap<const xiiDocument*, Data> m_ActiveManipulator;
};
