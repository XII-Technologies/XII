#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class XII_EDITORFRAMEWORK_DLL xiiCppSettings
{
public:
  xiiResult Save(xiiStringView sFile = ":project/Editor/CppProject.ddl");
  xiiResult Load(xiiStringView sFile = ":project/Editor/CppProject.ddl");

  xiiString m_sPluginName;
};
