/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class XII_FOUNDATION_DLL xiiApplicationFileSystemConfig
{
public:
  static constexpr const xiiStringView s_sConfigFile = ":project/RuntimeConfigs/DataDirectories.ddl"_xiisv;

  xiiResult Save(xiiStringView sPath = s_sConfigFile);
  void      Load(xiiStringView sPath = s_sConfigFile);

  /// Sets up the data directories that were configured or loaded into this object
  void Apply();

  /// Removes all data directories that were set up by any call to xiiApplicationFileSystemConfig::Apply()
  static void Clear();

  xiiResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    xiiString m_sDataDirSpecialPath;
    xiiString m_sRootName;
    bool      m_bWritable;            ///< Whether the directory is going to be mounted for writing
    bool      m_bHardCodedDependency; ///< If set to true, this indicates that it may not be removed by the user (in a config dialog)

    DataDirConfig()
    {
      m_bWritable            = false;
      m_bHardCodedDependency = false;
    }

    bool operator==(const DataDirConfig& rhs) const
    {
      return m_bWritable == rhs.m_bWritable && m_sDataDirSpecialPath == rhs.m_sDataDirSpecialPath && m_sRootName == rhs.m_sRootName;
    }
  };

  bool operator==(const xiiApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  xiiHybridArray<DataDirConfig, 4> m_DataDirs;
};

using xiiApplicationFileSystemConfig_DataDirConfig = xiiApplicationFileSystemConfig::DataDirConfig;

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiApplicationFileSystemConfig);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiApplicationFileSystemConfig_DataDirConfig);
