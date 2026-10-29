/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

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

/// Base class for all preferences.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// The constructor's Domain selects application, project or document user preferences.
/// Declare matching scope metadata for discovery before the first query. Shared project configuration
/// retains its own project/profile owner rather than using these user preference folders.
class XII_EDITORFRAMEWORK_DLL xiiPreferences : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPreferences, xiiSettings);

public:
  enum class Domain
  {
    Application,
    Project,
    Document
  };

  /// Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  template <typename TYPE>
  static TYPE* QueryPreferences(const xiiDocument* pDocument = nullptr)
  {
    static_assert((std::is_base_of<xiiPreferences, TYPE>::value == true), "All preferences objects must be derived from xiiPreferences");
    return static_cast<TYPE*>(QueryPreferences(xiiGetStaticRTTI<TYPE>(), pDocument));
  }

  /// Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  static xiiPreferences* QueryPreferences(const xiiRTTI* pRtti, const xiiDocument* pDocument = nullptr);

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
  static void SaveDocumentPreferences(const xiiDocument* pDocument);

  /// Removes all preferences for the given document. Does not save them.
  /// A subsequent query or discovery in a valid context can create them again.
  static void ClearDocumentPreferences(const xiiDocument* pDocument);

  /// Saves all project specific preferences.
  static void SaveProjectPreferences();

  /// Removes all project specific preferences. Does not save them.
  /// A subsequent query or discovery in an open project can create them again.
  static void ClearProjectPreferences();

  /// Saves all application specific preferences.
  static void SaveApplicationPreferences();

  /// Removes all application specific preferences. Does not save them.
  /// A subsequent query or discovery can create them again.
  static void ClearApplicationPreferences();

  //// Fills the list with all currently known preferences
  static void GatherAllPreferences(xiiHybridArray<xiiPreferences*, 16>& out_allPreferences);

  /// Whether the preferences are app, project or document specific
  Domain GetDomain() const { return m_Domain; }

  /// Within the same domain and visibility the name must be unique, but across those it can be reused.
  xiiString GetName() const;

  /// If these preferences are per document, the pointer is valid, otherwise nullptr.
  const xiiDocument* GetDocumentAssociation() const { return m_pDocument; }

  /// A simple event that can be fired when any preference property changes. No specific change details are given.
  xiiEvent<xiiPreferences*> m_ChangedEvent;

  /// Call this to broadcast that this preference object was modified.
  void TriggerPreferencesChangedEvent() { NotifyChanged(); }

protected:
  xiiPreferences(Domain domain, const char* szUniqueName);

  xiiString GetFilePath() const;
  void      OnChanged() override { m_ChangedEvent.Broadcast(this); }

private:
  static void SavePreferences(const xiiDocument* pDocument, Domain domain);
  static void ClearPreferences(const xiiDocument* pDocument, Domain domain);

  static void PluginEventHandler(const xiiPluginEvent& event);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, Preferences);

private:
  Domain             m_Domain;
  xiiString          m_sUniqueName;
  const xiiDocument* m_pDocument;

  struct Entry
  {
    xiiSettings* m_pSettings = nullptr;
    Domain       m_Domain    = Domain::Application;
    xiiString    m_sName;
    bool         m_bCanSave = true;
  };

  static xiiString                                                 GetFilePath(const Entry& entry, const xiiDocument* pDocument);
  static xiiResult                                                 Save(const Entry& entry, const xiiDocument* pDocument);
  static xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, Entry>> s_Preferences;
};
