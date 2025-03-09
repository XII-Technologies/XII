#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

namespace xiiStackTraceLogParser
{
  XII_EDITORFRAMEWORK_DLL bool ParseStackTraceFileNameAndLineNumber(const xiiStringView& sLine, xiiStringView& ref_sFileName, xiiInt32& ref_iLineNumber); // [tested]
  XII_EDITORFRAMEWORK_DLL bool ParseAssertFileNameAndLineNumber(const xiiStringView& sLine, xiiStringView& ref_sFileName, xiiInt32& ref_iLineNumber);     // [tested]
  void                         Register();
  void                         Unregister();
} // namespace xiiStackTraceLogParser
