/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Settings.h>

/// Runtime user configuration owned by xiiGameApplication. This is not an editor installation preference.
/// App.VSync remains a compatibility control; this object's reflected file is authoritative after migration.
class XII_GAMEENGINE_DLL xiiGameApplicationSettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameApplicationSettings, xiiSettings);

public:
  bool m_bVSync = true;

  /// If no reflected file exists, adopts the legacy saved CVar value. Existing reflected data wins.
  /// Failed reads leave current values intact. The caller must preserve an unreadable file.
  xiiResult Load(xiiStringView sPath, bool bLegacyVSync);
  xiiResult Save(xiiStringView sPath) const;
};
