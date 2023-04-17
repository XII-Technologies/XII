#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Core/Console/QuakeConsole.h>

void xiiCommandInterpreter::FindPossibleCVars(xiiStringView sVariable, xiiDeque<xiiString>& AutoCompleteOptions, xiiDeque<xiiConsoleString>& AutoCompleteDescriptions)
{
  xiiStringBuilder sText;

  xiiCVar* pCVar = xiiCVar::GetFirstInstance();
  while (pCVar)
  {
    if (pCVar->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} = {1}", pCVar->GetName(), xiiQuakeConsole::GetFullInfoAsString(pCVar));

      xiiConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type  = xiiConsoleString::Type::VarName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void xiiCommandInterpreter::FindPossibleFunctions(xiiStringView sVariable, xiiDeque<xiiString>& AutoCompleteOptions, xiiDeque<xiiConsoleString>& AutoCompleteDescriptions)
{
  xiiStringBuilder sText;

  xiiConsoleFunctionBase* pFunc = xiiConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (pFunc->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      xiiConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type  = xiiConsoleString::Type::FuncName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pFunc->GetName());
    }

    pFunc = pFunc->GetNextInstance();
  }
}


const xiiString xiiQuakeConsole::GetValueAsString(xiiCVar* pCVar)
{
  xiiStringBuilder s = "undefined";

  switch (pCVar->GetType())
  {
    case xiiCVarType::Int:
    {
      xiiCVarInt* pInt = static_cast<xiiCVarInt*>(pCVar);
      s.Format("{0}", pInt->GetValue());
    }
    break;

    case xiiCVarType::Bool:
    {
      xiiCVarBool* pBool = static_cast<xiiCVarBool*>(pCVar);
      if (pBool->GetValue() == true)
        s = "true";
      else
        s = "false";
    }
    break;

    case xiiCVarType::String:
    {
      xiiCVarString* pString = static_cast<xiiCVarString*>(pCVar);
      s.Format("\"{0}\"", pString->GetValue());
    }
    break;

    case xiiCVarType::Float:
    {
      xiiCVarFloat* pFloat = static_cast<xiiCVarFloat*>(pCVar);
      s.Format("{0}", xiiArgF(pFloat->GetValue(), 4));
    }
    break;

    case xiiCVarType::Double:
    {
      xiiCVarDouble* pFloat = static_cast<xiiCVarDouble*>(pCVar);
      s.Format("{0}", xiiArgF(pFloat->GetValue(), 8));
    }
    break;

    case xiiCVarType::ENUM_COUNT:
      break;
  }

  return s.GetData();
}

xiiString xiiQuakeConsole::GetFullInfoAsString(xiiCVar* pCVar)
{
  xiiStringBuilder s = GetValueAsString(pCVar);

  const bool bAnyFlags = pCVar->GetFlags().IsAnySet(xiiCVarFlags::RequiresRestart | xiiCVarFlags::Save);

  if (bAnyFlags)
    s.Append(" [ ");

  if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
    s.Append("SAVE ");

  if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::RequiresRestart))
    s.Append("RESTART ");

  if (bAnyFlags)
    s.Append("]");

  return s;
}

const xiiString xiiCommandInterpreter::FindCommonString(const xiiDeque<xiiString>& vStrings)
{
  xiiStringBuilder sCommon;
  xiiUInt32        c;

  xiiUInt32 uiPos = 0;
  auto      it1   = vStrings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)vStrings.GetCount(); v++)
    {
      auto it2 = vStrings[v].GetIteratorFront();

      it2 += uiPos;

      if (it2.GetCharacter() != c)
        return sCommon;
    }

    sCommon.Append(c);

    ++uiPos;
    ++it1;
  }

  return sCommon;
}

void xiiCommandInterpreter::AutoComplete(xiiCommandInterpreterState& inout_State)
{
  xiiString sVarName = inout_State.m_sInput;

  auto it = rbegin(inout_State.m_sInput);

  // dots are allowed in CVar names
  while (it.IsValid() && (it.GetCharacter() == '.' || !xiiStringUtils::IsIdentifierDelimiter_C_Code(*it)))
    ++it;

  const char* szLastWordDelimiter = nullptr;
  if (it.IsValid() && xiiStringUtils::IsIdentifierDelimiter_C_Code(*it) && it.GetCharacter() != '.')
    szLastWordDelimiter = it.GetData();

  if (szLastWordDelimiter != nullptr)
    sVarName = szLastWordDelimiter + 1;

  xiiDeque<xiiString>        AutoCompleteOptions;
  xiiDeque<xiiConsoleString> AutoCompleteDescriptions;

  FindPossibleCVars(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);
  FindPossibleFunctions(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);

  if (AutoCompleteDescriptions.GetCount() > 1)
  {
    AutoCompleteDescriptions.Sort();

    inout_State.AddOutputLine("");

    for (xiiUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_State.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_State.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_State.m_sInput = xiiStringView(inout_State.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_State.m_sInput.Clear();

    inout_State.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}


XII_STATICLINK_FILE(Core, Core_Console_Implementation_Autocomplete);
