#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>

/// \brief A central place for creating and retrieving action maps.
class XII_GUIFOUNDATION_DLL xiiActionMapManager
{
public:
  /// \brief Adds a new action map with the given name. Returns XII_FAILURE if the name was already used before.
  static xiiResult RegisterActionMap(xiiStringView sMapping);

  /// \brief Deletes the action map with the given name. Returns XII_FAILURE, if no such map exists.
  static xiiResult UnregisterActionMap(xiiStringView sMapping);

  /// \brief Returns the action map with the given name, or nullptr, if it doesn't exist.
  static xiiActionMap* GetActionMap(xiiStringView sMapping);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static xiiMap<xiiString, xiiActionMap*> s_Mappings;
};
