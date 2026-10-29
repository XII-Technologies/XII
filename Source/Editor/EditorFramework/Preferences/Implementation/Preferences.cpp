/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentSettingsAttribute, 1, xiiRTTIDefaultAllocator<xiiDocumentSettingsAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPreferences, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, Preferences)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ToolsFoundation"
  END_SUBSYSTEM_DEPENDENCIES
  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiPreferences::PluginEventHandler);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiPreferences::PluginEventHandler);
    xiiPreferences::ClearApplicationPreferences();
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, xiiPreferences::Entry>> xiiPreferences::s_Preferences;
xiiEvent<const xiiDocument*>                                              xiiPreferences::s_SettingsInvalidatedEvent;

xiiPreferences::xiiPreferences(Domain domain, const char* szUniqueName) :
  m_Domain(domain), m_sUniqueName(szUniqueName), m_pDocument(nullptr)
{
}

xiiPreferences* xiiPreferences::QueryPreferences(const xiiRTTI* pRtti, const xiiDocument* pDocument)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<xiiPreferences>())
    return nullptr;
  return static_cast<xiiPreferences*>(QuerySettings(pRtti, pDocument));
}

xiiSettings* xiiPreferences::QuerySettings(const xiiRTTI* pRtti, const xiiDocument* pDocument)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<xiiSettings>() || xiiApplicationServices::GetSingleton() == nullptr)
    return nullptr;

  if (const auto* pContext = pRtti->GetAttributeByType<xiiDocumentSettingsAttribute>())
  {
    const auto* pDocumentType = xiiRTTI::FindTypeByName(pContext->GetDocumentType());
    if (pDocument == nullptr || pDocumentType == nullptr || !pDocument->GetDynamicRTTI()->IsDerivedFrom(pDocumentType))
      return nullptr;
  }

  const auto* pScope = pRtti->GetAttributeByType<xiiSettingsScopeAttribute>();
  if (pScope != nullptr && (pScope->GetScope() < xiiSettingsScope::Application || pScope->GetScope() > xiiSettingsScope::Document || (pScope->GetScope() == xiiSettingsScope::Document) != (pDocument != nullptr) || (pScope->GetScope() != xiiSettingsScope::Application && !xiiToolsProject::IsProjectOpen())))
    return nullptr;

  if (auto itDocument = s_Preferences.Find(pDocument); itDocument.IsValid())
  {
    if (auto it = itDocument.Value().Find(pRtti); it.IsValid())
      return it.Value().m_pSettings;
  }

  // Unannotated legacy preferences retain their query API, but are not constructed speculatively.
  if (pScope == nullptr && !pRtti->IsDerivedFrom<xiiPreferences>())
    return nullptr;

  Entry entry;
  entry.m_pSettings = xiiSettings::Create(pRtti);
  if (entry.m_pSettings == nullptr)
    return nullptr;

  auto* pPreferences = xiiDynamicCast<xiiPreferences*>(entry.m_pSettings);
  entry.m_Domain     = pPreferences ? pPreferences->GetDomain() : static_cast<Domain>(pScope->GetScope());
  if (pPreferences)
    entry.m_sName = pPreferences->m_sUniqueName;
  else
    entry.m_sName = xiiStringBuilder("Settings_", pRtti->GetTypeName());

  if (entry.m_sName.IsEmpty() || xiiPathUtils::ContainsInvalidFilenameChars(entry.m_sName) ||
      (entry.m_Domain == Domain::Document) != (pDocument != nullptr) ||
      (entry.m_Domain != Domain::Application && !xiiToolsProject::IsProjectOpen()) ||
      (pScope != nullptr && pScope->GetScope() != static_cast<xiiInt32>(entry.m_Domain)))
  {
    pRtti->GetAllocator()->Deallocate(entry.m_pSettings);
    return nullptr;
  }

  // Never let two types silently share a persisted identity within one scope.
  for (const auto& existing : s_Preferences[pDocument])
  {
    if (GetFilePath(existing.Value(), pDocument).IsEqual_NoCase(GetFilePath(entry, pDocument)))
    {
      xiiLog::Error("Settings type '{}' conflicts with persisted identity '{}'.", pRtti->GetTypeName(), entry.m_sName);
      pRtti->GetAllocator()->Deallocate(entry.m_pSettings);
      return nullptr;
    }
  }

  if (pPreferences)
    pPreferences->m_pDocument = pDocument;

  xiiFileReader   file;
  const xiiString path = GetFilePath(entry, pDocument);
  if (file.Open(path).Succeeded())
  {
    entry.m_bCanSave = entry.m_pSettings->ReadFromDDL(file).Succeeded();
    if (!entry.m_bCanSave)
      xiiLog::Warning("Invalid settings '{}'; using defaults and preserving the original file. Repair or move it before saving.", path);
  }
  else if (xiiFileSystem::ExistsFile(path))
  {
    entry.m_bCanSave = false;
    xiiLog::Warning("Unable to read settings '{}'; preserving the original file.", path);
  }
  s_Preferences[pDocument].Insert(pRtti, entry);
  entry.m_pSettings->NotifyChanged();
  return entry.m_pSettings;
}

xiiString xiiPreferences::GetFilePath(const Entry& entry, const xiiDocument* pDocument)
{
  auto*            pServices = xiiApplicationServices::GetSingleton();
  xiiStringBuilder path;
  switch (entry.m_Domain)
  {
    case Domain::Application:
      path = pServices->GetApplicationPreferencesFolder();
      break;
    case Domain::Project:
      path = pServices->GetProjectPreferencesFolder();
      break;
    case Domain::Document:
      path = pServices->GetDocumentPreferencesFolder(pDocument);
      break;
  }
  path.AppendPath(entry.m_sName);
  if (entry.m_pSettings != nullptr && !entry.m_pSettings->GetDynamicRTTI()->IsDerivedFrom<xiiPreferences>())
    path.Append(".pref");
  else
    path.ChangeFileExtension("pref");
  return path;
}

xiiString xiiPreferences::GetFilePath() const
{
  Entry entry;
  entry.m_Domain = m_Domain;
  entry.m_sName  = m_sUniqueName;
  return GetFilePath(entry, m_pDocument);
}

xiiResult xiiPreferences::Save(const Entry& entry, const xiiDocument* pDocument)
{
  if (!entry.m_bCanSave)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(entry.m_pSettings->Validate());

  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  entry.m_pSettings->GetDynamicRTTI()->GetAllProperties(properties);
  bool bHasProperties = false;
  for (const auto* pProperty : properties)
    bHasProperties |= pProperty->GetCategory() != xiiPropertyCategory::Constant && !pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly);
  if (!bHasProperties)
    return XII_SUCCESS;

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiReflectionSerializer::WriteObjectToDDL(writer, entry.m_pSettings->GetDynamicRTTI(), entry.m_pSettings, false, xiiOpenDdlWriter::TypeStringMode::Compliant);
  xiiDeferredFileWriter file;
  file.SetOutput(GetFilePath(entry, pDocument));
  XII_SUCCEED_OR_RETURN(storage.CopyToStream(file));
  return file.Close();
}

xiiResult xiiPreferences::SaveSettings(xiiSettings* pSettings, const xiiSettings* pSource)
{
  for (auto& document : s_Preferences)
    for (auto& type : document.Value())
      if (type.Value().m_pSettings == pSettings)
      {
        Entry entry = type.Value();
        if (pSource != nullptr)
        {
          if (pSource->GetDynamicRTTI() != pSettings->GetDynamicRTTI())
            return XII_FAILURE;
          entry.m_pSettings = const_cast<xiiSettings*>(pSource);
        }
        return Save(entry, document.Key());
      }
  return XII_FAILURE;
}

void xiiPreferences::SavePreferences(const xiiDocument* pDocument, Domain domain)
{
  auto itDocument = s_Preferences.Find(pDocument);
  if (!itDocument.IsValid())
    return;
  for (const auto& type : itDocument.Value())
    if (type.Value().m_Domain == domain && Save(type.Value(), pDocument).Failed())
      xiiLog::Error("Failed to save settings '{}'.", GetFilePath(type.Value(), pDocument));
}

void xiiPreferences::ClearPreferences(const xiiDocument* pDocument, Domain domain)
{
  s_SettingsInvalidatedEvent.Broadcast(pDocument);
  auto itDocument = s_Preferences.Find(pDocument);
  if (!itDocument.IsValid())
    return;
  auto& preferences = itDocument.Value();
  for (auto it = preferences.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_Domain == domain)
    {
      auto* pSettings = it.Value().m_pSettings;
      pSettings->GetDynamicRTTI()->GetAllocator()->Deallocate(pSettings);
      it = preferences.Remove(it);
    }
    else
      ++it;
  }
  if (preferences.IsEmpty())
    s_Preferences.Remove(itDocument);
}

void xiiPreferences::SaveDocumentPreferences(const xiiDocument* pDocument) { SavePreferences(pDocument, Domain::Document); }
void xiiPreferences::ClearDocumentPreferences(const xiiDocument* pDocument) { ClearPreferences(pDocument, Domain::Document); }
void xiiPreferences::SaveProjectPreferences() { SavePreferences(nullptr, Domain::Project); }
void xiiPreferences::ClearProjectPreferences() { ClearPreferences(nullptr, Domain::Project); }
void xiiPreferences::SaveApplicationPreferences() { SavePreferences(nullptr, Domain::Application); }
void xiiPreferences::ClearApplicationPreferences() { ClearPreferences(nullptr, Domain::Application); }

void xiiPreferences::GatherAllPreferences(xiiHybridArray<xiiPreferences*, 16>& out_allPreferences)
{
  out_allPreferences.Clear();
  for (const auto& document : s_Preferences)
    for (const auto& type : document.Value())
      if (auto* pPreferences = xiiDynamicCast<xiiPreferences*>(type.Value().m_pSettings))
        out_allPreferences.PushBack(pPreferences);
}

void xiiPreferences::GatherSettings(xiiDynamicArray<Settings>& out_settings, const xiiDocument* pDocument)
{
  out_settings.Clear();
  xiiDynamicArray<const xiiRTTI*> types;
  for (xiiInt32 iScope = xiiSettingsScope::Application; iScope <= xiiSettingsScope::Document; ++iScope)
  {
    if (iScope != xiiSettingsScope::Application && !xiiToolsProject::IsProjectOpen())
      continue;
    if (iScope == xiiSettingsScope::Document && pDocument == nullptr)
      continue;
    xiiSettings::GatherTypes(static_cast<xiiSettingsScope::Enum>(iScope), types);
    for (const auto* pType : types)
      QuerySettings(pType, iScope == xiiSettingsScope::Document ? pDocument : nullptr);
  }
  for (const auto& document : s_Preferences)
  {
    if (document.Key() != nullptr && document.Key() != pDocument)
      continue;
    for (const auto& type : document.Value())
    {
      const auto& entry = type.Value();
      Settings    settings;
      settings.m_pSettings = entry.m_pSettings;
      settings.m_pDocument = document.Key();
      settings.m_Domain    = entry.m_Domain;
      settings.m_sName     = entry.m_sName;
      out_settings.PushBack(settings);
    }
  }
}

void xiiPreferences::PluginEventHandler(const xiiPluginEvent& event)
{
  if (event.m_EventType != xiiPluginEvent::AfterStartupShutdown)
    return;
  s_SettingsInvalidatedEvent.Broadcast(nullptr);
  for (auto& document : s_Preferences)
  {
    auto& preferences = document.Value();
    for (auto it = preferences.GetIterator(); it.IsValid();)
    {
      if (it.Key()->GetPluginName() == event.m_sPluginBinary)
      {
        if (Save(it.Value(), document.Key()).Failed())
          xiiLog::Error("Failed to save settings before unloading '{}'.", event.m_sPluginBinary);
        it.Key()->GetAllocator()->Deallocate(it.Value().m_pSettings);
        it = preferences.Remove(it);
      }
      else
        ++it;
    }
  }
}

xiiString xiiPreferences::GetName() const
{
  xiiStringBuilder name;
  if (m_Domain == Domain::Document)
    name.Set(m_sUniqueName, ": ", m_pDocument ? xiiPathUtils::GetFileName(m_pDocument->GetDocumentPath()) : xiiStringView());
  else
    name.Set(m_Domain == Domain::Application ? "Application: " : "Project: ", m_sUniqueName);
  return name;
}
