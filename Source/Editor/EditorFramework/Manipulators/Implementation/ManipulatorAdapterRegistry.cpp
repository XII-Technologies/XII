#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>

XII_IMPLEMENT_SINGLETON(xiiManipulatorAdapterRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ManipulatorAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ManipulatorManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiManipulatorAdapterRegistry);
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = xiiManipulatorAdapterRegistry::GetSingleton();
    XII_DEFAULT_DELETE(ptr);
  }
 
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiManipulatorAdapterRegistry::xiiManipulatorAdapterRegistry() :
  m_SingletonRegistrar(this)
{
  xiiManipulatorManager::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));
}

xiiManipulatorAdapterRegistry::~xiiManipulatorAdapterRegistry()
{
  xiiManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(
    xiiMakeDelegate(&xiiManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}

void xiiManipulatorAdapterRegistry::QueryGridSettings(const xiiDocument* pDocument, xiiGridSettingsMsgToEngine& out_gridSettings)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    adapt->QueryGridSettings(out_gridSettings);
  }
}

void xiiManipulatorAdapterRegistry::ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  if (e.m_pManipulator == nullptr || e.m_bHideManipulators)
    return;

  for (const auto& sel : *e.m_pSelection)
  {
    xiiManipulatorAdapter* pAdapter = m_Factory.CreateObject(e.m_pManipulator->GetDynamicRTTI());

    if (pAdapter)
    {
      m_DocumentAdapters[e.m_pDocument].m_Adapters.PushBack(pAdapter);
      pAdapter->SetManipulator(e.m_pManipulator, sel.m_pObject);
    }
  }
}

void xiiManipulatorAdapterRegistry::ClearAdapters(const xiiDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    XII_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
