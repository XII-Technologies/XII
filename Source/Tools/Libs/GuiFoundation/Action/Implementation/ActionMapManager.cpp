/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

void xiiActionMapManager::RegisterActionMap(xiiStringView sMapping, xiiStringView sParentMapping)
{
  auto it = s_Mappings.Find(sMapping);
  XII_ASSERT_ALWAYS(!it.IsValid(), "Mapping '{}' already exists", sMapping);
  s_Mappings.Insert(sMapping, XII_DEFAULT_NEW(xiiActionMap, sParentMapping));
}

void xiiActionMapManager::UnregisterActionMap(xiiStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  XII_ASSERT_ALWAYS(it.IsValid(), "Mapping '{}' not found", sMapping);
  XII_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
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
  xiiActionMapManager::RegisterActionMap("DocumentWindowTabMenu");
  xiiDocumentActions::MapMenuActions("DocumentWindowTabMenu", "");
}

void xiiActionMapManager::Shutdown()
{
  xiiActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");

  while (!s_Mappings.IsEmpty())
  {
    UnregisterActionMap(s_Mappings.GetIterator().Key());
  }
}
