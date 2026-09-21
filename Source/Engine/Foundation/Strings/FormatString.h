/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifndef XII_INCLUDING_BASICS_H
#  error "FormatString.h must not be included directly, but instead include Foundation/Basics.h."
#endif

class xiiStringBuilder;

#include <Foundation/Strings/StringView.h>

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

/// Implements formating of strings with placeholders and formatting options.
///
/// xiiFormatString can be used anywhere where a string should be formatable when passing it into a function.
/// Good examples are xiiStringBuilder::SetFormat() or xiiLog::Info().
///
/// A function taking a xiiFormatString can internally call xiiFormatString::GetText() to retrieve he formatted result.
/// When calling such a function, one must wrap the parameter into 'xiiFmt' to enable formatting options, example:
///   void MyFunc(const xiiFormatString& text);
///   MyFunc(xiiFmt("Cool Story {}", "Bro"));
///
/// To provide more convenience, one can add a template-function overload like this:
///   template <typename... ARGS>
///   void MyFunc(const char* szFormat, ARGS&&... args)
///   {
///     MyFunc(xiiFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
///   }
///
/// This allows to call MyFunc() without the 'xiiFmt' wrapper.
///
///
/// === Formatting ===
///
/// Placeholders for variables are specified using '{}'. These may use numbers from 0 to 9,
/// ie. {0}, {3}, {2}, etc. which allows to change the order or insert duplicates.
/// If no number is provided, each {} instance represents the next argument.
///
/// To specify special formatting, wrap the argument into a xiiArgXY call:
///   xiiArgC - for characters
///   xiiArgI - for integer formatting
///   xiiArgU - for unsigned integer formatting (e.g. HEX)
///   xiiArgF - for floating point formatting
///   xiiArgP - for pointer formatting
///   xiiArgDateTime - for xiiDateTime formatting options
///   xiiArgErrorCode - for Windows error code formatting
///   xiiArgHumanReadable - for shortening numbers with common abbreviations
///   xiiArgFileSize - for representing file sizes
///
/// Example:
///   xiiStringBuilder::SetFormat("HEX: {}", xiiArgU(1337, 8 /*width*/, true /*pad with zeros*/, 16 /*base16*/, true/*upper case*/));
///
/// Arbitrary other types can support special formatting even without a xiiArgXY call. E.g. xiiTime and xiiAngle do special formatting.
/// xiiArgXY calls are only necessary if formatting options are needed for a specific formatting should be enforced (e.g. xiiArgErrorCode
/// would otherwise just use uint32 formatting).
///
/// To implement custom formatting see the various free standing 'BuildString' functions.
class XII_FOUNDATION_DLL xiiFormatString
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiFormatString); // pass by reference, never pass by value

public:
  XII_ALWAYS_INLINE xiiFormatString() = default;
  XII_ALWAYS_INLINE xiiFormatString(const char* szString) { m_sString = szString; }
  XII_ALWAYS_INLINE xiiFormatString(xiiStringView sString) { m_sString = sString; }

  xiiFormatString(const xiiStringBuilder& s);
  virtual ~xiiFormatString() = default;

  /// Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires a xiiStringBuilder as storage, ie. POTENTIALLY writes the formatted text into it.
  /// However, if no formatting is required, it may not touch the string builder at all and just return a string directly.
  ///
  /// \note Do not assume that the result is stored in \a sb. Always only use the return value. The string builder is only used
  /// when necessary.
  [[nodiscard]] virtual xiiStringView GetText(xiiStringBuilder&) const { return m_sString; }

  /// Similar to GetText() but guaranteed to copy the string into the given string builder,
  /// and thus guaranteeing that the generated string is zero terminated.
  virtual const char* GetTextCStr(xiiStringBuilder& out_sString) const;

  bool IsEmpty() const { return m_sString.IsEmpty(); }

  /// Helper function to build the formatted text with the given arguments.
  ///
  /// \note We can't use xiiArrayPtr here because of include order.
  xiiStringView BuildFormattedText(xiiStringBuilder& ref_sStorage, xiiStringView* pArgs, xiiUInt32 uiNumArgs) const;

protected:
  xiiStringView m_sString;
};

#include <Foundation/Strings/Implementation/FormatStringImpl.h>

template <typename... ARGS>
XII_ALWAYS_INLINE xiiFormatStringImpl<ARGS...> xiiFmt(const char* szFormat, ARGS&&... args)
{
  return xiiFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...);
}
