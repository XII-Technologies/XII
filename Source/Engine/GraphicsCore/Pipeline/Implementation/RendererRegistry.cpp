#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RendererRegistry.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RendererRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRendererRegistry::UpdateRendererTypes();

    xiiPlugin::Events().AddEventHandler(xiiRendererRegistry::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiRendererRegistry::PluginEventHandler);

    xiiRendererRegistry::ClearRendererInstances();
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHybridArray<const xiiRTTI*, 16>         xiiRendererRegistry::s_RendererTypes;
xiiDynamicArray<xiiUniquePtr<xiiRenderer>> xiiRendererRegistry::s_RendererInstances;
xiiHashTable<const xiiRTTI*, xiiUInt32>    xiiRendererRegistry::s_RenderDataTypeToRendererIndex;
bool                                       xiiRendererRegistry::s_bRendererInstancesDirty = false;

// static
void xiiRendererRegistry::PluginEventHandler(const xiiPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void xiiRendererRegistry::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  xiiRTTI::ForEachDerivedType<xiiRenderer>([](const xiiRTTI* pRtti) -> void { s_RendererTypes.PushBack(pRtti); }, xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void xiiRendererRegistry::CreateRendererInstances()
{
  if (!s_bRendererInstancesDirty)
    return;

  ClearRendererInstances();

  for (const xiiRTTI* pRendererType : s_RendererTypes)
  {
    XII_ASSERT_DEV(pRendererType->IsDerivedFrom(xiiGetStaticRTTI<xiiRenderer>()), "Renderer type '{}' must be derived from xiiRenderer", pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<xiiRenderer>();

    xiiUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    xiiHybridArray<const xiiRTTI*, 8U> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (const xiiRTTI* pType : supportedTypes)
    {
      s_RenderDataTypeToRendererIndex.Insert(pType, uiIndex);
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void xiiRendererRegistry::ClearRendererInstances()
{
  s_RendererInstances.Clear();
  s_RenderDataTypeToRendererIndex.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RendererRegistry);
