#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiCommandLineOption);

void xiiCommandLineOption::GetSortingGroup(xiiStringBuilder& out) const
{
  out = m_szSortingGroup;
}

void xiiCommandLineOption::GetSplitOptions(xiiStringBuilder& outAll, xiiDynamicArray<xiiStringView>& splitOptions) const
{
  GetOptions(outAll);
  outAll.Split(false, splitOptions, ";", "|");
}

bool xiiCommandLineOption::IsHelpRequested(const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  return pUtils->GetBoolOption("-help") || pUtils->GetBoolOption("--help") || pUtils->GetBoolOption("-h") || pUtils->GetBoolOption("-?");
}

xiiResult xiiCommandLineOption::RequireOptions(const char* requiredOptions, xiiString* pMissingOption /*= nullptr*/, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  xiiStringBuilder                  tmp;
  xiiStringBuilder                  allOpts = requiredOptions;
  xiiHybridArray<xiiStringView, 16> options;
  allOpts.Split(false, options, ";");

  for (auto opt : options)
  {
    opt.Trim(" ");

    if (pUtils->GetOptionIndex(opt.GetData(tmp)) < 0)
    {
      if (pMissingOption)
      {
        *pMissingOption = opt;
      }

      return XII_FAILURE;
    }
  }

  if (pMissingOption)
  {
    pMissingOption->Clear();
  }

  return XII_SUCCESS;
}

bool xiiCommandLineOption::LogAvailableOptions(LogAvailableModes mode, const char* szGroupFilter /*= nullptr*/, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  if (mode == LogAvailableModes::IfHelpRequested)
  {
    if (!IsHelpRequested(pUtils))
      return false;
  }

  xiiMap<xiiString, xiiHybridArray<xiiCommandLineOption*, 16>> sorted;

  xiiStringBuilder sGroupFilter;
  if (!xiiStringUtils::IsNullOrEmpty(szGroupFilter))
  {
    sGroupFilter.Set(";", szGroupFilter, ";");
  }

  for (xiiCommandLineOption* pOpt = xiiCommandLineOption::GetFirstInstance(); pOpt != nullptr; pOpt = pOpt->GetNextInstance())
  {
    xiiStringBuilder sGroup;
    pOpt->GetSortingGroup(sGroup);
    sGroup.Prepend(";");
    sGroup.Append(";");

    if (!sGroupFilter.IsEmpty())
    {
      if (sGroupFilter.FindSubString_NoCase(sGroup) == nullptr)
        continue;
    }

    sorted[sGroup].PushBack(pOpt);
  }

  if (xiiApplication::GetApplicationInstance())
  {
    xiiLog::Info("");
    xiiLog::Info("{} command line options:", xiiApplication::GetApplicationInstance()->GetApplicationName());
  }

  if (sorted.IsEmpty())
  {
    xiiLog::Info("This application has no documented command line options.");
    return true;
  }

  xiiStringBuilder sLine;

  for (auto optIt : sorted)
  {
    for (auto pOpt : optIt.Value())
    {
      xiiStringBuilder sOptions, sParamShort, sParamDefault, sLongDesc;

      sLine.Clear();

      pOpt->GetOptions(sOptions);
      pOpt->GetParamShortDesc(sParamShort);
      pOpt->GetParamDefaultValueDesc(sParamDefault);
      pOpt->GetLongDesc(sLongDesc);

      xiiHybridArray<xiiStringView, 4> lines;

      sOptions.Split(false, lines, ";", "|");

      for (auto o : lines)
      {
        sLine.AppendWithSeparator(", ", o);
      }

      if (!sParamShort.IsEmpty())
      {
        sLine.Append(" ", sParamShort);

        if (!sParamDefault.IsEmpty())
        {
          sLine.Append(" = ", sParamDefault);
        }
      }

      xiiLog::Info("");
      xiiLog::Info(sLine);

      sLongDesc.Trim(" \t\n\r");
      sLongDesc.Split(true, lines, "\n");

      for (auto o : lines)
      {
        sLine = o;
        sLine.Trim("\t\n\r");
        sLine.Prepend("    ");

        xiiLog::Info(sLine);
      }
    }

    xiiLog::Info("");
  }

  xiiLog::Info("");

  return true;
}


bool xiiCommandLineOption::LogAvailableOptionsToBuffer(xiiStringBuilder& out_Buffer, LogAvailableModes mode, const char* szGroupFilter /*= nullptr*/, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  xiiLogSystemToBuffer log;
  xiiLogSystemScope    ls(&log);

  const bool res = xiiCommandLineOption::LogAvailableOptions(mode, szGroupFilter, pUtils);

  out_Buffer = log.m_sBuffer;

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionDoc::xiiCommandLineOptionDoc(const char* szSortingGroup, const char* szArgument, const char* szParamShortDesc, const char* szLongDesc, const char* szDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOption(szSortingGroup)
{
  m_szArgument          = szArgument;
  m_szParamShortDesc    = szParamShortDesc;
  m_szParamDefaultValue = szDefaultValue;
  m_szLongDesc          = szLongDesc;
  m_bCaseSensitive      = bCaseSensitive;
}

void xiiCommandLineOptionDoc::GetOptions(xiiStringBuilder& out) const
{
  out = m_szArgument;
}

void xiiCommandLineOptionDoc::GetParamShortDesc(xiiStringBuilder& out) const
{
  out = m_szParamShortDesc;
}

void xiiCommandLineOptionDoc::GetParamDefaultValueDesc(xiiStringBuilder& out) const
{
  out = m_szParamDefaultValue;
}

void xiiCommandLineOptionDoc::GetLongDesc(xiiStringBuilder& out) const
{
  out = m_szLongDesc;
}

bool xiiCommandLineOptionDoc::IsOptionSpecified(xiiStringBuilder* out_which, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiStringBuilder                 sOptions, tmp;
  xiiHybridArray<xiiStringView, 4> eachOption;
  GetSplitOptions(sOptions, eachOption);

  for (auto o : eachOption)
  {
    if (pUtils->GetOptionIndex(o.GetData(tmp), m_bCaseSensitive) >= 0)
    {
      if (out_which)
      {
        *out_which = tmp;
      }

      return true;
    }
  }

  if (out_which)
  {
    *out_which = m_szArgument;
  }

  return false;
}


bool xiiCommandLineOptionDoc::ShouldLog(LogMode mode, bool bWasSpecified) const
{
  if (mode == LogMode::Never)
    return false;

  if (m_bLoggedOnce && (mode == LogMode::FirstTime || mode == LogMode::FirstTimeIfSpecified))
    return false;

  if (!bWasSpecified && (mode == LogMode::FirstTimeIfSpecified || mode == LogMode::AlwaysIfSpecified))
    return false;

  return true;
}

void xiiCommandLineOptionDoc::LogOption(const char* szOption, const char* szValue, bool bWasSpecified) const
{
  m_bLoggedOnce = true;

  if (bWasSpecified)
  {
    xiiLog::Info("Option '{}' is set to '{}'", szOption, szValue);
  }
  else
  {
    xiiLog::Info("Option '{}' is not set, default value is '{}'", szOption, szValue);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionBool::xiiCommandLineOptionBool(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, bool bDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<bool>", szLongDesc, bDefaultValue ? "true" : "false", bCaseSensitive)
{
  m_bDefaultValue = bDefaultValue;
}

bool xiiCommandLineOptionBool::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  bool result = m_bDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetBoolOption(sOption, m_bDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result ? "true" : "false", bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionInt::xiiCommandLineOptionInt(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, int iDefaultValue, int iMinValue /*= xiiMath::MinValue<int>()*/, int iMaxValue /*= xiiMath::MaxValue<int>()*/, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<int>", szLongDesc, "0", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_iMinValue     = iMinValue;
  m_iMaxValue     = iMaxValue;

  XII_ASSERT_DEV(m_iMinValue < m_iMaxValue, "Invalid min/max value");
}

void xiiCommandLineOptionInt::GetParamDefaultValueDesc(xiiStringBuilder& out) const
{
  out.Format("{}", m_iDefaultValue);
}


void xiiCommandLineOptionInt::GetParamShortDesc(xiiStringBuilder& out) const
{
  if (m_iMinValue == xiiMath::MinValue<int>() && m_iMaxValue == xiiMath::MaxValue<int>())
  {
    out = "<int>";
  }
  else
  {
    out.Format("<int> [{} .. {}]", m_iMinValue, m_iMaxValue);
  }
}

int xiiCommandLineOptionInt::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  int result = m_iDefaultValue;

  xiiStringBuilder sOption, tmp;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetIntOption(sOption, m_iDefaultValue, m_bCaseSensitive);

    if (result < m_iMinValue || result > m_iMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        xiiLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_iMinValue, m_iMaxValue);
      }

      result = m_iDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.Format("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionFloat::xiiCommandLineOptionFloat(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, float fDefaultValue, float fMinValue /*= xiiMath::MinValue<float>()*/, float fMaxValue /*= xiiMath::MaxValue<float>()*/, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<float>", szLongDesc, "0", bCaseSensitive)
{
  m_fDefaultValue = fDefaultValue;
  m_fMinValue     = fMinValue;
  m_fMaxValue     = fMaxValue;

  XII_ASSERT_DEV(m_fMinValue < m_fMaxValue, "Invalid min/max value");
}

void xiiCommandLineOptionFloat::GetParamDefaultValueDesc(xiiStringBuilder& out) const
{
  out.Format("{}", m_fDefaultValue);
}

void xiiCommandLineOptionFloat::GetParamShortDesc(xiiStringBuilder& out) const
{
  if (m_fMinValue == xiiMath::MinValue<float>() && m_fMaxValue == xiiMath::MaxValue<float>())
  {
    out = "<float>";
  }
  else
  {
    out.Format("<float> [{} .. {}]", m_fMinValue, m_fMaxValue);
  }
}

float xiiCommandLineOptionFloat::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  float result = m_fDefaultValue;

  xiiStringBuilder sOption, tmp;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = static_cast<float>(pUtils->GetFloatOption(sOption, m_fDefaultValue, m_bCaseSensitive));

    if (result < m_fMinValue || result > m_fMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        xiiLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_fMinValue, m_fMaxValue);
      }

      result = m_fDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.Format("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionString::xiiCommandLineOptionString(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, const char* szDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<string>", szLongDesc, szDefaultValue, bCaseSensitive)
{
  m_szDefaultValue = szDefaultValue;
}

const char* xiiCommandLineOptionString::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  const char* result = m_szDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetStringOption(sOption, 0, m_szDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionPath::xiiCommandLineOptionPath(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, const char* szDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<path>", szLongDesc, szDefaultValue, bCaseSensitive)
{
  m_szDefaultValue = szDefaultValue;
}

xiiString xiiCommandLineOptionPath::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiString result = m_szDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetAbsolutePathOption(sOption, 0, m_szDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

xiiCommandLineOptionEnum::xiiCommandLineOptionEnum(const char* szSortingGroup, const char* szArgument, const char* szLongDesc, const char* szEnumKeysAndValues, xiiInt32 iDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(szSortingGroup, szArgument, "<enum>", szLongDesc, "", bCaseSensitive)
{
  m_iDefaultValue       = iDefaultValue;
  m_szEnumKeysAndValues = szEnumKeysAndValues;
}

xiiInt32 xiiCommandLineOptionEnum::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiInt32 result = m_iDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  xiiHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  if (bSpecified)
  {
    const char* selected = pUtils->GetStringOption(sOption, 0, "", m_bCaseSensitive);

    for (const auto& e : keysAndValues)
    {
      if (e.m_Key.IsEqual_NoCase(selected))
      {
        result = e.m_iValue;
        goto found;
      }
    }

    if (ShouldLog(logMode, bSpecified))
    {
      xiiLog::Warning("Option '{}' selected value '{}' is unknown. Using default value instead.", sOption, selected);
    }
  }

found:

  if (ShouldLog(logMode, bSpecified))
  {
    xiiStringBuilder opt;

    for (const auto& e : keysAndValues)
    {
      if (e.m_iValue == result)
      {
        opt = e.m_Key;
        break;
      }
    }

    LogOption(sOption, opt, bSpecified);
  }

  return result;
}

void xiiCommandLineOptionEnum::GetParamShortDesc(xiiStringBuilder& out) const
{
  xiiHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    out.AppendWithSeparator(" | ", e.m_Key);
  }

  out.Prepend("<");
  out.Append(">");
}

void xiiCommandLineOptionEnum::GetParamDefaultValueDesc(xiiStringBuilder& out) const
{
  xiiHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    if (m_iDefaultValue == e.m_iValue)
    {
      out = e.m_Key;
      return;
    }
  }
}

void xiiCommandLineOptionEnum::GetEnumKeysAndValues(xiiDynamicArray<EnumKeyValue>& out_KeysAndValues) const
{
  xiiStringBuilder tmp = m_szEnumKeysAndValues;

  xiiHybridArray<xiiStringView, 16> enums;
  tmp.Split(false, enums, ";", "|");

  out_KeysAndValues.SetCount(enums.GetCount());

  xiiInt32 eVal = 0;
  for (xiiUInt32 e = 0; e < enums.GetCount(); ++e)
  {
    xiiStringView eName;

    if (const char* eq = enums[e].FindSubString("="))
    {
      eName = xiiStringView(enums[e].GetStartPointer(), eq);

      XII_VERIFY(xiiConversionUtils::StringToInt(eq + 1, eVal).Succeeded(), "Invalid enum declaration");
    }
    else
    {
      eName = enums[e];
    }

    eName.Trim(" \n\r\t=");

    const char* pStart = m_szEnumKeysAndValues;
    pStart += (xiiInt64)eName.GetStartPointer();
    pStart -= (xiiInt64)tmp.GetData();

    out_KeysAndValues[e].m_iValue = eVal;
    out_KeysAndValues[e].m_Key    = xiiStringView(pStart, eName.GetElementCount());

    eVal++;
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_CommandLineOptions);
