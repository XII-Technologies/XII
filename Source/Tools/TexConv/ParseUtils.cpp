#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

xiiResult xiiTexConv::ParseUIntOption(xiiStringView sOption, xiiInt32 iMinValue, xiiInt32 iMaxValue, xiiUInt32& ref_uiResult) const
{
  const auto      pCmd      = xiiCommandLineUtils::GetGlobalInstance();
  const xiiUInt32 uiDefault = ref_uiResult;

  const xiiInt32 val = pCmd->GetIntOption(sOption, ref_uiResult);

  if (!xiiMath::IsInRange(val, iMinValue, iMaxValue))
  {
    xiiLog::Error("'{}' value {} is out of valid range [{}; {}]", sOption, val, iMinValue, iMaxValue);
    return XII_FAILURE;
  }

  ref_uiResult = static_cast<xiiUInt32>(val);

  if (ref_uiResult == uiDefault)
  {
    xiiLog::Info("Using default '{}': '{}'.", sOption, ref_uiResult);
    return XII_SUCCESS;
  }

  xiiLog::Info("Selected '{}': '{}'.", sOption, ref_uiResult);

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseStringOption(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed, xiiInt32& ref_iResult) const
{
  const auto             pCmd   = xiiCommandLineUtils::GetGlobalInstance();
  const xiiStringBuilder sValue = pCmd->GetStringOption(sOption, 0);

  if (sValue.IsEmpty())
  {
    ref_iResult = allowed[0].m_iEnumValue;

    xiiLog::Info("Using default '{}': '{}'", sOption, allowed[0].m_sKey);
    return XII_SUCCESS;
  }

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_sKey))
    {
      ref_iResult = allowed[i].m_iEnumValue;

      xiiLog::Info("Selected '{}': '{}'", sOption, allowed[i].m_sKey);
      return XII_SUCCESS;
    }
  }

  xiiLog::Error("Unknown value for option '{}': '{}'.", sOption, sValue);

  PrintOptionValues(sOption, allowed);

  return XII_FAILURE;
}

void xiiTexConv::PrintOptionValues(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const
{
  xiiLog::Info("Valid values for option '{}' are:", sOption);

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    xiiLog::Info("  {}", allowed[i].m_sKey);
  }
}

void xiiTexConv::PrintOptionValuesHelp(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const
{
  xiiStringBuilder out(sOption, " ");

  for (xiiUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_sKey);
  }

  xiiLog::Info(out);
}

bool xiiTexConv::ParseFile(xiiStringView sOption, xiiString& ref_sResult) const
{
  const auto pCmd = xiiCommandLineUtils::GetGlobalInstance();
  ref_sResult     = pCmd->GetAbsolutePathOption(sOption);

  if (!ref_sResult.IsEmpty())
  {
    xiiLog::Info("'{}' file: '{}'", sOption, ref_sResult);
    return true;
  }
  else
  {
    xiiLog::Info("No '{}' file specified.", sOption);
    return false;
  }
}
