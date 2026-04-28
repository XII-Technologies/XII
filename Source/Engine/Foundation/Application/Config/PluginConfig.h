/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class XII_FOUNDATION_DLL xiiApplicationPluginConfig
{
public:
  xiiApplicationPluginConfig();

  static constexpr const xiiStringView s_sConfigFile = ":project/RuntimeConfigs/Plugins.ddl"_xiisv;

  xiiResult Save(xiiStringView sConfigPath = s_sConfigFile) const;
  void      Load(xiiStringView sConfigPath = s_sConfigFile);
  void      Apply();

  struct XII_FOUNDATION_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    xiiString m_sAppDirRelativePath;
    bool      m_bLoadCopy = false;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  mutable xiiHybridArray<PluginConfig, 8> m_Plugins;
};


using xiiApplicationPluginConfig_PluginConfig = xiiApplicationPluginConfig::PluginConfig;

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiApplicationPluginConfig);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiApplicationPluginConfig_PluginConfig);
