#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Document/Document.h>

XII_IMPLEMENT_SINGLETON(xiiManipulatorManager);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ManipulatorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiManipulatorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiManipulatorManager::GetSingleton())
    {
      auto ptr = xiiManipulatorManager::GetSingleton();
      XII_DEFAULT_DELETE(ptr);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiManipulatorManager::xiiManipulatorManager() :
  m_SingletonRegistrar(this)
{
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiManipulatorManager::PhantomTypeManagerEventHandler, this));
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiManipulatorManager::DocumentManagerEventHandler, this));
}

xiiManipulatorManager::~xiiManipulatorManager()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorManager::PhantomTypeManagerEventHandler, this));
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorManager::DocumentManagerEventHandler, this));
}

const xiiManipulatorAttribute* xiiManipulatorManager::GetActiveManipulator(
  const xiiDocument*                              pDoc,
  const xiiHybridArray<xiiPropertySelection, 8>*& out_pSelection) const
{
  out_pSelection = nullptr;
  auto it        = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    out_pSelection = &(it.Value().m_Selection);

    return it.Value().m_pAttribute;
  }

  return nullptr;
}

void xiiManipulatorManager::InternalSetActiveManipulator(
  const xiiDocument*                             pDoc,
  const xiiManipulatorAttribute*                 pManipulator,
  const xiiHybridArray<xiiPropertySelection, 8>& selection,
  bool                                           bUnhide)
{
  bool existed = false;
  auto it      = m_ActiveManipulator.FindOrAdd(pDoc, &existed);

  it.Value().m_pAttribute = pManipulator;
  it.Value().m_Selection  = selection;

  if (!existed)
  {
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiManipulatorManager::StructureEventHandler, this));
    pDoc->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiManipulatorManager::SelectionEventHandler, this));
  }

  auto& data = m_ActiveManipulator[pDoc];

  if (bUnhide)
  {
    data.m_bHideManipulators = false;
  }

  xiiManipulatorManagerEvent e;
  e.m_bHideManipulators = data.m_bHideManipulators;
  e.m_pDocument         = pDoc;
  e.m_pManipulator      = pManipulator;
  e.m_pSelection        = &data.m_Selection;

  m_Events.Broadcast(e);
}


void xiiManipulatorManager::SetActiveManipulator(
  const xiiDocument*                             pDoc,
  const xiiManipulatorAttribute*                 pManipulator,
  const xiiHybridArray<xiiPropertySelection, 8>& selection)
{
  InternalSetActiveManipulator(pDoc, pManipulator, selection, true);
}

void xiiManipulatorManager::ClearActiveManipulator(const xiiDocument* pDoc)
{
  xiiHybridArray<xiiPropertySelection, 8> clearSel;

  InternalSetActiveManipulator(pDoc, nullptr, clearSel, false);
}

void xiiManipulatorManager::HideActiveManipulator(const xiiDocument* pDoc, bool bHide)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid() && it.Value().m_bHideManipulators != bHide)
  {
    it.Value().m_bHideManipulators = bHide;

    if (bHide)
    {
      xiiHybridArray<xiiPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void xiiManipulatorManager::ToggleHideActiveManipulator(const xiiDocument* pDoc)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    it.Value().m_bHideManipulators = !it.Value().m_bHideManipulators;

    if (it.Value().m_bHideManipulators)
    {
      xiiHybridArray<xiiPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void xiiManipulatorManager::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    auto pDoc = e.m_pObject->GetDocumentObjectManager()->GetDocument();
    auto it   = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        if (sel.m_pObject == e.m_pObject)
        {
          it.Value().m_Selection.RemoveAndCopy(sel);
          InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
          return;
        }
      }
    }
  }

  if (e.m_EventType == xiiDocumentObjectStructureEvent::Type::BeforeReset)
  {
    auto pDoc = e.m_pDocument;
    auto it   = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        it.Value().m_Selection.RemoveAndCopy(sel);
        InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
      }
    }
  }
}

void xiiManipulatorManager::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  TransferToCurrentSelection(e.m_pDocument->GetMainDocument());
}

void xiiManipulatorManager::TransferToCurrentSelection(const xiiDocument* pDoc)
{
  auto& data       = m_ActiveManipulator[pDoc];
  auto  pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  if (data.m_bHideManipulators)
    return;

  xiiHybridArray<xiiPropertySelection, 8> newSelection;

  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  XII_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != xiiManipulatorSearchStrategy::None, "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  if (pDoc->GetManipulatorSearchStrategy() == xiiManipulatorSearchStrategy::SelectedObject)
  {
    for (xiiUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& OtherAttributes = selection[i]->GetTypeAccessor().GetType()->GetAttributes();

      for (const auto pOtherAttr : OtherAttributes)
      {
        if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
        {
          auto pOtherManip = static_cast<const xiiManipulatorAttribute*>(pOtherAttr);

          if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
              pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
              pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
          {
            auto& newItem     = newSelection.ExpandAndGetRef();
            newItem.m_pObject = selection[i];
          }
        }
      }
    }
  }

  if (pDoc->GetManipulatorSearchStrategy() == xiiManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (xiiUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& children = selection[i]->GetChildren();

      for (const auto& child : children)
      {
        const auto& OtherAttributes = child->GetTypeAccessor().GetType()->GetAttributes();

        for (const auto pOtherAttr : OtherAttributes)
        {
          if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
          {
            auto pOtherManip = static_cast<const xiiManipulatorAttribute*>(pOtherAttr);

            if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
                pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
                pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
            {
              auto& newItem     = newSelection.ExpandAndGetRef();
              newItem.m_pObject = child;
            }
          }
        }
      }
    }
  }

  InternalSetActiveManipulator(pDoc, pAttribute, newSelection, false);
}

void xiiManipulatorManager::PhantomTypeManagerEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeChanged || e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    for (auto it = m_ActiveManipulator.GetIterator(); it.IsValid(); ++it)
    {
      ClearActiveManipulator(it.Key());
    }
  }
}

void xiiManipulatorManager::DocumentManagerEventHandler(const xiiDocumentManager::Event& e)
{
  if (e.m_Type == xiiDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveManipulator(e.m_pDocument);

    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorManager::StructureEventHandler, this));
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorManager::SelectionEventHandler, this));

    m_ActiveManipulator.Remove(e.m_pDocument);
  }
}
