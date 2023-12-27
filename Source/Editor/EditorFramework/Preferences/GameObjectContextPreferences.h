#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class xiiGameObjectContextPreferencesUser : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectContextPreferencesUser, xiiPreferences);

public:
  xiiGameObjectContextPreferencesUser();

  xiiUuid GetContextDocument() const;
  void    SetContextDocument(xiiUuid val);
  xiiUuid GetContextObject() const;
  void    SetContextObject(xiiUuid val);

protected:
  xiiUuid m_ContextDocument;
  xiiUuid m_ContextObject;
};
