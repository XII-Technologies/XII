/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocument;

class XII_TOOLSFOUNDATION_DLL xiiApplicationServices
{
  XII_DECLARE_SINGLETON(xiiApplicationServices);

public:
  xiiApplicationServices();

  /// A writable folder in which application specific user data may be stored.
  xiiString GetApplicationUserDataFolder() const;

  /// A read-only folder in which application specific data may be located.
  xiiString GetApplicationDataFolder() const;

  /// The writable location where the application should store preferences (user specific settings).
  xiiString GetApplicationPreferencesFolder() const;

  /// The writable location where preferences for the current xiiToolsProject should be stored (user specific settings).
  xiiString GetProjectPreferencesFolder() const;

  xiiString GetProjectPreferencesFolder(xiiStringView sProjectFilePath) const;

  /// The writable location where preferences for the given xiiDocument should be stored (user specific settings).
  xiiString GetDocumentPreferencesFolder(const xiiDocument* pDocument) const;

  /// The read-only folder where pre-compiled binaries for external tools can be found.
  xiiString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;

  /// The folder under which the sample projects are stored.
  xiiString GetSampleProjectsFolder() const;
};
