/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Settings/EditorSettings.h>
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorSettings, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorSettings)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ToolsFoundation"
  END_SUBSYSTEM_DEPENDENCIES
  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiEditorSettings::PluginEventHandler);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiEditorSettings::PluginEventHandler);
    xiiEditorSettings::ClearApplicationSettings();
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, xiiEditorSettings::Entry>> xiiEditorSettings::s_Settings;
xiiEvent<const xiiDocument*>                                                 xiiEditorSettings::s_SettingsInvalidatedEvent;

xiiEditorSettings::xiiEditorSettings(Domain domain, const char* szUniqueName) :
  m_Domain(domain), m_sUniqueName(szUniqueName)
{
}

xiiSettings* xiiEditorSettings::QuerySettings(const xiiRTTI* pRtti, const xiiDocument* pDocument)
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

  if (auto itDocument = s_Settings.Find(pDocument); itDocument.IsValid())
  {
    if (auto it = itDocument.Value().Find(pRtti); it.IsValid())
      return it.Value().m_pSettings;
  }

  // All discoverable types declare their scope explicitly.
  if (pScope == nullptr)
    return nullptr;

  Entry entry;
  entry.m_pSettings = xiiSettings::Create(pRtti);
  if (entry.m_pSettings == nullptr)
    return nullptr;

  auto* pPreferences = xiiDynamicCast<xiiEditorSettings*>(entry.m_pSettings);
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
  for (const auto& existing : s_Settings[pDocument])
  {
    if (GetFilePath(existing.Value(), pDocument).IsEqual_NoCase(GetFilePath(entry, pDocument)))
    {
      xiiLog::Error("Settings type '{}' conflicts with persisted identity '{}'.", pRtti->GetTypeName(), entry.m_sName);
      pRtti->GetAllocator()->Deallocate(entry.m_pSettings);
      return nullptr;
    }
  }

  xiiFileReader file;
  xiiString     path = GetFilePath(entry, pDocument);
  // The initial unified adapter stored direct project settings beside per-user preferences.
  // Read that data only when no shared file exists; future saves use the shared location.
  if (!pPreferences && entry.m_Domain == Domain::Project && !xiiFileSystem::ExistsFile(path))
  {
    xiiStringBuilder previous(xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder(), "/", entry.m_sName, ".pref");
    if (xiiFileSystem::ExistsFile(previous))
      path = previous;
  }
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
  s_Settings[pDocument].Insert(pRtti, entry);
  entry.m_pSettings->NotifyChanged();
  return entry.m_pSettings;
}

xiiString xiiEditorSettings::GetFilePath(const Entry& entry, const xiiDocument* pDocument)
{
  auto*            pServices = xiiApplicationServices::GetSingleton();
  xiiStringBuilder path;
  switch (entry.m_Domain)
  {
    case Domain::Application:
      path = pServices->GetApplicationPreferencesFolder();
      break;
    case Domain::Project:
      if (entry.m_pSettings->GetDynamicRTTI()->IsDerivedFrom<xiiEditorSettings>())
        path = pServices->GetProjectPreferencesFolder();
      else
        path.Set(xiiToolsProject::GetSingleton()->GetProjectDirectory(), "/Editor/Settings");
      break;
    case Domain::Document:
      path = pServices->GetDocumentPreferencesFolder(pDocument);
      break;
  }
  path.AppendPath(entry.m_sName);
  if (entry.m_pSettings != nullptr && !entry.m_pSettings->GetDynamicRTTI()->IsDerivedFrom<xiiEditorSettings>())
    path.Append(".pref");
  else
    path.ChangeFileExtension("pref");
  return path;
}

xiiResult xiiEditorSettings::Save(const Entry& entry, const xiiDocument* pDocument)
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

xiiResult xiiEditorSettings::SaveSettings(xiiSettings* pSettings, const xiiSettings* pSource)
{
  for (auto& document : s_Settings)
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

void xiiEditorSettings::SaveScope(const xiiDocument* pDocument, Domain domain)
{
  auto itDocument = s_Settings.Find(pDocument);
  if (!itDocument.IsValid())
    return;
  for (const auto& type : itDocument.Value())
    if (type.Value().m_Domain == domain && Save(type.Value(), pDocument).Failed())
      xiiLog::Error("Failed to save settings '{}'.", GetFilePath(type.Value(), pDocument));
}

void xiiEditorSettings::ClearScope(const xiiDocument* pDocument, Domain domain)
{
  s_SettingsInvalidatedEvent.Broadcast(pDocument);
  auto itDocument = s_Settings.Find(pDocument);
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
    s_Settings.Remove(itDocument);
}

void xiiEditorSettings::SaveDocumentSettings(const xiiDocument* pDocument) { SaveScope(pDocument, Domain::Document); }
void xiiEditorSettings::ClearDocumentSettings(const xiiDocument* pDocument) { ClearScope(pDocument, Domain::Document); }
void xiiEditorSettings::SaveProjectSettings() { SaveScope(nullptr, Domain::Project); }
void xiiEditorSettings::ClearProjectSettings() { ClearScope(nullptr, Domain::Project); }
void xiiEditorSettings::SaveApplicationSettings() { SaveScope(nullptr, Domain::Application); }
void xiiEditorSettings::ClearApplicationSettings() { ClearScope(nullptr, Domain::Application); }

void xiiEditorSettings::GatherSettings(xiiDynamicArray<Settings>& out_settings, const xiiDocument* pDocument)
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
  for (const auto& document : s_Settings)
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

void xiiEditorSettings::PluginEventHandler(const xiiPluginEvent& event)
{
  if (event.m_EventType != xiiPluginEvent::AfterStartupShutdown)
    return;
  s_SettingsInvalidatedEvent.Broadcast(nullptr);
  for (auto& document : s_Settings)
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
