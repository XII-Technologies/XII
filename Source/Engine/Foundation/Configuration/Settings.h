/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Reflection/Reflection.h>

class xiiStreamReader;
class xiiAbstractObjectGraph;

/// Ownership contexts, not storage backends. Platform settings remain owned by their profile.
struct xiiSettingsScope
{
  enum Enum
  {
    Application,
    Project,
    Document,
    Platform
  };
};

/// Opts a concrete settings type into context-aware discovery without constructing it.
/// Use xiiCategoryAttribute for navigation and the existing property attributes for editing.
class XII_FOUNDATION_DLL xiiSettingsScopeAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettingsScopeAttribute, xiiPropertyAttribute);

public:
  xiiSettingsScopeAttribute() = default;
  explicit xiiSettingsScopeAttribute(xiiInt32 iScope) : m_iScope(iScope) {}

  xiiInt32 GetScope() const { return m_iScope; }

private:
  xiiInt32 m_iScope = xiiSettingsScope::Application;
};

/// A property or type whose changes need explicit subsystem reinitialization or an application restart.
/// The owner decides when to apply these values. This attribute never restarts a subsystem itself.
class XII_FOUNDATION_DLL xiiSettingsRestartAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettingsRestartAttribute, xiiPropertyAttribute);
};

/// Reflected configuration independent of Qt, storage paths and global instance registries.
/// Constructors and reflected setters must only modify object state: copies are used for validation.
/// Owners load/commit on their owning thread, then notify consumers at an appropriate synchronization point.
class XII_FOUNDATION_DLL xiiSettings : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettings, xiiReflectedClass);

public:
  /// Allocates a concrete settings type and applies explicit reflected member defaults over constructor defaults.
  /// The caller owns the result and must deallocate it through its dynamic RTTI allocator.
  static xiiSettings* Create(const xiiRTTI* pType);

  /// Enumerates concrete, allocatable settings with exactly the requested scope metadata. No instances are created.
  static void GatherTypes(xiiSettingsScope::Enum scope, xiiDynamicArray<const xiiRTTI*>& out_types);

  /// Checks finite scalar values, reflected ranges and enums. Overrides should call the base implementation.
  virtual xiiResult Validate() const;

  /// Validates before copying reflected properties. Unchanged values do not emit a notification.
  xiiResult Apply(const xiiSettings& source);

  /// Compares writable reflected values, excluding read-only presentation and ownership information.
  bool HasSameValues(const xiiSettings& other) const;

  /// Transactional DDL restore. Missing properties retain current defaults; graph patches run normally.
  /// Invalid input leaves this object unchanged. Loading does not broadcast change events.
  xiiResult ReadFromDDL(xiiStreamReader& ref_stream);

  /// The same validated restore for an existing reflected graph, e.g. the editor's staged document.
  xiiResult ReadFromGraph(const xiiAbstractObjectGraph& graph);

  /// Explicit commit boundary for code modifying properties directly. Call only after successful validation.
  void NotifyChanged();

  xiiEvent<xiiSettings*> m_SettingsChangedEvent;

protected:
  virtual void OnChanged() {}
};
