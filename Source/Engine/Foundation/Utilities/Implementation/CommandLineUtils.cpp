#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <shellapi.h>
#endif

static xiiCommandLineUtils g_pCmdLineInstance;

xiiCommandLineUtils* xiiCommandLineUtils::GetGlobalInstance()
{
  return &g_pCmdLineInstance;
}

void xiiCommandLineUtils::SplitCommandLineString(xiiStringView sCommandString, bool bAddExecutableDir, xiiDynamicArray<xiiString>& out_args, xiiDynamicArray<const char*>& out_argsV)
{
  // Add application dir as first argument as customary on other platforms.
  if (bAddExecutableDir)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    wchar_t moduleFilename[256];
    GetModuleFileNameW(nullptr, moduleFilename, 256);
    out_args.PushBack(xiiStringUtf8(moduleFilename).GetData());
#else
    XII_ASSERT_NOT_IMPLEMENTED;
#endif
  }

  // Simple args splitting. Not as powerful as Win32's CommandLineToArgvW.
  const char* currentChar = sCommandString.GetStartPointer();
  const char* lastEnd     = currentChar;
  bool        inQuotes    = false;
  while (*currentChar != '\0')
  {
    if (*currentChar == '\"')
      inQuotes = !inQuotes;
    else if (*currentChar == ' ' && !inQuotes)
    {
      xiiStringBuilder path = xiiStringView(lastEnd, currentChar);
      path.Trim(" \"");
      out_args.PushBack(path);
      lastEnd = currentChar + 1;
    }
    xiiUnicodeUtils::MoveToNextUtf8(currentChar);
  }

  out_argsV.Reserve(out_argsV.GetCount());
  for (xiiString& str : out_args)
    out_argsV.PushBack(str.GetData());
}

void xiiCommandLineUtils::SetCommandLine(xiiUInt32 uiArgc, const char** pArgv, ArgMode mode /*= UseArgcArgv*/)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  if (mode == ArgMode::PreferOsArgs)
  {
    SetCommandLine();
    return;
  }
#endif

  m_Commands.Clear();
  m_Commands.Reserve(uiArgc);

  for (xiiUInt32 i = 0; i < uiArgc; ++i)
    m_Commands.PushBack(pArgv[i]);
}

void xiiCommandLineUtils::SetCommandLine(xiiArrayPtr<xiiString> commands)
{
  m_Commands = commands;
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

void xiiCommandLineUtils::SetCommandLine()
{
  int argc = 0;

  LPWSTR* argvw = CommandLineToArgvW(::GetCommandLineW(), &argc);

  XII_ASSERT_RELEASE(argvw != nullptr, "CommandLineToArgvW failed");

  xiiArrayPtr<xiiStringUtf8> ArgvUtf8 = XII_DEFAULT_NEW_ARRAY(xiiStringUtf8, argc);
  xiiArrayPtr<const char*>   argv     = XII_DEFAULT_NEW_ARRAY(const char*, argc);

  for (xiiInt32 i = 0; i < argc; ++i)
  {
    ArgvUtf8[i] = argvw[i];
    argv[i]     = ArgvUtf8[i].GetData();
  }

  SetCommandLine(argc, argv.GetPtr(), ArgMode::UseArgcArgv);


  XII_DEFAULT_DELETE_ARRAY(ArgvUtf8);
  XII_DEFAULT_DELETE_ARRAY(argv);
  LocalFree(argvw);
}

#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
// Not implemented on Windows UWP.
#elif XII_ENABLED(XII_PLATFORM_OSX)
// Not implemented on OSX.
#elif XII_ENABLED(XII_PLATFORM_LINUX)
// Not implemented on Linux.
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
// Not implemented on Android.
#else
#  error "xiiCommandLineUtils::SetCommandLine(): Abstraction missing."
#endif

const xiiDynamicArray<xiiString>& xiiCommandLineUtils::GetCommandLineArray() const
{
  return m_Commands;
}

xiiString xiiCommandLineUtils::GetCommandLineString() const
{
  xiiStringBuilder commandLine;
  for (const xiiString& command : m_Commands)
  {
    if (commandLine.IsEmpty())
    {
      commandLine.Append(command.GetView());
    }
    else
    {
      commandLine.Append(" ", command);
    }
  }
  return commandLine;
}

xiiUInt32 xiiCommandLineUtils::GetParameterCount() const
{
  return m_Commands.GetCount();
}

const xiiString& xiiCommandLineUtils::GetParameter(xiiUInt32 uiParam) const
{
  return m_Commands[uiParam];
}

xiiInt32 xiiCommandLineUtils::GetOptionIndex(xiiStringView sOption, bool bCaseSensitive) const
{
  XII_ASSERT_DEV(sOption.StartsWith("-"), "All command line option names must start with a hyphen (e.g. -file)");

  for (xiiUInt32 i = 0; i < m_Commands.GetCount(); ++i)
  {
    if ((bCaseSensitive && m_Commands[i].IsEqual(sOption)) || (!bCaseSensitive && m_Commands[i].IsEqual_NoCase(sOption)))
      return i;
  }

  return -1;
}

bool xiiCommandLineUtils::HasOption(xiiStringView sOption, bool bCaseSensitive /*= false*/) const
{
  return GetOptionIndex(sOption, bCaseSensitive) >= 0;
}

xiiUInt32 xiiCommandLineUtils::GetStringOptionArguments(xiiStringView sOption, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return 0;

  xiiUInt32 uiParamCount = 0;

  for (xiiUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> no parameters
      break;

    ++uiParamCount;
  }

  return uiParamCount;
}

xiiStringView xiiCommandLineUtils::GetStringOption(xiiStringView sOption, xiiUInt32 uiArgument, xiiStringView sDefault, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return sDefault;

  xiiUInt32 uiParamCount = 0;

  for (xiiUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> not enough parameters
      return sDefault;

    // found the right one, return it
    if (uiParamCount == uiArgument)
      return m_Commands[uiParam].GetData();

    ++uiParamCount;
  }

  return sDefault;
}

const xiiString xiiCommandLineUtils::GetAbsolutePathOption(xiiStringView sOption, xiiUInt32 uiArgument /*= 0*/, xiiStringView sDefault /*= ""*/, bool bCaseSensitive /*= false*/) const
{
  xiiStringView sPath = GetStringOption(sOption, uiArgument, sDefault, bCaseSensitive);

  if (sPath.IsEmpty())
    return sPath;

  return xiiOSFile::MakePathAbsoluteWithCWD(sPath);
}

bool xiiCommandLineUtils::GetBoolOption(xiiStringView sOption, bool bDefault, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return bDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command, treat this as 'on'
    return true;

  if (m_Commands[iIndex + 1].StartsWith("-")) // next command is the next option -> treat this as 'on' as well
    return true;

  // otherwise try to convert the next option to a boolean
  bool bRes = bDefault;
  xiiConversionUtils::StringToBool(m_Commands[iIndex + 1].GetData(), bRes).IgnoreResult();

  return bRes;
}

xiiInt32 xiiCommandLineUtils::GetIntOption(xiiStringView sOption, xiiInt32 iDefault, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return iDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return iDefault;

  // try to convert the next option to a number
  xiiInt32 iRes = iDefault;
  xiiConversionUtils::StringToInt(m_Commands[iIndex + 1].GetData(), iRes).IgnoreResult();

  return iRes;
}

xiiUInt32 xiiCommandLineUtils::GetUIntOption(xiiStringView sOption, xiiUInt32 uiDefault, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return uiDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return uiDefault;

  // try to convert the next option to a number
  xiiUInt32 uiRes = uiDefault;
  xiiConversionUtils::StringToUInt(m_Commands[iIndex + 1].GetData(), uiRes).IgnoreResult();

  return uiRes;
}

double xiiCommandLineUtils::GetFloatOption(xiiStringView sOption, double fDefault, bool bCaseSensitive) const
{
  const xiiInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return fDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return fDefault;

  // try to convert the next option to a number
  double fRes = fDefault;
  xiiConversionUtils::StringToFloat(m_Commands[iIndex + 1].GetData(), fRes).IgnoreResult();

  return fRes;
}

void xiiCommandLineUtils::InjectCustomArgument(xiiStringView sArgument)
{
  m_Commands.PushBack(sArgument);
}

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_CommandLineUtils);
