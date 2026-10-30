/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Settings.h>
#include <Foundation/Containers/Map.h>

class xiiDocument;

/// Optional editor context restriction. The RTTI name is resolved before constructing document settings.
class XII_EDITORFRAMEWORK_DLL xiiDocumentSettingsAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentSettingsAttribute, xiiPropertyAttribute);

public:
  xiiDocumentSettingsAttribute() = default;
  explicit xiiDocumentSettingsAttribute(xiiStringView sDocumentType) : m_sDocumentType(sDocumentType) {}
  xiiStringView GetDocumentType() const { return m_sDocumentType; }

private:
  xiiUntrackedString m_sDocumentType;
};

/// Editor ownership and persistence adapter for reflected user settings.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// The constructor's Domain selects application, project or document user preferences.
/// Declare matching scope metadata for discovery before the first query. Shared project configuration
/// retains its own project/profile owner rather than using these user preference folders.
class XII_EDITORFRAMEWORK_DLL xiiEditorSettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorSettings, xiiSettings);

public:
  enum class Domain
  {
    Application,
    Project,
    Document
  };

  /// Editor ownership adapter for settings from any module. New types declare scope through RTTI metadata.
  /// Borrowed pointers expire when their context closes or their plugin unloads.
  static xiiSettings* QuerySettings(const xiiRTTI* pRtti, const xiiDocument* pDocument = nullptr);

  template <typename TYPE>
  static TYPE* QuerySettings(const xiiDocument* pDocument = nullptr)
  {
    static_assert(std::is_base_of_v<xiiSettings, TYPE>);
    return static_cast<TYPE*>(QuerySettings(xiiGetStaticRTTI<TYPE>(), pDocument));
  }

  struct Settings
  {
    xiiSettings*       m_pSettings = nullptr;
    const xiiDocument* m_pDocument = nullptr;
    Domain             m_Domain    = Domain::Application;
    xiiString          m_sName;
  };

  /// Discovers eligible types before gathering application, current project and optional document settings.
  static void      GatherSettings(xiiDynamicArray<Settings>& out_settings, const xiiDocument* pDocument = nullptr);
  static xiiResult SaveSettings(xiiSettings* pSettings, const xiiSettings* pSource = nullptr);

  /// Sent before owned objects are destroyed. UI copies must be released synchronously.
  static xiiEvent<const xiiDocument*> s_SettingsInvalidatedEvent;

  /// Saves all preferences that are tied to the given document
  static void SaveDocumentSettings(const xiiDocument* pDocument);

  /// Removes all preferences for the given document. Does not save them.
  /// A subsequent query or discovery in a valid context can create them again.
  static void ClearDocumentSettings(const xiiDocument* pDocument);

  /// Saves all project specific preferences.
  static void SaveProjectSettings();

  /// Removes all project specific preferences. Does not save them.
  /// A subsequent query or discovery in an open project can create them again.
  static void ClearProjectSettings();

  /// Saves all application specific preferences.
  static void SaveApplicationSettings();

  /// Removes all application specific preferences. Does not save them.
  /// A subsequent query or discovery can create them again.
  static void ClearApplicationSettings();

  /// Whether the preferences are app, project or document specific
  Domain GetDomain() const { return m_Domain; }

protected:
  xiiEditorSettings(Domain domain, const char* szUniqueName);


private:
  static void SaveScope(const xiiDocument* pDocument, Domain domain);
  static void ClearScope(const xiiDocument* pDocument, Domain domain);

  static void PluginEventHandler(const xiiPluginEvent& event);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, EditorSettings);

private:
  Domain    m_Domain;
  xiiString m_sUniqueName;

  struct Entry
  {
    xiiSettings* m_pSettings = nullptr;
    Domain       m_Domain    = Domain::Application;
    xiiString    m_sName;
    bool         m_bCanSave = true;
  };

  static xiiString                                                 GetFilePath(const Entry& entry, const xiiDocument* pDocument);
  static xiiResult                                                 Save(const Entry& entry, const xiiDocument* pDocument);
  static xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, Entry>> s_Settings;
};
