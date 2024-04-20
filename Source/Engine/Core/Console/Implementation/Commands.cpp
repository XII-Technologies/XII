#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Foundation/Configuration/CVar.h>

void xiiQuakeConsole::ExecuteCommand(xiiStringView sInput)
{
  const bool bBind   = sInput.StartsWith_NoCase("bind ");
  const bool bUnbind = sInput.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    xiiStringBuilder tmp;
    const char*      szAfterCmd = xiiStringUtils::FindWordEnd(sInput.GetData(tmp), xiiStringUtils::IsWhiteSpace); // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = xiiStringUtils::SkipCharacters(szAfterCmd, xiiStringUtils::IsWhiteSpace);                  // go to the next word
    const char* szKeyNameEnd   = xiiStringUtils::FindWordEnd(szKeyNameStart, xiiStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    xiiStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey; // copy the word into a zero terminated string

    const char* szCommandToBind = xiiStringUtils::SkipCharacters(szKeyNameEnd, xiiStringUtils::IsWhiteSpace);

    if (bUnbind || xiiStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  xiiConsole::ExecuteCommand(sInput);
}

void xiiQuakeConsole::BindKey(xiiStringView sKey, xiiStringView sCommand)
{
  xiiStringBuilder s;
  s.SetFormat("Binding key '{0}' to command '{1}'", sKey, sCommand);
  AddConsoleString(s, xiiConsoleString::Type::Success);

  m_BoundKeys[sKey] = sCommand;
}

void xiiQuakeConsole::UnbindKey(xiiStringView sKey)
{
  xiiStringBuilder s;
  s.SetFormat("Unbinding key '{0}'", sKey);
  AddConsoleString(s, xiiConsoleString::Type::Success);

  m_BoundKeys.Remove(sKey);
}

void xiiQuakeConsole::ExecuteBoundKey(xiiStringView sKey)
{
  auto it = m_BoundKeys.Find(sKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value());
  }
}

XII_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
