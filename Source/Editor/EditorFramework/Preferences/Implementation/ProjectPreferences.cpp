#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProjectPreferencesUser, 1, xiiRTTIDefaultAllocator<xiiProjectPreferencesUser>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Players", m_PlayerApps)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ExportFolder", m_sExportFolder)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiProjectPreferencesUser::xiiProjectPreferencesUser() :
  xiiPreferences(Domain::Project, "General")
{
}

void xiiQtEditorApp::LoadProjectPreferences()
{
  XII_PROFILE_SCOPE("LoadProjectPreferences");
  xiiPreferences::QueryPreferences<xiiProjectPreferencesUser>();
}
