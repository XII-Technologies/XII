#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiConsoleFunctionBase);

xiiQuakeConsole::xiiQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled                = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings              = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(XII_DEFAULT_NEW(xiiCommandInterpreterLua));
#endif
}

xiiQuakeConsole::~xiiQuakeConsole()
{
  EnableLogOutput(false);
}

void xiiQuakeConsole::AddConsoleString(xiiStringView sText, xiiConsoleString::Type type)
{
  XII_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  xiiConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText           = sText;
  cs.m_Type            = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
  {
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);
  }

  xiiConsole::AddConsoleString(sText, type);
}

const xiiDeque<xiiConsoleString>& xiiQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }
  return m_ConsoleStrings;
}

void xiiQuakeConsole::LogHandler(const xiiLoggingEventData& data)
{
  xiiConsoleString::Type type = xiiConsoleString::Type::Default;

  switch (data.m_EventType)
  {
    case xiiLogMsgType::GlobalDefault:
    case xiiLogMsgType::Flush:
    case xiiLogMsgType::BeginGroup:
    case xiiLogMsgType::EndGroup:
    case xiiLogMsgType::None:
    case xiiLogMsgType::ENUM_COUNT:
    case xiiLogMsgType::All:
      return;

    case xiiLogMsgType::ErrorMsg:
      type = xiiConsoleString::Type::Error;
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      type = xiiConsoleString::Type::SeriousWarning;
      break;

    case xiiLogMsgType::WarningMsg:
      type = xiiConsoleString::Type::Warning;
      break;

    case xiiLogMsgType::SuccessMsg:
      type = xiiConsoleString::Type::Success;
      break;

    case xiiLogMsgType::InfoMsg:
      break;

    case xiiLogMsgType::DevMsg:
      type = xiiConsoleString::Type::Dev;
      break;

    case xiiLogMsgType::DebugMsg:
      type = xiiConsoleString::Type::Debug;
      break;
  }

  xiiStringBuilder sFormat;
  sFormat.SetPrintf("%*s", data.m_uiIndentation, "");
  sFormat.Append(data.m_sText);

  AddConsoleString(sFormat.GetData(), type);
}

void xiiQuakeConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    xiiStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
}

void xiiQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    xiiGlobalLog::AddLogWriter(xiiMakeDelegate(&xiiQuakeConsole::LogHandler, this));
  }
  else
  {
    xiiGlobalLog::RemoveLogWriter(xiiMakeDelegate(&xiiQuakeConsole::LogHandler, this));
  }
}

void xiiQuakeConsole::SaveState(xiiStreamWriter& ref_stream) const
{
  XII_LOCK(m_Mutex);

  const xiiUInt8 uiVersion = 1;
  ref_stream << uiVersion;

  ref_stream << m_InputHistory.GetCount();
  for (xiiUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    ref_stream << m_InputHistory[i];
  }

  ref_stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    ref_stream << it.Key();
    ref_stream << it.Value();
  }
}

void xiiQuakeConsole::LoadState(xiiStreamReader& ref_stream)
{
  XII_LOCK(m_Mutex);

  xiiUInt8 uiVersion = 0;
  ref_stream >> uiVersion;

  if (uiVersion == 1)
  {
    xiiUInt32 count = 0;
    ref_stream >> count;
    m_InputHistory.SetCount(count);

    for (xiiUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      ref_stream >> m_InputHistory[i];
    }

    ref_stream >> count;

    xiiString sKey;
    xiiString sValue;

    for (xiiUInt32 i = 0; i < count; ++i)
    {
      ref_stream >> sKey;
      ref_stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}

void xiiCommandInterpreterState::AddOutputLine(const xiiFormatString& text, xiiConsoleString::Type type /*= xiiCommandOutputLine::Type::Default*/)
{
  auto& line  = m_sOutput.ExpandAndGetRef();
  line.m_Type = type;

  xiiStringBuilder tmp;
  line.m_sText = text.GetText(tmp);
}

xiiColor xiiConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case xiiConsoleString::Type::Default:
      return xiiColor::White;

    case xiiConsoleString::Type::Error:
      return xiiColor(1.0f, 0.2f, 0.2f);

    case xiiConsoleString::Type::SeriousWarning:
      return xiiColor(1.0f, 0.4f, 0.1f);

    case xiiConsoleString::Type::Warning:
      return xiiColor(1.0f, 0.6f, 0.1f);

    case xiiConsoleString::Type::Note:
      return xiiColor(1, 200.0f / 255.0f, 0);

    case xiiConsoleString::Type::Success:
      return xiiColor(0.1f, 1.0f, 0.1f);

    case xiiConsoleString::Type::Executed:
      return xiiColor(1.0f, 0.5f, 0.0f);

    case xiiConsoleString::Type::VarName:
      return xiiColorGammaUB(255, 210, 0);

    case xiiConsoleString::Type::FuncName:
      return xiiColorGammaUB(100, 255, 100);

    case xiiConsoleString::Type::Dev:
      return xiiColor(0.6f, 0.6f, 0.6f);

    case xiiConsoleString::Type::Debug:
      return xiiColor(0.4f, 0.6f, 0.8f);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiColor::White;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiConsole::xiiConsole() = default;

xiiConsole::~xiiConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void xiiConsole::SetMainConsole(xiiConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

xiiConsole* xiiConsole::GetMainConsole()
{
  return s_pMainConsole;
}

xiiConsole* xiiConsole::s_pMainConsole = nullptr;

bool xiiConsole::AutoComplete(xiiStringBuilder& ref_sText)
{
  XII_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    xiiCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void xiiConsole::ExecuteCommand(xiiStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  XII_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    xiiCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void xiiConsole::AddConsoleString(xiiStringView sText, xiiConsoleString::Type type /*= xiiConsoleString::Type::Default*/)
{
  xiiConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type  = type;

  // Broadcast that we have added a string to the console
  xiiConsoleEvent e;
  e.m_Type                = xiiConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void xiiConsole::AddToInputHistory(xiiStringView sText)
{
  XII_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (xiiInt32 i = 0; i < (xiiInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (xiiInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(xiiMath::Min<xiiUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (xiiUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void xiiConsole::RetrieveInputHistory(xiiInt32 iHistoryUp, xiiStringBuilder& ref_sResult)
{
  XII_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = xiiMath::Clamp<xiiInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

xiiResult xiiConsole::SaveInputHistory(xiiStringView sFile)
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiStringBuilder str;

  for (const xiiString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    XII_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return XII_SUCCESS;
}

void xiiConsole::LoadInputHistory(xiiStringView sFile)
{
  xiiFileReader file;
  if (file.Open(sFile).Failed())
    return;

  xiiStringBuilder str;
  str.ReadAll(file);

  xiiHybridArray<xiiStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (xiiUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}

XII_STATICLINK_FILE(Core, Core_Console_Implementation_Console);
