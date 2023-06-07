#pragma once

#include <array>
#include <tuple>
#include <utility>

template <typename... ARGS>
class xiiFormatStringImpl : public xiiFormatString
{
  // this is the size of the temp buffer that BuildString functions get for writing their result to.
  // The buffer is always available and allocated on the stack, so this prevents the need for memory allocations.
  // If a BuildString function requires no storage at all, it can return an xiiStringView to unrelated memory
  // (e.g. if the memory already exists).
  // If a BuildString function requires more storage, it may need to do some trickery.
  // For an example look at BuildString for xiiArgErrorCode, which uses an increased thread_local temp buffer.
  static constexpr xiiUInt32 TempStringLength = 64;
  // Maximum number of parameters. Results in compilation error if exceeded.
  static constexpr xiiUInt32 MaxNumParameters = 12;

public:
  xiiFormatStringImpl(const char* szFormat, ARGS&&... args) :
    m_Arguments(std::forward<ARGS>(args)...)
  {
    m_szString = szFormat;
  }

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an xiiStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a const char* to that
  /// string builder data for convenience.
  virtual const char* GetText(xiiStringBuilder& ref_sSb) const override
  {
    if (xiiStringUtils::IsNullOrEmpty(m_szString))
    {
      return "";
    }

    xiiStringView param[MaxNumParameters];

    char tmp[MaxNumParameters][TempStringLength];
    ReplaceString<0>(tmp, param);

    const char* szString = m_szString;

    int iLastParam = -1;

    SBClear(ref_sSb);
    while (*szString != '\0')
    {
      if (*szString == '%')
      {
        if (*(szString + 1) == '%')
        {
          SBAppendView(ref_sSb, "%");
        }
        else
        {
          XII_ASSERT_DEBUG(false, "Single percentage signs are not allowed in xiiFormatString. Did you forgot to migrate a printf-style "
                                  "string? Use double percentage signs for the actual character.");
        }

        szString += 2;
      }
      else if (*szString == '{' && *(szString + 1) >= '0' && *(szString + 1) <= '9' && *(szString + 2) == '}')
      {
        iLastParam = *(szString + 1) - '0';
        SBAppendView(ref_sSb, param[iLastParam]);

        szString += 3;
      }
      else if (*szString == '{' && *(szString + 1) == '}')
      {
        ++iLastParam;
        XII_ASSERT_DEV(iLastParam < MaxNumParameters, "Too many placeholders in format string");

        if (iLastParam < MaxNumParameters)
        {
          SBAppendView(ref_sSb, param[iLastParam]);
        }

        szString += 2;
      }
      else
      {
        const xiiUInt32 character = xiiUnicodeUtils::DecodeUtf8ToUtf32(szString);
        SBAppendChar(ref_sSb, character);
      }
    }

    return SBReturn(ref_sSb);
  }

private:
  template <xiiInt32 N>
  typename std::enable_if<sizeof...(ARGS) != N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], xiiStringView* pViews) const
  {
    XII_CHECK_AT_COMPILETIME_MSG(N < MaxNumParameters, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    pViews[N] = BuildString(tmp[N], TempStringLength - 1, std::get<N>(m_Arguments));

    // Recurse, chip off one argument
    ReplaceString<N + 1>(tmp, pViews);
  }

  // Recursion end if we reached the number of arguments.
  template <xiiInt32 N>
  typename std::enable_if<sizeof...(ARGS) == N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], xiiStringView* pViews) const
  {
  }


  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};
