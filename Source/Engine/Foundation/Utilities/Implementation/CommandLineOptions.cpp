/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiCommandLineOption);

void xiiCommandLineOption::GetSortingGroup(xiiStringBuilder& ref_sOut) const
{
  ref_sOut = m_sSortingGroup;
}

void xiiCommandLineOption::GetSplitOptions(xiiStringBuilder& out_sAll, xiiDynamicArray<xiiStringView>& ref_splitOptions) const
{
  GetOptions(out_sAll);
  out_sAll.Split(false, ref_splitOptions, ";", "|");
}

bool xiiCommandLineOption::IsHelpRequested(const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  return pUtils->GetBoolOption("-help") || pUtils->GetBoolOption("--help") || pUtils->GetBoolOption("-h") || pUtils->GetBoolOption("-?");
}

xiiResult xiiCommandLineOption::RequireOptions(xiiStringView sRequiredOptions, xiiString* pMissingOption /*= nullptr*/, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  xiiStringBuilder                  tmp;
  xiiStringBuilder                  allOpts = sRequiredOptions;
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

bool xiiCommandLineOption::LogAvailableOptions(LogAvailableModes mode, xiiStringView sGroupFilter /*= {} */, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  if (mode == LogAvailableModes::IfHelpRequested)
  {
    if (!IsHelpRequested(pUtils))
      return false;
  }

  xiiMap<xiiString, xiiHybridArray<xiiCommandLineOption*, 16>> sorted;

  xiiStringBuilder sGroupFilterResult;
  if (!sGroupFilter.IsEmpty())
  {
    sGroupFilterResult.Set(";", sGroupFilter, ";");
  }

  for (xiiCommandLineOption* pOpt = xiiCommandLineOption::GetFirstInstance(); pOpt != nullptr; pOpt = pOpt->GetNextInstance())
  {
    xiiStringBuilder sGroup;
    pOpt->GetSortingGroup(sGroup);
    sGroup.Prepend(";");
    sGroup.Append(";");

    if (!sGroupFilterResult.IsEmpty())
    {
      if (sGroupFilterResult.FindSubString_NoCase(sGroup) == nullptr)
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


bool xiiCommandLineOption::LogAvailableOptionsToBuffer(xiiStringBuilder& out_sBuffer, LogAvailableModes mode, xiiStringView sGroupFilter /*= {} */, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/)
{
  xiiLogSystemToBuffer log;
  xiiLogSystemScope    ls(&log);

  const bool res = xiiCommandLineOption::LogAvailableOptions(mode, sGroupFilter, pUtils);

  out_sBuffer = log.m_sBuffer;

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionDoc::xiiCommandLineOptionDoc(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sParamShortDesc, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOption(sSortingGroup)
{
  m_sArgument          = sArgument;
  m_sParamShortDesc    = sParamShortDesc;
  m_sParamDefaultValue = sDefaultValue;
  m_sLongDesc          = sLongDesc;
  m_bCaseSensitive     = bCaseSensitive;
}

void xiiCommandLineOptionDoc::GetOptions(xiiStringBuilder& ref_sOut) const
{
  ref_sOut = m_sArgument;
}

void xiiCommandLineOptionDoc::GetParamShortDesc(xiiStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamShortDesc;
}

void xiiCommandLineOptionDoc::GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamDefaultValue;
}

void xiiCommandLineOptionDoc::GetLongDesc(xiiStringBuilder& ref_sOut) const
{
  ref_sOut = m_sLongDesc;
}

bool xiiCommandLineOptionDoc::IsOptionSpecified(xiiStringBuilder* out_pWhich, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiStringBuilder                 sOptions, tmp;
  xiiHybridArray<xiiStringView, 4> eachOption;
  GetSplitOptions(sOptions, eachOption);

  for (auto o : eachOption)
  {
    if (pUtils->GetOptionIndex(o.GetData(tmp), m_bCaseSensitive) >= 0)
    {
      if (out_pWhich)
      {
        *out_pWhich = tmp;
      }

      return true;
    }
  }

  if (out_pWhich)
  {
    *out_pWhich = m_sArgument;
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

void xiiCommandLineOptionDoc::LogOption(xiiStringView sOption, xiiStringView sValue, bool bWasSpecified) const
{
  m_bLoggedOnce = true;

  if (bWasSpecified)
  {
    xiiLog::Info("Option '{}' is set to '{}'", sOption, sValue);
  }
  else
  {
    xiiLog::Info("Option '{}' is not set, default value is '{}'", sOption, sValue);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionBool::xiiCommandLineOptionBool(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<bool>", sLongDesc, bDefaultValue ? "true" : "false", bCaseSensitive)
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

xiiCommandLineOptionInt::xiiCommandLineOptionInt(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiInt32 iDefaultValue, xiiInt32 iMinValue /*= xiiMath::MinValue<xiiInt32>()*/, xiiInt32 iMaxValue /*= xiiMath::MaxValue<xiiInt32>()*/, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<int>", sLongDesc, "0", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_iMinValue     = iMinValue;
  m_iMaxValue     = iMaxValue;

  XII_ASSERT_DEV(m_iMinValue < m_iMaxValue, "Invalid min/max value");
}

void xiiCommandLineOptionInt::GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_iDefaultValue);
}


void xiiCommandLineOptionInt::GetParamShortDesc(xiiStringBuilder& ref_sOut) const
{
  if (m_iMinValue == xiiMath::MinValue<xiiInt32>() && m_iMaxValue == xiiMath::MaxValue<xiiInt32>())
  {
    ref_sOut = "<int>";
  }
  else
  {
    ref_sOut.SetFormat("<int> [{} .. {}]", m_iMinValue, m_iMaxValue);
  }
}

xiiInt32 xiiCommandLineOptionInt::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiInt32 result = m_iDefaultValue;

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
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionFloat::xiiCommandLineOptionFloat(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, float fDefaultValue, float fMinValue /*= xiiMath::MinValue<float>()*/, float fMaxValue /*= xiiMath::MaxValue<float>()*/, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<float>", sLongDesc, "0", bCaseSensitive)
{
  m_fDefaultValue = fDefaultValue;
  m_fMinValue     = fMinValue;
  m_fMaxValue     = fMaxValue;

  XII_ASSERT_DEV(m_fMinValue < m_fMaxValue, "Invalid min/max value");
}

void xiiCommandLineOptionFloat::GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_fDefaultValue);
}

void xiiCommandLineOptionFloat::GetParamShortDesc(xiiStringBuilder& ref_sOut) const
{
  if (m_fMinValue == xiiMath::MinValue<float>() && m_fMaxValue == xiiMath::MaxValue<float>())
  {
    ref_sOut = "<float>";
  }
  else
  {
    ref_sOut.SetFormat("<float> [{} .. {}]", m_fMinValue, m_fMaxValue);
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
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionString::xiiCommandLineOptionString(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<string>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

xiiStringView xiiCommandLineOptionString::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiStringView result = m_sDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetStringOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
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

xiiCommandLineOptionPath::xiiCommandLineOptionPath(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<path>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

xiiString xiiCommandLineOptionPath::GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils /*= xiiCommandLineUtils::GetGlobalInstance()*/) const
{
  xiiString result = m_sDefaultValue;

  xiiStringBuilder sOption;
  const bool       bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetAbsolutePathOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

xiiCommandLineOptionEnum::xiiCommandLineOptionEnum(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sEnumKeysAndValues, xiiInt32 iDefaultValue, bool bCaseSensitive /*= false*/) :
  xiiCommandLineOptionDoc(sSortingGroup, sArgument, "<enum>", sLongDesc, "", bCaseSensitive)
{
  m_iDefaultValue      = iDefaultValue;
  m_sEnumKeysAndValues = sEnumKeysAndValues;
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
    xiiStringView selected = pUtils->GetStringOption(sOption, 0, "", m_bCaseSensitive);

    for (const auto& e : keysAndValues)
    {
      if (e.m_Key.IsEqual_NoCase(selected))
      {
        result = e.m_iValue;
        goto Found;
      }
    }

    if (ShouldLog(logMode, bSpecified))
    {
      xiiLog::Warning("Option '{}' selected value '{}' is unknown. Using default value instead.", sOption, selected);
    }
  }

Found:

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

void xiiCommandLineOptionEnum::GetParamShortDesc(xiiStringBuilder& ref_sOut) const
{
  xiiHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    ref_sOut.AppendWithSeparator(" | ", e.m_Key);
  }

  ref_sOut.Prepend("<");
  ref_sOut.Append(">");
}

void xiiCommandLineOptionEnum::GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const
{
  xiiHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    if (m_iDefaultValue == e.m_iValue)
    {
      ref_sOut = e.m_Key;
      return;
    }
  }
}

void xiiCommandLineOptionEnum::GetEnumKeysAndValues(xiiDynamicArray<EnumKeyValue>& out_keysAndValues) const
{
  xiiStringBuilder tmp = m_sEnumKeysAndValues;

  xiiHybridArray<xiiStringView, 16> enums;
  tmp.Split(false, enums, ";", "|");

  out_keysAndValues.SetCount(enums.GetCount());

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

    const char* pStart = m_sEnumKeysAndValues.GetStartPointer();
    pStart += (xiiInt64)eName.GetStartPointer();
    pStart -= (xiiInt64)tmp.GetData();

    out_keysAndValues[e].m_iValue = eVal;
    out_keysAndValues[e].m_Key    = xiiStringView(pStart, eName.GetElementCount());

    eVal++;
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_CommandLineOptions);
