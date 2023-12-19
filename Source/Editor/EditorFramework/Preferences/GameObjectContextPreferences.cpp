#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/GameObjectContextPreferences.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectContextPreferencesUser, 1, xiiRTTIDefaultAllocator<xiiGameObjectContextPreferencesUser>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ContextDocument", m_ContextDocument)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("ContextObject", m_ContextObject)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGameObjectContextPreferencesUser::xiiGameObjectContextPreferencesUser() :
  xiiPreferences(Domain::Document, "GameObjectContext")
{
}

xiiUuid xiiGameObjectContextPreferencesUser::GetContextDocument() const
{
  return m_ContextDocument;
}


void xiiGameObjectContextPreferencesUser::SetContextDocument(xiiUuid val)
{
  m_ContextDocument = val;
  TriggerPreferencesChangedEvent();
}

xiiUuid xiiGameObjectContextPreferencesUser::GetContextObject() const
{
  return m_ContextObject;
}

void xiiGameObjectContextPreferencesUser::SetContextObject(xiiUuid val)
{
  m_ContextObject = val;
  TriggerPreferencesChangedEvent();
}
