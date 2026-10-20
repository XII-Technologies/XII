/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class XII_EDITORFRAMEWORK_DLL xiiCppSettings
{
public:
  /// Names are used as C++ identifiers, CMake targets, and directory names.
  static bool IsValidPluginName(xiiStringView sName);

  xiiResult Save(xiiStringView sFile = ":project/Editor/CppProject.ddl");
  xiiResult Load(xiiStringView sFile = ":project/Editor/CppProject.ddl");

  xiiString m_sPluginName;
};
