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

  /// \brief A writable folder in which application specific user data may be stored.
  xiiString GetApplicationUserDataFolder() const;

  /// \brief A read-only folder in which application specific data may be located.
  xiiString GetApplicationDataFolder() const;

  /// \brief The writable location where the application should store preferences (user specific settings).
  xiiString GetApplicationPreferencesFolder() const;

  /// \brief The writable location where preferences for the current xiiToolsProject should be stored (user specific settings).
  xiiString GetProjectPreferencesFolder() const;

  xiiString GetProjectPreferencesFolder(xiiStringView sProjectFilePath) const;

  /// \brief The writable location where preferences for the given xiiDocument should be stored (user specific settings).
  xiiString GetDocumentPreferencesFolder(const xiiDocument* pDocument) const;

  /// \brief The read-only folder where pre-compiled binaries for external tools can be found.
  xiiString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;

  /// \brief The folder under which the sample projects are stored.
  xiiString GetSampleProjectsFolder() const;
};
