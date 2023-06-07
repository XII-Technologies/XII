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

void xiiQuakeConsole::BindKey(const char* szKey, const char* szCommand)
{
  xiiStringBuilder s;
  s.Format("Binding key '{0}' to command '{1}'", szKey, szCommand);
  AddConsoleString(s, xiiConsoleString::Type::Success);

  m_BoundKeys[szKey] = szCommand;
}

void xiiQuakeConsole::UnbindKey(const char* szKey)
{
  xiiStringBuilder s;
  s.Format("Unbinding key '{0}'", szKey);
  AddConsoleString(s, xiiConsoleString::Type::Success);

  m_BoundKeys.Remove(szKey);
}

void xiiQuakeConsole::ExecuteBoundKey(const char* szKey)
{
  auto it = m_BoundKeys.Find(szKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value().GetData());
  }
}



XII_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
