#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
// Include inline file
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/System/Implementation/Win/Process_win.h>
#  elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID) || XII_ENABLED(XII_PLATFORM_OSX)
#    include <Foundation/System/Implementation/Posix/Process_posix.h>
#  else
#    error "Process functions are not implemented on current platform"
#  endif

#  include <Foundation/Strings/Implementation/StringIterator.h>

xiiProcess::xiiProcess(xiiProcess&& rhs) = default;

void xiiProcessOptions::AddArgument(const xiiFormatString& arg)
{
  xiiStringBuilder formatted, tmp;
  formatted = arg.GetText(tmp);
  formatted.Trim(" \t\n");

  m_Arguments.PushBack(formatted);
}

void xiiProcessOptions::AddCommandLine(const char* szCmdLine)
{
  xiiStringBuilder curArg;

  xiiStringView cmdView(szCmdLine);

  bool isInString = false;

  for (auto it = cmdView.GetIteratorFront(); it.IsValid(); ++it)
  {
    bool commit      = false;
    bool commitEmpty = false;
    bool append      = true;

    if (it.GetCharacter() == '\"')
    {
      append = false;

      if (isInString)
      {
        commitEmpty = true; // push-back even empty strings (they are there for a purpose)
      }
      else
      {
        commit = true; // only commit non-empty stuff that is not a string argument
      }

      isInString = !isInString;
    }
    else if (it.GetCharacter() == ' ')
    {
      if (!isInString)
      {
        commit = true;
        append = false;
      }
    }

    if (commitEmpty || (commit && !curArg.IsEmpty()))
    {
      m_Arguments.PushBack(curArg);
      curArg.Clear();
    }

    if (append)
    {
      curArg.Append(it.GetCharacter());
    }
  }

  if (!curArg.IsEmpty())
  {
    m_Arguments.PushBack(curArg);
    curArg.Clear();
  }
}

xiiInt32 xiiProcess::GetExitCode() const
{
  if (m_iExitCode == -0xFFFF)
  {
    // this may update m_iExitCode, if the state has switched to 'finished'
    GetState();
  }

  return m_iExitCode;
}

void xiiProcessOptions::BuildCommandLineString(xiiStringBuilder& cmd) const
{
  for (const auto& arg0 : m_Arguments)
  {
    xiiStringView arg = arg0;

    while (arg.StartsWith("\""))
      arg.ChopAwayFirstCharacterAscii();

    while (arg.EndsWith("\""))
      arg.Shrink(0, 1);

    // also wrap empty arguments in quotes, otherwise they would get lost
    if (arg.IsEmpty() || arg.FindSubString(" ") != nullptr || arg.FindSubString("\t") != nullptr || arg.FindSubString("\n") != nullptr)
    {
      cmd.Append(" \"");
      cmd.Append(arg);
      cmd.Append("\"");
    }
    else
    {
      cmd.Append(" ");
      cmd.Append(arg);
    }
  }

  cmd.Trim(" ");
}

void xiiProcess::BuildFullCommandLineString(const xiiProcessOptions& opt, const char* szProcess, xiiStringBuilder& cmd) const
{
  // have to set the full path to the process as the very first argument
  cmd.Set("\"", szProcess, "\"");

  opt.BuildCommandLineString(cmd);
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_Process);
