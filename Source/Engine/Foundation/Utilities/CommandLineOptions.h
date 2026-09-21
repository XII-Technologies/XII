/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/EnumerableClass.h>

class xiiStringBuilder;
class xiiLogInterface;

/// xiiCommandLineOption (and derived types) are used to define options that the application supports.
///
/// Command line options are created as global variables anywhere throughout the code, wherever they are needed.
/// The point of using them over going through xiiCommandLineUtils directly, is that the options can be listed automatically
/// and thus an application can print all available options, when the user requests help.
///
/// Consequently, their main purpose is to make options discoverable and to document them in a consistent manner.
///
/// Additionally, classes like xiiCommandLineOptionEnum add functionality that makes some options easier to setup.
class XII_FOUNDATION_DLL xiiCommandLineOption : public xiiEnumerable<xiiCommandLineOption>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiCommandLineOption);

public:
  enum class LogAvailableModes
  {
    Always,         ///< Logs the available modes no matter what
    IfHelpRequested ///< Only logs the modes, if '-h', '-help', '-?' or something similar was specified
  };

  /// Describes whether the value of an option (and whether something went wrong), should be printed to xiiLog.
  enum class LogMode
  {
    Never,                ///< Don't log anything.
    FirstTime,            ///< Only print the information the first time a value is accessed.
    FirstTimeIfSpecified, ///< Only on first access and only if the user specified the value on the command line.
    Always,               ///< Always log the options value on access.
    AlwaysIfSpecified,    ///< Always log values, if the user specified non-default ones.
  };

  /// Checks whether a command line was passed that requests help output.
  static bool IsHelpRequested(const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Checks whether all required options are passed to the command line.
  ///
  /// The options are passed as a semicolon-separated list (spare spaces are stripped away), for instance "-opt1; -opt2"
  static xiiResult RequireOptions(xiiStringView sRequiredOptions, xiiString* pMissingOption = nullptr, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Prints all available options to the xiiLog.
  ///
  /// \param sGroupFilter
  ///   If this is empty, all options from all 'sorting groups' are logged.
  ///   If non-empty, only options from sorting groups that appear in this string will be logged.
  static bool LogAvailableOptions(LogAvailableModes mode, xiiStringView sGroupFilter = {}, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Same as LogAvailableOptions() but captures the output from xiiLog and returns it in a xiiStringBuilder.
  static bool LogAvailableOptionsToBuffer(xiiStringBuilder& out_sBuffer, LogAvailableModes mode, xiiStringView sGroupFilter = {}, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()); // [tested]

public:
  /// \param sSortingGroup
  ///   This string is used to sort options. Application options should start with an underscore, such that they appear first
  ///   in the output.
  xiiCommandLineOption(xiiStringView sSortingGroup) { m_sSortingGroup = sSortingGroup; }

  /// Writes the sorting group name to 'out'.
  virtual void GetSortingGroup(xiiStringBuilder& ref_sOut) const;

  /// Writes all the supported options (e.g. '-arg') to 'out'.
  /// If more than one option is allowed, they should be separated with semicolons or pipes.
  virtual void GetOptions(xiiStringBuilder& ref_sOut) const = 0;

  /// Returns the supported option names (e.g. '-arg') as split strings.
  void GetSplitOptions(xiiStringBuilder& out_sAll, xiiDynamicArray<xiiStringView>& ref_splitOptions) const;

  /// Returns a very short description of the option (type). For example "<int>" or "<enum>".
  virtual void GetParamShortDesc(xiiStringBuilder& ref_sOut) const = 0;

  /// Returns a very short string for the options default value. For example "0" or "auto".
  virtual void GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const = 0;

  /// Returns a proper description of the option.
  ///
  /// The long description is allowed to contain newlines (\n) and the output will be formatted accordingly.
  virtual void GetLongDesc(xiiStringBuilder& ref_sOut) const = 0;

  /// Returns a string indicating the exact implementation type.
  virtual xiiStringView GetType() = 0;

protected:
  xiiStringView m_sSortingGroup;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// xiiCommandLineOptionDoc can be used to document a command line option whose logic might be more complex than what the other option types provide.
///
/// This class is meant to be used for options that are actually queried directly through xiiCommandLineUtils,
/// but should still show up in the command line option documentation, such that the user can discover them.
///
class XII_FOUNDATION_DLL xiiCommandLineOptionDoc : public xiiCommandLineOption
{
public:
  xiiCommandLineOptionDoc(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sParamShortDesc, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive = false);

  virtual void GetOptions(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetLongDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  /// Returns "Doc"
  virtual xiiStringView GetType() override { return "Doc"; }

  /// Checks whether any of the option variants is set on the command line, and returns which one. For example '-h' or '-help'.
  bool IsOptionSpecified(xiiStringBuilder* out_pWhich = nullptr, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

protected:
  bool ShouldLog(LogMode mode, bool bWasSpecified) const;
  void LogOption(xiiStringView sOption, xiiStringView sValue, bool bWasSpecified) const;

  xiiStringView m_sArgument;
  xiiStringView m_sParamShortDesc;
  xiiStringView m_sParamDefaultValue;
  xiiStringView m_sLongDesc;
  bool          m_bCaseSensitive = false;
  mutable bool  m_bLoggedOnce    = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes simple on/off switches.
class XII_FOUNDATION_DLL xiiCommandLineOptionBool : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionBool(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  bool GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(bool value)
  {
    m_bDefaultValue = value;
  }

  /// Returns the default value.
  bool GetDefaultValue() const { return m_bDefaultValue; }

  /// Returns "Bool"
  virtual xiiStringView GetType() override { return "Bool"; }

protected:
  bool m_bDefaultValue = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes integer values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class XII_FOUNDATION_DLL xiiCommandLineOptionInt : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionInt(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiInt32 iDefaultValue, xiiInt32 iMinValue = xiiMath::MinValue<xiiInt32>(), xiiInt32 iMaxValue = xiiMath::MaxValue<xiiInt32>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  xiiInt32 GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(xiiInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// Returns "Int"
  virtual xiiStringView GetType() override { return "Int"; }

  /// Returns the minimum value.
  xiiInt32 GetMinValue() const { return m_iMinValue; }

  /// Returns the maximum value.
  xiiInt32 GetMaxValue() const { return m_iMaxValue; }

  /// Returns the default value.
  xiiInt32 GetDefaultValue() const { return m_iDefaultValue; }

protected:
  xiiInt32 m_iDefaultValue = 0;
  xiiInt32 m_iMinValue     = 0;
  xiiInt32 m_iMaxValue     = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes float values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class XII_FOUNDATION_DLL xiiCommandLineOptionFloat : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionFloat(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, float fDefaultValue, float fMinValue = xiiMath::MinValue<float>(), float fMaxValue = xiiMath::MaxValue<float>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  float GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(float value)
  {
    m_fDefaultValue = value;
  }

  /// Returns "Float"
  virtual xiiStringView GetType() override { return "Float"; }

  /// Returns the minimum value.
  float GetMinValue() const { return m_fMinValue; }

  /// Returns the maximum value.
  float GetMaxValue() const { return m_fMaxValue; }

  /// Returns the default value.
  float GetDefaultValue() const { return m_fDefaultValue; }

protected:
  float m_fDefaultValue = 0;
  float m_fMinValue     = 0;
  float m_fMaxValue     = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes simple string values.
class XII_FOUNDATION_DLL xiiCommandLineOptionString : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionString(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  xiiStringView GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(xiiStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// Returns the default value.
  xiiStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// Returns "String"
  virtual xiiStringView GetType() override { return "String"; }

protected:
  xiiStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes absolute paths. If the user provides a relative path, it will be concatenated with the current working directory.
class XII_FOUNDATION_DLL xiiCommandLineOptionPath : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionPath(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  xiiString GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(xiiStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// Returns the default value.
  xiiStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// Returns "Path"
  virtual xiiStringView GetType() override { return "Path"; }

protected:
  xiiStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// An 'enum' option is a string option that only allows certain phrases ('keys').
///
/// Each phrase has an integer value, and GetOptionValue() returns the integer value of the selected phrase.
/// It is valid for the default value to be different from all the phrase values,
/// which can be used to detect whether the user provided any phrase at all.
///
/// The allowed values are passed in as a single string, in the form "OptA = 0 | OptB = 1 | ..."
/// Phrase values ("= 0" etc) are optional, and if not given are automatically assigned starting at zero.
/// Multiple phrases may share the same value.
class XII_FOUNDATION_DLL xiiCommandLineOptionEnum : public xiiCommandLineOptionDoc
{
public:
  xiiCommandLineOptionEnum(xiiStringView sSortingGroup, xiiStringView sArgument, xiiStringView sLongDesc, xiiStringView sEnumKeysAndValues, xiiInt32 iDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  xiiInt32 GetOptionValue(LogMode logMode, const xiiCommandLineUtils* pUtils = xiiCommandLineUtils::GetGlobalInstance()) const; // [tested]

  virtual void GetParamShortDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamDefaultValueDesc(xiiStringBuilder& ref_sOut) const override; // [tested]

  struct EnumKeyValue
  {
    xiiStringView m_Key;
    xiiInt32      m_iValue = 0;
  };

  /// Returns the enum keys (names) and values (integers) extracted from the string that was passed to the constructor.
  void GetEnumKeysAndValues(xiiDynamicArray<EnumKeyValue>& out_keysAndValues) const;

  /// Modifies the default value
  void SetDefaultValue(xiiInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// Returns the default value.
  xiiInt32 GetDefaultValue() const { return m_iDefaultValue; }

  /// Returns "Enum"
  virtual xiiStringView GetType() override { return "Enum"; }

protected:
  xiiInt32      m_iDefaultValue = 0;
  xiiStringView m_sEnumKeysAndValues;
};
