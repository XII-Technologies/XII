/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>

class xiiStreamReader;
class xiiStreamWriter;

/// Portable project entry point shared by tools and runtime applications.
/// Dedicated configuration files remain authoritative for their respective settings.
class XII_FOUNDATION_DLL xiiProjectManifest
{
public:
  static constexpr xiiUInt32 s_uiFormatVersion = 1;

  xiiUuid   m_ProjectID;
  xiiString m_sName;
  xiiString m_sDescription;
  xiiUInt32 m_uiEngineMajor       = 0;
  xiiUInt32 m_uiEngineMinor       = 0;
  xiiUInt32 m_uiEnginePatch       = 0;
  xiiString m_sDataDirectories    = "RuntimeConfigs/DataDirectories.ddl";
  xiiString m_sPluginSelection    = "Editor/PluginSelection.ddl";
  xiiString m_sRuntimePlugins     = "RuntimeConfigs/Plugins.ddl";
  xiiString m_sAssetProfiles      = "Editor/AssetProfiles.ddl";
  xiiString m_sCppSettings        = "Editor/CppProject.ddl";
  xiiString m_sActiveAssetProfile = "Default";
  xiiString m_sDefaultScene;

  /// Creates a new persistent project identity independently of document UUIDs.
  void      Initialize(xiiStringView sProjectFile);
  xiiStatus Validate() const;
  xiiStatus CheckEngineCompatibility() const;
  /// Transactional read: leaves this object unchanged on failure. Unknown optional fields survive saving.
  xiiStatus Read(xiiStreamReader& stream);
  xiiStatus Write(xiiStreamWriter& stream) const;
  /// Loads a validated OpenDDL manifest, leaving this object unchanged on failure.
  xiiStatus Load(xiiStringView sProjectFile);
  /// Same-directory staging followed by atomic replacement; never truncates the destination.
  xiiStatus Save(xiiStringView sProjectFile) const;
  /// Resolves a canonical :project configuration filename through the mounted manifest.
  /// Applications without a mounted manifest retain their established configuration paths.
  static xiiStatus ResolveConfigPath(xiiStringView sDefaultPath, xiiStringBuilder& out_sPath);

private:
  xiiString m_sOptionalFields;
};
