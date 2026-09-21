/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Component.h>

/// Base class for settings components, of which only one per type should exist in each world.
///
/// Settings components are used to store global scene specific settings, e.g. for physics it would be the scene gravity,
/// for rendering it might be the time of day, fog settings, etc.
///
/// Components of this type should be managed by a xiiSettingsComponentManager, which makes it easy to query for the one instance
/// in the world.
class XII_CORE_DLL xiiSettingsComponent : public xiiComponent
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettingsComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiSettingsComponent

public:
  /// The constructor marks the component as modified.
  xiiSettingsComponent();
  ~xiiSettingsComponent();

  /// Marks the component as modified. Individual bits can be used to mark only specific settings (groups) as modified.
  void SetModified(xiiUInt32 uiBits = 0xFFFFFFFF) { m_uiSettingsModified |= uiBits; }

  /// Checks whether the component (or some settings group) was marked as modified.
  bool IsModified(xiiUInt32 uiBits = 0xFFFFFFFF) const { return (m_uiSettingsModified & uiBits) != 0; }

  /// Marks the settings as not-modified.
  void ResetModified(xiiUInt32 uiBits = 0xFFFFFFFF) { m_uiSettingsModified &= ~uiBits; }

private:
  xiiUInt32 m_uiSettingsModified = 0xFFFFFFFF;
};
