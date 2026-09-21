/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>

class xiiDocument;

/// Base class for all preferences.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// Pass the 'Domain' and 'Visibility' to the constructor to configure whether the preference class
/// is per application, per project or per document, and whether the data is shared among all users
/// or custom for every user.
class XII_EDITORFRAMEWORK_DLL xiiPreferences : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPreferences, xiiReflectedClass);

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

  /// Saves all preferences that are tied to the given document
  static void SaveDocumentPreferences(const xiiDocument* pDocument);

  /// Removes all preferences for the given document. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearDocumentPreferences(const xiiDocument* pDocument);

  /// Saves all project specific preferences.
  static void SaveProjectPreferences();

  /// Removes all project specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearProjectPreferences();

  /// Saves all application specific preferences.
  static void SaveApplicationPreferences();

  /// Removes all application specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
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
  void TriggerPreferencesChangedEvent() { m_ChangedEvent.Broadcast(this); }

protected:
  xiiPreferences(Domain domain, const char* szUniqueName);

  xiiString GetFilePath() const;

private:
  static void SavePreferences(const xiiDocument* pDocument, Domain domain);
  static void ClearPreferences(const xiiDocument* pDocument, Domain domain);

  void Load();
  void Save() const;

private:
  Domain             m_Domain;
  xiiString          m_sUniqueName;
  const xiiDocument* m_pDocument;

  static xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, xiiPreferences*>> s_Preferences;
};
