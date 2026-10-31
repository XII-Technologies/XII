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

  /// Creates a new persistent identity. Legacy projects did not have a project UUID;
  /// migration never changes document UUIDs or their path-based resolution.
  void      Initialize(xiiStringView sProjectFile);
  xiiStatus Validate() const;
  xiiStatus CheckEngineCompatibility() const;
  /// Transactional read: leaves this object unchanged on failure. Unknown optional fields survive saving.
  xiiStatus Read(xiiStreamReader& stream);
  xiiStatus Write(xiiStreamWriter& stream) const;
  /// Accepts only the exact legacy marker or a validated manifest. Does not migrate on read.
  xiiStatus Load(xiiStringView sProjectFile, bool* out_pLegacy = nullptr);
  /// Same-directory staging followed by atomic replacement; never truncates the destination.
  xiiStatus Save(xiiStringView sProjectFile) const;
  /// Migrates once, keeping an exact xiiProject.legacy backup. Must succeed before opening in tools.
  xiiStatus Migrate(xiiStringView sProjectFile) const;
  /// Resolves a canonical :project configuration filename through the mounted manifest.
  /// Legacy / exported projects without a manifest retain their established configuration paths.
  static xiiStatus ResolveConfigPath(xiiStringView sDefaultPath, xiiStringBuilder& out_sPath);

private:
  xiiString m_sOptionalFields;
};
