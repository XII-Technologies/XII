#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class XII_EDITORFRAMEWORK_DLL xiiCppSettings
{
public:
  xiiResult Save(xiiStringView sFile = ":project/Editor/CppProject.ddl");
  xiiResult Load(xiiStringView sFile = ":project/Editor/CppProject.ddl");

  enum class Compiler
  {
    None,
    Vs2022,
  };

  xiiString         m_sPluginName;
  Compiler          m_Compiler = Compiler::None;
  mutable xiiString m_sMsBuildPath;
};
