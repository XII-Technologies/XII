/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>

namespace xiiStackTraceLogParser
{
  static void StackTraceLogCallback(const xiiStringView& sLogText)
  {
    xiiStringView sFileName;
    xiiInt32      lineNumber;

    if (!ParseStackTraceFileNameAndLineNumber(sLogText, sFileName, lineNumber))
    {
      if (!ParseAssertFileNameAndLineNumber(sLogText, sFileName, lineNumber))
      {
        return;
      }
    }

    xiiCppSettings cpp;
    cpp.Load().IgnoreResult();
    const xiiStatus res = xiiCppProject::OpenInCodeEditor(sFileName, lineNumber);
  }

  bool ParseAssertFileNameAndLineNumber(const xiiStringView& sLine, xiiStringView& ref_sFileName, xiiInt32& ref_iLineNumber)
  {
    const char*     szFileMarker     = "File: ";
    const xiiUInt32 fileMarkerLength = xiiStringUtils::GetStringElementCount(szFileMarker);
    const char*     szLineMarker     = "Line: ";
    const xiiUInt32 lineMarkerLength = xiiStringUtils::GetStringElementCount(szLineMarker);

    if (sLine.FindSubString("*** Assertion ***") == nullptr)
    {
      return false;
    }

    const char* szFile = sLine.FindSubString(szFileMarker);
    if (szFile == nullptr)
    {
      return false;
    }

    const char* szLine = sLine.FindSubString(szLineMarker);
    if (szLine == nullptr)
    {
      return false;
    }

    const char* szFileEnd = xiiStringUtils::FindSubString(szFile + fileMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szFileEnd == nullptr)
    {
      return false;
    }

    const char* szLineEnd = xiiStringUtils::FindSubString(szLine + lineMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szLineEnd == nullptr)
    {
      return false;
    }

    ref_sFileName = xiiStringView(szFile + fileMarkerLength + 1, szFileEnd);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const xiiResult res = xiiConversionUtils::StringToInt(xiiStringView(szLine + lineMarkerLength + 1, szLineEnd), ref_iLineNumber);
    if (res != XII_SUCCESS)
    {
      return false;
    }

    return true;
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  bool ParseStackTraceFileNameAndLineNumber(const xiiStringView& sLine, xiiStringView& ref_sFileName, xiiInt32& ref_iLineNumber)
  {
    const char* szEndFileNameMarker   = "(";
    const char* szEndLineNumberMarker = "):";

    const char* szEndLineNumber = sLine.FindSubString(szEndLineNumberMarker);
    if (szEndLineNumber == nullptr)
    {
      return false;
    }

    const char* szEndFileName = sLine.FindLastSubString(szEndFileNameMarker, szEndLineNumber);
    if (szEndFileName == nullptr)
    {
      return false;
    }

    ref_sFileName = xiiStringView(sLine.GetStartPointer(), szEndFileName);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const xiiResult res = xiiConversionUtils::StringToInt(xiiStringView(szEndFileName + 1, szEndLineNumber), ref_iLineNumber);
    if (res != XII_SUCCESS)
    {
      return false;
    }

    return true;
  }
#else
  bool ParseStackTraceFileNameAndLineNumber(const xiiStringView& sLine, xiiStringView& ref_sFileName, xiiInt32& ref_iLineNumber)
  {
    return false;
  }
#endif

  void Register()
  {
    xiiQtLogWidget::AddLogItemContextActionCallback("StackTraceLog", &StackTraceLogCallback);
  }

  void Unregister()
  {
    xiiQtLogWidget::RemoveLogItemContextActionCallback("StackTraceLog");
  }
} // namespace xiiStackTraceLogParser
