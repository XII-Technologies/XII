/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <array>
#include <tuple>
#include <utility>

template <typename... ARGS>
class xiiFormatStringImpl : public xiiFormatString
{
  // this is the size of the temp buffer that BuildString functions get for writing their result to.
  // The buffer is always available and allocated on the stack, so this prevents the need for memory allocations.
  // If a BuildString function requires no storage at all, it can return a xiiStringView to unrelated memory
  // (e.g. if the memory already exists).
  // If a BuildString function requires more storage, it may need to do some trickery.
  // For an example look at BuildString for xiiArgErrorCode, which uses an increased thread_local temp buffer.
  static constexpr xiiUInt32 TempStringLength = 64;
  // Maximum number of parameters. Results in compilation error if exceeded.
  static constexpr xiiUInt32 MaxNumParameters = 12;

public:
  xiiFormatStringImpl(xiiStringView sFormat, ARGS&&... args) :
    m_Arguments(std::forward<ARGS>(args)...)
  {
    m_sString = sFormat;
  }

  xiiFormatStringImpl(const char* szFormat, ARGS&&... args) :
    m_Arguments(std::forward<ARGS>(args)...)
  {
    m_sString = szFormat;
  }

  /// Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires a xiiStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a xiiStringView to that
  /// string builder data for convenience.
  virtual xiiStringView GetText(xiiStringBuilder& ref_sStorage) const override
  {
    if (m_sString.IsEmpty())
    {
      return {};
    }

    xiiStringView param[MaxNumParameters];

    char tmp[MaxNumParameters][TempStringLength];
    ReplaceString<0>(tmp, param);

    return BuildFormattedText(ref_sStorage, param, MaxNumParameters);
  }

  virtual const char* GetTextCStr(xiiStringBuilder& out_sString) const override
  {
    xiiStringView param[MaxNumParameters];

    char tmp[MaxNumParameters][TempStringLength];
    ReplaceString<0>(tmp, param);

    return BuildFormattedText(out_sString, param, MaxNumParameters).GetStartPointer();
  }

private:
  template <xiiInt32 N>
  typename std::enable_if<sizeof...(ARGS) != N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], xiiStringView* pViews) const
  {
    static_assert(N < MaxNumParameters, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    pViews[N] = BuildString(tmp[N], TempStringLength - 1, std::get<N>(m_Arguments));

    // Recurse, chip off one argument
    ReplaceString<N + 1>(tmp, pViews);
  }

  // Recursion end if we reached the number of arguments.
  template <xiiInt32 N>
  typename std::enable_if<sizeof...(ARGS) == N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], xiiStringView* pViews) const
  {
    XII_IGNORE_UNUSED(tmp);
    XII_IGNORE_UNUSED(pViews);
  }

  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};
