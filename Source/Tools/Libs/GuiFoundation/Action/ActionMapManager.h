#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class XII_GUIFOUNDATION_DLL xiiActionMapManager
{
public:
  static xiiResult     RegisterActionMap(xiiStringView sMapping);
  static xiiResult     UnregisterActionMap(xiiStringView sMapping);
  static xiiActionMap* GetActionMap(xiiStringView sMapping);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static xiiMap<xiiString, xiiActionMap*> s_Mappings;
};
