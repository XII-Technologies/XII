/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/StackTracer.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

#include <DbgHelp.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

struct Module
{
  xiiString m_sFilePath;
  xiiUInt64 m_uiBaseAddress;
  xiiUInt32 m_uiSize;
};

struct Stackframe
{
  xiiUInt32 m_uiModuleIndex = 0xFFFFFFFF;
  xiiUInt32 m_uiLineNumber  = 0;
  xiiString m_sFilename;
  xiiString m_sSymbol;
};

xiiCommandLineOptionString opt_ModuleList("_app", "-ModuleList", "List of modules as a string in this format:\n\n\
File1Path?File1BaseAddressHEX?File1Size|File2Path?File2BaseAddressHEX?File2Size|...\n\n\
For example:\n\
  $[A]/app.exe?7FF7E5540000?106496|$[S]/System32/KERNELBASE.dll?7FFE2B780000?2920448\n\n\
  $[A] represents the application directory and will be adjusted as necessary.\n\
  $[S] represents the system root directory and will be adjusted as necessary.",
                                          "");
xiiCommandLineOptionString opt_Callstack("_app", "-Callstack", "Callstack in this format:\n\n7FFE2DD6CE74|7FFE2B7AAA86|7FFE034C22D1", "");

xiiCommandLineOptionEnum opt_OutputFormat("_app", "-Format", "How to output the resolved callstack.", "Text=0|JSON=1", 0);

xiiCommandLineOptionPath opt_OutputFile("_app", "-File", "The target file where to write the output to.\nIf left empty, the output is printed to the console.", "");

class xiiStackResolver : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiStackResolver() :
    xiiApplication("xiiStackResolver")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual xiiApplication::Execution Run() override;

  xiiResult LoadModules();
  xiiResult ParseModules();
  xiiResult ParseCallstack();

  void ResolveStackFrames();
  void FormatAsText(xiiStringBuilder& ref_sOutput);
  void FormatAsJSON(xiiStringBuilder& ref_sOutput);

  HANDLE                      m_hProcess;
  xiiDynamicArray<Module>     m_Modules;
  xiiDynamicArray<xiiUInt64>  m_Callstack;
  xiiDynamicArray<Stackframe> m_Stackframes;
  xiiStringBuilder            m_SystemRootDir;
  xiiStringBuilder            m_ApplicationDir;
};

void xiiStackResolver::AfterCoreSystemsStartup()
{
  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  m_Modules.Reserve(128);
  m_Callstack.Reserve(128);
  m_Stackframes.Reserve(128);
}

void xiiStackResolver::BeforeCoreSystemsShutdown()
{
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
}

xiiResult xiiStackResolver::ParseModules()
{
  const xiiStringBuilder sModules = opt_ModuleList.GetOptionValue(xiiCommandLineOption::LogMode::Never);

  xiiDynamicArray<xiiStringView> parts;
  sModules.Split(false, parts, "|");

  for (xiiStringView sModView : parts)
  {
    xiiStringBuilder               sMod = sModView;
    xiiDynamicArray<xiiStringView> parts2;
    sMod.Split(false, parts2, "?");

    xiiUInt64 base;
    if (xiiConversionUtils::ConvertHexStringToUInt64(parts2[1], base).Failed())
    {
      xiiLog::Error("Failed to convert HEX string '{}' to UINT64", parts2[1]);
      return XII_FAILURE;
    }

    xiiStringBuilder sSize = parts2[2];
    xiiUInt32        size;
    if (xiiConversionUtils::StringToUInt(sSize, size).Failed())
    {
      xiiLog::Error("Failed to convert string '{}' to UINT32", sSize);
      return XII_FAILURE;
    }

    xiiStringBuilder sModuleName = parts2[0];
    sModuleName.ReplaceFirst_NoCase("$[S]", m_SystemRootDir);
    sModuleName.ReplaceFirst_NoCase("$[A]", m_ApplicationDir);
    sModuleName.MakeCleanPath();

    auto& mod           = m_Modules.ExpandAndGetRef();
    mod.m_sFilePath     = sModuleName;
    mod.m_uiBaseAddress = base;
    mod.m_uiSize        = size;
  }

  return XII_SUCCESS;
}

xiiResult xiiStackResolver::ParseCallstack()
{
  xiiStringBuilder sCallstack = opt_Callstack.GetOptionValue(xiiCommandLineOption::LogMode::Never);

  xiiDynamicArray<xiiStringView> parts;
  sCallstack.Split(false, parts, "|");
  for (xiiStringView sModView : parts)
  {
    xiiUInt64 base;
    if (xiiConversionUtils::ConvertHexStringToUInt64(sModView, base).Failed())
    {
      xiiLog::Error("Failed to convert HEX string '{}' to UINT64", sModView);
      return XII_FAILURE;
    }

    m_Callstack.PushBack(base);
  }

  return XII_SUCCESS;
}

xiiResult xiiStackResolver::LoadModules()
{
  if (SymInitialize(m_hProcess, nullptr, FALSE) != TRUE) // TODO specify PDB search path as second parameter?
  {
    xiiLog::Error("SymInitialize failed");
    return XII_FAILURE;
  }

  for (const auto& curModule : m_Modules)
  {
    if (SymLoadModuleExW(m_hProcess, nullptr, xiiStringWChar(curModule.m_sFilePath), nullptr, curModule.m_uiBaseAddress, curModule.m_uiSize, nullptr, 0) == 0)
    {
      xiiLog::Warning("Couldn't load module '{}'", curModule.m_sFilePath);
    }
    else
    {
      xiiLog::Success("Loaded module '{}'", curModule.m_sFilePath);
    }
  }

  return XII_SUCCESS;
}

void xiiStackResolver::ResolveStackFrames()
{
  xiiStringBuilder tmp;

  char buffer[1024];
  for (xiiUInt32 i = 0; i < m_Callstack.GetCount(); i++)
  {
    DWORD64 symbolAddress = m_Callstack[i];

    _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
    xiiMemoryUtils::ZeroFill(&symbolInfo, 1);
    symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
    symbolInfo.MaxNameLen   = (XII_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

    DWORD64 displacement = 0;
    BOOL    result       = SymFromAddrW(m_hProcess, symbolAddress, &displacement, &symbolInfo);
    if (!result)
    {
      wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
    }

    IMAGEHLP_LINEW64 lineInfo;
    DWORD            displacement2 = static_cast<DWORD>(displacement);
    xiiMemoryUtils::ZeroFill(&lineInfo, 1);
    lineInfo.SizeOfStruct = sizeof(lineInfo);
    SymGetLineFromAddrW64(m_hProcess, symbolAddress, &displacement2, &lineInfo);

    auto& frame = m_Stackframes.ExpandAndGetRef();

    for (xiiUInt32 modIndex = 0; modIndex < m_Modules.GetCount(); modIndex++)
    {
      if (m_Modules[modIndex].m_uiBaseAddress == (xiiUInt64)symbolInfo.ModBase)
      {
        frame.m_uiModuleIndex = modIndex;
        break;
      }
    }

    frame.m_uiLineNumber = (xiiUInt32)lineInfo.LineNumber;
    frame.m_sSymbol      = xiiStringUtf8(symbolInfo.Name).GetView();

    tmp = xiiStringUtf8(lineInfo.FileName).GetView();
    tmp.MakeCleanPath();
    frame.m_sFilename = tmp;
  }
}

void xiiStackResolver::FormatAsText(xiiStringBuilder& ref_sOutput)
{
  xiiLog::Info("Formatting callstack as text.");

  for (const auto& frame : m_Stackframes)
  {
    const char* szModuleName = "<unknown module>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      szModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    const char* szFileName = "<unknown file>";
    if (!frame.m_sFilename.IsEmpty())
    {
      szFileName = frame.m_sFilename;
    }

    const char* szSymbol = "<unknown symbol>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      szSymbol = frame.m_sSymbol;
    }

    ref_sOutput.AppendFormat("[][{}] {}({}): '{}'\n", szModuleName, szFileName, frame.m_uiLineNumber, szSymbol);
  }
}

void xiiStackResolver::FormatAsJSON(xiiStringBuilder& ref_sOutput)
{
  xiiLog::Info("Formatting callstack as JSON.");

  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);

  xiiStandardJSONWriter json;
  json.SetOutputStream(&writer);
  json.SetWhitespaceMode(xiiJSONWriter::WhitespaceMode::LessIndentation);

  json.BeginObject();
  json.BeginArray("Stackframes");

  for (const auto& frame : m_Stackframes)
  {
    const char* szModuleName = "<unknown>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      szModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    const char* szFileName = "<unknown>";
    if (!frame.m_sFilename.IsEmpty())
    {
      szFileName = frame.m_sFilename;
    }

    const char* szSymbol = "<unknown>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      szSymbol = frame.m_sSymbol;
    }

    json.BeginObject();
    json.AddVariableString("Module", szModuleName);
    json.AddVariableString("File", szFileName);
    json.AddVariableUInt32("Line", frame.m_uiLineNumber);
    json.AddVariableString("Symbol", szSymbol);
    json.EndObject();
  }

  json.EndArray();
  json.EndObject();

  xiiStringView text((const char*)storage.GetData(), storage.GetStorageSize32());

  ref_sOutput.Append(text);
}

xiiApplication::Execution xiiStackResolver::Run()
{
  if (xiiCommandLineOption::LogAvailableOptions(xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_app"))
    return Execution::Quit;

  xiiString sMissingOpt;
  if (xiiCommandLineOption::RequireOptions("-ModuleList;-Callstack", &sMissingOpt).Failed())
  {
    xiiLog::Error("Command line option '{}' was not specified.", sMissingOpt);

    xiiCommandLineOption::LogAvailableOptions(xiiCommandLineOption::LogAvailableModes::Always, "_app");
    return Execution::Quit;
  }

  m_hProcess = GetCurrentProcess();

  m_ApplicationDir = xiiOSFile::GetApplicationDirectory();
  m_ApplicationDir.MakeCleanPath();
  m_ApplicationDir.Trim("", "/");

  m_SystemRootDir = xiiEnvironmentVariableUtils::GetValueString("SystemRoot");
  m_SystemRootDir.MakeCleanPath();
  m_SystemRootDir.Trim("", "/");

  if (ParseModules().Failed())
    return Execution::Quit;

  if (ParseCallstack().Failed())
    return Execution::Quit;

  if (LoadModules().Failed())
    return Execution::Quit;

  ResolveStackFrames();

  xiiStringBuilder output;

  if (opt_OutputFormat.GetOptionValue(xiiCommandLineOption::LogMode::Never) == 0)
  {
    FormatAsText(output);
  }
  else if (opt_OutputFormat.GetOptionValue(xiiCommandLineOption::LogMode::Never) == 1)
  {
    FormatAsJSON(output);
  }

  if (opt_OutputFile.IsOptionSpecified())
  {
    xiiLog::Info("Writing output to '{}'.", opt_OutputFile.GetOptionValue(xiiCommandLineOption::LogMode::Never));

    xiiOSFile file;
    if (file.Open(opt_OutputFile.GetOptionValue(xiiCommandLineOption::LogMode::Never), xiiFileOpenMode::Write).Failed())
    {
      xiiLog::Error("Could not open file for writing: '{}'", opt_OutputFile.GetOptionValue(xiiCommandLineOption::LogMode::Never));
      return Execution::Quit;
    }

    file.Write(output.GetData(), output.GetElementCount()).IgnoreResult();
  }
  else
  {
    xiiLog::Info("Writing output to console.");

    XII_LOG_BLOCK("Resolved callstack");

    xiiDynamicArray<xiiStringView> lines;
    output.Split(true, lines, "\n");

    for (auto l : lines)
    {
      xiiLog::Info("{}", l);
    }
  }

  return Execution::Quit;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiStackResolver);
