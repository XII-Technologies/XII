/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  m_Factory.m_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualizerAdapterRegistry::FactoryEventHandler, this));
}

xiiVisualizerAdapterRegistry::~xiiVisualizerAdapterRegistry()
{
  xiiVisualizerManager::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));
  m_Factory.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapterRegistry::FactoryEventHandler, this));

  while (!m_DocumentAdapters.IsEmpty())
  {
    ClearAdapters(m_DocumentAdapters.GetIterator().Key());
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

  if (e.m_pSelection->IsEmpty())
    return;
  auto& data       = m_DocumentAdapters[e.m_pDocument];
  data.m_Selection = *e.m_pSelection;
  for (const auto sel : data.m_Selection)
  {
    CreateAdapters(e.m_pDocument, sel);
  }
}

void xiiVisualizerAdapterRegistry::ClearAdapters(const xiiDocument* pDocument)
{
  auto it = m_DocumentAdapters.Find(pDocument);
  if (!it.IsValid())
    return;
  for (auto& adapt : it.Value().m_Adapters)
  {
    XII_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters.Remove(it);
}

void xiiVisualizerAdapterRegistry::FactoryEventHandler(const xiiRttiMappedObjectFactory<xiiVisualizerAdapter>::Event& e)
{
  XII_IGNORE_UNUSED(e);
  // Registration can change the closest matching creator. Rebuild while the removed plugin's
  // code is still loaded, so no adapter or synchronized handle survives its creator's unload.
  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    auto& data = it.Value();
    for (auto* adapter : data.m_Adapters)
      XII_DEFAULT_DELETE(adapter);
    data.m_Adapters.Clear();
    for (const auto* selected : data.m_Selection)
      CreateAdapters(it.Key(), selected);
  }
}
