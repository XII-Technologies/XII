/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPreferences, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMap<const xiiDocument*, xiiMap<const xiiRTTI*, xiiPreferences*>> xiiPreferences::s_Preferences;

xiiPreferences::xiiPreferences(Domain domain, const char* szUniqueName)
{
  m_Domain      = domain;
  m_sUniqueName = szUniqueName;
  m_pDocument   = nullptr;
}

xiiPreferences* xiiPreferences::QueryPreferences(const xiiRTTI* pRtti, const xiiDocument* pDocument)
{
  XII_ASSERT_DEV(xiiQtEditorApp::GetSingleton() != nullptr, "Editor app is not available in this process");

  auto it = s_Preferences[pDocument].Find(pRtti);

  if (it.IsValid())
    return it.Value();

  auto pAlloc = pRtti->GetAllocator();
  XII_ASSERT_DEV(pAlloc != nullptr, "Invalid allocator for preferences type");

  if (!pAlloc->CanAllocate())
  {
    XII_ASSERT_DEV(pAlloc->CanAllocate(), "Cannot create a preferences object that does not have a proper allocator");
    return nullptr;
  }

  xiiPreferences* pPref           = pAlloc->Allocate<xiiPreferences>();
  pPref->m_pDocument              = pDocument;
  s_Preferences[pDocument][pRtti] = pPref;

  if (pPref->m_Domain == Domain::Document)
  {
    XII_ASSERT_DEV(pDocument != nullptr, "Preferences of this type can only be used per document");
  }
  else
  {
    XII_ASSERT_DEV(pDocument == nullptr, "Preferences of this type cannot be used with a document");
  }

  pPref->Load();
  return pPref;
}

xiiString xiiPreferences::GetFilePath() const
{
  xiiStringBuilder path;

  if (m_Domain == Domain::Application)
  {
    path = xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Project)
  {
    path = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Document)
  {
    path = xiiApplicationServices::GetSingleton()->GetDocumentPreferencesFolder(m_pDocument);
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  return path;
}

void xiiPreferences::Load()
{
  xiiFileReader file;
  if (file.Open(GetFilePath()).Failed())
    return;

  xiiReflectionSerializer::ReadObjectPropertiesFromDDL(file, *GetDynamicRTTI(), this);
}

void xiiPreferences::Save() const
{
  bool bNothingToSerialize = true;

  xiiHybridArray<const xiiAbstractProperty*, 32> allProperties;
  GetDynamicRTTI()->GetAllProperties(allProperties);

  for (const xiiAbstractProperty* pProp : allProperties)
  {
    if (pProp->GetCategory() == xiiPropertyCategory::Constant || pProp->GetFlags().IsAnySet(xiiPropertyFlags::ReadOnly))
      continue;

    bNothingToSerialize = false;
    break;
  }

  if (bNothingToSerialize)
    return;

  xiiDeferredFileWriter file;
  file.SetOutput(GetFilePath());

  xiiReflectionSerializer::WriteObjectToDDL(file, GetDynamicRTTI(), this, false, xiiOpenDdlWriter::TypeStringMode::Compliant);

  if (file.Close().Failed())
    xiiLog::Error("Failed to open file for writing '{0}'.", GetFilePath());
}


void xiiPreferences::SavePreferences(const xiiDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid(); ++it)
  {
    auto pPref = it.Value();

    if (pPref->m_Domain == domain)
      pPref->Save();
  }
}

void xiiPreferences::ClearPreferences(const xiiDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid();)
  {
    xiiPreferences* pPref = it.Value();

    if (pPref->m_Domain == domain)
    {
      pPref->GetDynamicRTTI()->GetAllocator()->Deallocate(pPref);
      it = docPrefs.Remove(it);
    }
    else
      ++it;
  }
}

void xiiPreferences::SaveDocumentPreferences(const xiiDocument* pDocument)
{
  SavePreferences(pDocument, Domain::Document);
}

void xiiPreferences::ClearDocumentPreferences(const xiiDocument* pDocument)
{
  ClearPreferences(pDocument, Domain::Document);
}

void xiiPreferences::SaveProjectPreferences()
{
  SavePreferences(nullptr, Domain::Project);
}

void xiiPreferences::ClearProjectPreferences()
{
  ClearPreferences(nullptr, Domain::Project);
}

void xiiPreferences::SaveApplicationPreferences()
{
  SavePreferences(nullptr, Domain::Application);
}

void xiiPreferences::ClearApplicationPreferences()
{
  ClearPreferences(nullptr, Domain::Application);
}

void xiiPreferences::GatherAllPreferences(xiiHybridArray<xiiPreferences*, 16>& out_allPreferences)
{
  out_allPreferences.Clear();
  out_allPreferences.Reserve(s_Preferences.GetCount() * 2);

  for (auto itDoc = s_Preferences.GetIterator(); itDoc.IsValid(); ++itDoc)
  {
    for (auto itType = itDoc.Value().GetIterator(); itType.IsValid(); ++itType)
    {
      out_allPreferences.PushBack(itType.Value());
    }
  }
}

xiiString xiiPreferences::GetName() const
{
  xiiStringBuilder s;

  if (m_Domain == Domain::Document)
  {
    s.Set(m_sUniqueName, ": ");
    s.Append(xiiPathUtils::GetFileName(m_pDocument->GetDocumentPath()));
  }
  else
  {
    if (m_Domain == Domain::Application)
      s.Append("Application");
    else if (m_Domain == Domain::Project)
      s.Append("Project");

    s.Append(": ", m_sUniqueName);
  }

  return s;
}
