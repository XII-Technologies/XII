/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Settings/EditorSettings.h>

class xiiGameObjectContextPreferencesUser : public xiiEditorSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectContextPreferencesUser, xiiEditorSettings);

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
