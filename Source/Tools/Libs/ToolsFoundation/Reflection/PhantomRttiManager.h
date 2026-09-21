/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiPhantomRTTI;

struct xiiPhantomRttiManagerEvent
{
  enum class Type
  {
    TypeAdded,
    TypeRemoved,
    TypeChanged,
  };

  xiiPhantomRttiManagerEvent() = default;

  Type           m_Type         = Type::TypeAdded;
  const xiiRTTI* m_pChangedType = nullptr;
};

/// Manages all xiiPhantomRTTI types that have been added to him.
///
/// A xiiPhantomRTTI cannot be created directly but must be created via this managers
/// RegisterType function with a given xiiReflectedTypeDescriptor.
class XII_TOOLSFOUNDATION_DLL xiiPhantomRttiManager
{
public:
  /// Adds a reflected type to the list of accessible types.
  ///
  /// Types must be added in the correct order, any type must be added before
  /// it can be referenced in other types. Any base class must be added before
  /// any class deriving from it can be added.
  /// Call the function again if a type has changed during the run of the
  /// program. If the type actually differs the last known class layout the
  /// m_TypeChangedEvent event will be called with the old and new xiiRTTI.
  ///
  /// \sa xiiReflectionUtils::GetReflectedTypeDescriptorFromRtti
  static const xiiRTTI* RegisterType(xiiReflectedTypeDescriptor& ref_desc);

  /// Removes a type from the list of accessible types.
  ///
  /// No instance of the given type or storage must still exist when this function is called.
  static bool UnregisterType(const xiiRTTI* pRtti);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeManager);

  static void Startup();
  static void Shutdown();
  static void PluginEventHandler(const xiiPluginEvent& e);

public:
  static xiiCopyOnBroadcastEvent<const xiiPhantomRttiManagerEvent&> s_Events;

private:
  static xiiSet<const xiiRTTI*>                       s_RegisteredConcreteTypes;
  static xiiHashTable<xiiStringView, xiiPhantomRTTI*> s_NameToPhantom;
};
