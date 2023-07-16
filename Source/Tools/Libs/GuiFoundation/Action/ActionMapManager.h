#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class XII_GUIFOUNDATION_DLL xiiActionMapManager
{
public:
  static xiiResult     RegisterActionMap(const char* szMapping);
  static xiiResult     UnregisterActionMap(const char* szMapping);
  static xiiActionMap* GetActionMap(const char* szMapping);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static xiiMap<xiiString, xiiActionMap*> s_Mappings;
};
