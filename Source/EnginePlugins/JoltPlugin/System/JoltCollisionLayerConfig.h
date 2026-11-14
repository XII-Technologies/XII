/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>

/// Project configuration for the existing 256 Jolt layers. Unnamed layers remain usable by ID.
/// Missing configuration preserves the default all-enabled matrix. Changes apply to newly created worlds.
class XII_JOLTPLUGIN_DLL xiiJoltCollisionLayerConfig
{
public:
  static constexpr xiiUInt32   LayerCount = 256;
  static constexpr const char* s_szPath   = ":project/RuntimeConfigs/JoltCollisionLayers.ddl";

  xiiJoltCollisionLayerConfig();
  const xiiString&        GetName(xiiUInt8 uiLayer) const { return m_Names[uiLayer]; }
  [[nodiscard]] xiiStatus SetName(xiiUInt8 uiLayer, xiiStringView sName);
  xiiUInt32               FindLayer(xiiStringView sName) const;
  XII_ALWAYS_INLINE bool  IsCollisionEnabled(xiiUInt8 uiA, xiiUInt8 uiB) const
  {
    return (m_Masks[uiA][uiB / 64] & (xiiUInt64{1} << (uiB % 64))) != 0;
  }
  void                    SetCollisionEnabled(xiiUInt8 uiA, xiiUInt8 uiB, bool bEnabled);
  [[nodiscard]] xiiStatus Load(xiiStringView sPath = s_szPath);
  [[nodiscard]] xiiStatus Save(xiiStringView sPath = s_szPath) const;

private:
  xiiString m_Names[LayerCount];
  xiiUInt64 m_Masks[LayerCount][4];
};
