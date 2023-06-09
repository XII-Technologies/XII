#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

xiiResult xiiTexConv::ParseUIntOption(const char* szOption, xiiInt32 iMinValue, xiiInt32 iMaxValue, xiiUInt32& ref_uiResult) const
{
  const auto      pCmd      = xiiCommandLineUtils::GetGlobalInstance();
  const xiiUInt32 uiDefault = ref_uiResult;

  const xiiInt32 val = pCmd->GetIntOption(szOption, ref_uiResult);

  if (!xiiMath::IsInRange(val, iMinValue, iMaxValue))
  {
    xiiLog::Error("'{}' value {} is out of valid range [{}; {}]", szOption, val, iMinValue, iMaxValue);
    return XII_FAILURE;
  }

  ref_uiResult = static_cast<xiiUInt32>(val);

  if (ref_uiResult == uiDefault)
  {
    xiiLog::Info("Using default '{}': '{}'.", szOption, ref_uiResult);
    return XII_SUCCESS;
  }

  xiiLog::Info("Selected '{}': '{}'.", szOption, ref_uiResult);

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseStringOption(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed, xiiInt32& ref_iResult) const
{
  const auto             pCmd   = xiiCommandLineUtils::GetGlobalInstance();
  const xiiStringBuilder sValue = pCmd->GetStringOption(szOption, 0);

  if (sValue.IsEmpty())
  {
    ref_iResult = allowed[0].m_iEnumValue;

    xiiLog::Info("Using default '{}': '{}'", szOption, allowed[0].m_szKey);
    return XII_SUCCESS;
  }

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_szKey))
    {
      ref_iResult = allowed[i].m_iEnumValue;

      xiiLog::Info("Selected '{}': '{}'", szOption, allowed[i].m_szKey);
      return XII_SUCCESS;
    }
  }

  xiiLog::Error("Unknown value for option '{}': '{}'.", szOption, sValue);

  PrintOptionValues(szOption, allowed);

  return XII_FAILURE;
}

void xiiTexConv::PrintOptionValues(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const
{
  xiiLog::Info("Valid values for option '{}' are:", szOption);

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    xiiLog::Info("  {}", allowed[i].m_szKey);
  }
}

void xiiTexConv::PrintOptionValuesHelp(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const
{
  xiiStringBuilder out(szOption, " ");

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_szKey);
  }

  xiiLog::Info(out);
}

bool xiiTexConv::ParseFile(const char* szOption, xiiString& ref_sResult) const
{
  const auto pCmd = xiiCommandLineUtils::GetGlobalInstance();
  ref_sResult     = pCmd->GetAbsolutePathOption(szOption);

  if (!ref_sResult.IsEmpty())
  {
    xiiLog::Info("'{}' file: '{}'", szOption, ref_sResult);
    return true;
  }
  else
  {
    xiiLog::Info("No '{}' file specified.", szOption);
    return false;
  }
}
