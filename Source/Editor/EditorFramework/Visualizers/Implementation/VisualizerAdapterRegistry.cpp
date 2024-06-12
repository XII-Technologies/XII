#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>

XII_IMPLEMENT_SINGLETON(xiiVisualizerAdapterRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualizerAdapterRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "VisualizerManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVisualizerAdapterRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = xiiVisualizerAdapterRegistry::GetSingleton();
    XII_DEFAULT_DELETE(ptr);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVisualizerAdapterRegistry::xiiVisualizerAdapterRegistry() :
  m_SingletonRegistrar(this)
{
  xiiVisualizerManager::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));
}

xiiVisualizerAdapterRegistry::~xiiVisualizerAdapterRegistry()
{
  xiiVisualizerManager::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}

void xiiVisualizerAdapterRegistry::CreateAdapters(const xiiDocument* pDocument, const xiiDocumentObject* pObject)
{
  const auto& attributes = pObject->GetTypeAccessor().GetType()->GetAttributes();

  for (const auto pAttr : attributes)
  {
    if (pAttr->IsInstanceOf<xiiVisualizerAttribute>())
    {
      xiiVisualizerAdapter* pAdapter = m_Factory.CreateObject(pAttr->GetDynamicRTTI());

      if (pAdapter)
      {
        m_DocumentAdapters[pDocument].m_Adapters.PushBack(pAdapter);
        pAdapter->SetVisualizer(static_cast<const xiiVisualizerAttribute*>(pAttr), pObject);
      }
    }
  }

  for (const auto pChild : pObject->GetChildren())
  {
    CreateAdapters(pDocument, pChild);
  }
}

void xiiVisualizerAdapterRegistry::VisualizerManagerEventHandler(const xiiVisualizerManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  for (const auto sel : *e.m_pSelection)
  {
    CreateAdapters(e.m_pDocument, sel);
  }
}

void xiiVisualizerAdapterRegistry::ClearAdapters(const xiiDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    XII_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
