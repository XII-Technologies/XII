#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>

xiiMap<xiiString, xiiActionMap*> xiiActionMapManager::s_Mappings;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiActionMapManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiActionMapManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiActionMapManager public functions
////////////////////////////////////////////////////////////////////////

xiiResult xiiActionMapManager::RegisterActionMap(xiiStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (it.IsValid())
    return XII_FAILURE;

  s_Mappings.Insert(sMapping, XII_DEFAULT_NEW(xiiActionMap));
  return XII_SUCCESS;
}

xiiResult xiiActionMapManager::UnregisterActionMap(xiiStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (!it.IsValid())
    return XII_FAILURE;

  XII_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
  return XII_SUCCESS;
}

xiiActionMap* xiiActionMapManager::GetActionMap(xiiStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (!it.IsValid())
    return nullptr;

  return it.Value();
}


////////////////////////////////////////////////////////////////////////
// xiiActionMapManager private functions
////////////////////////////////////////////////////////////////////////

void xiiActionMapManager::Startup()
{
  xiiActionMapManager::RegisterActionMap("DocumentWindowTabMenu").IgnoreResult();
  xiiDocumentActions::MapActions("DocumentWindowTabMenu", "", false);
}

void xiiActionMapManager::Shutdown()
{
  xiiActionMapManager::UnregisterActionMap("DocumentWindowTabMenu").IgnoreResult();

  while (!s_Mappings.IsEmpty())
  {
    xiiResult res = UnregisterActionMap(s_Mappings.GetIterator().Key());
    XII_ASSERT_DEV(res == XII_SUCCESS, "Failed to call UnregisterActionMap successfully!");
    res.IgnoreResult();
  }
}
