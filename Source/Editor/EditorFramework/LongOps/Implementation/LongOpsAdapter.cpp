#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/LongOps/LongOpsAdapter.h>

XII_IMPLEMENT_SINGLETON(xiiLongOpsAdapter);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, LongOpsAdapter)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager",
    "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiLongOpsAdapter);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiLongOpsAdapter::GetSingleton())
    {
      auto ptr = xiiLongOpsAdapter::GetSingleton();
      XII_DEFAULT_DELETE(ptr);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiLongOpsAdapter::xiiLongOpsAdapter() :
  m_SingletonRegistrar(this)
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::DocumentManagerEventHandler, this));
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
}

xiiLongOpsAdapter::~xiiLongOpsAdapter()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::DocumentManagerEventHandler, this));
}

void xiiLongOpsAdapter::DocumentManagerEventHandler(const xiiDocumentManager::Event& e)
{
  if (e.m_Type == xiiDocumentManager::Event::Type::DocumentOpened)
  {
    const xiiRTTI* pRttiScene = xiiRTTI::FindTypeByName("xiiSceneDocument");
    const bool     bIsScene   = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      CheckAllTypes();

      e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::StructureEventHandler, this));

      ObjectAdded(e.m_pDocument->GetObjectManager()->GetRootObject());
    }
  }

  if (e.m_Type == xiiDocumentManager::Event::Type::DocumentClosing)
  {
    const xiiRTTI* pRttiScene = xiiRTTI::FindTypeByName("xiiSceneDocument");
    const bool     bIsScene   = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      xiiLongOpControllerManager::GetSingleton()->CancelAndRemoveAllOpsForDocument(e.m_pDocument->GetGuid());

      e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiLongOpsAdapter::StructureEventHandler, this));
    }
  }
}

void xiiLongOpsAdapter::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectStructureEvent::Type::AfterObjectAdded)
  {
    ObjectAdded(e.m_pObject);
  }

  if (e.m_EventType == xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    ObjectRemoved(e.m_pObject);
  }
}

void xiiLongOpsAdapter::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  const bool bExists = m_TypesWithLongOps.Contains(e.m_pChangedType);

  if (bExists && e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    m_TypesWithLongOps.Remove(e.m_pChangedType);
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to remove
  }

  if (!bExists && e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetAttributeByType<xiiLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(e.m_pChangedType);
      // if this ever becomes relevant:
      // iterate over all open documents and figure out which long ops to add
    }
  }

  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeChanged)
  {
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to add or remove
  }
}

void xiiLongOpsAdapter::CheckAllTypes()
{
  xiiRTTI::ForEachType(
    [&](const xiiRTTI* pRtti) {
      if (pRtti->GetAttributeByType<xiiLongOpAttribute>() != nullptr)
      {
        m_TypesWithLongOps.Insert(pRtti);
      }
    });
}

void xiiLongOpsAdapter::ObjectAdded(const xiiDocumentObject* pObject)
{
  const xiiRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const xiiPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = xiiDynamicCast<const xiiLongOpAttribute*>(pAttr))
          {
            xiiLongOpControllerManager::GetSingleton()->RegisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }

    return;
  }

  if (pRtti->IsDerivedFrom<xiiGameObject>() || pObject->GetParent() == nullptr /*document root object*/)
  {
    for (const xiiDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectAdded(pChild);
    }
  }
}

void xiiLongOpsAdapter::ObjectRemoved(const xiiDocumentObject* pObject)
{
  const xiiRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const xiiPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = xiiDynamicCast<const xiiLongOpAttribute*>(pAttr))
          {
            xiiLongOpControllerManager::GetSingleton()->UnregisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
  }
  else if (pRtti->IsDerivedFrom<xiiGameObject>())
  {
    for (const xiiDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectRemoved(pChild);
    }
  }
}
