#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  struct ExpectedToken
  {
    xiiTokenType::Enum type;
    xiiStringView      value;
  };

  void CompareResults(const xiiDynamicArray<ExpectedToken>& expected, xiiTokenizer& inout_tokenizer, bool bIgnoreWhitespace)
  {
    auto& tokens = inout_tokenizer.GetTokens();

    const xiiUInt32 expectedCount = expected.GetCount();
    const xiiUInt32 tokenCount    = tokens.GetCount();

    xiiUInt32 expectedIndex = 0, tokenIndex = 0;
    while (expectedIndex < expectedCount && tokenIndex < tokenCount)
    {
      auto& token = tokens[tokenIndex];
      if (bIgnoreWhitespace && (token.m_iType == xiiTokenType::Whitespace || token.m_iType == xiiTokenType::Newline))
      {
        tokenIndex++;
        continue;
      }

      auto& e = expected[expectedIndex];

      if (!XII_TEST_BOOL_MSG(e.type == token.m_iType, "Token with index %u does not match in type, expected %d actual %d", expectedIndex, e.type, token.m_iType))
      {
        return;
      }

      if (!XII_TEST_BOOL_MSG(e.value == token.m_DataView, "Token with index %u does not match, expected '%.*s' actual '%.*s'", expectedIndex, e.value.GetElementCount(), e.value.GetStartPointer(), token.m_DataView.GetElementCount(), token.m_DataView.GetStartPointer()))
      {
        return;
      }
      tokenIndex++;
      expectedIndex++;
    }

    // Skip remaining whitespace and newlines
    if (bIgnoreWhitespace)
    {
      while (tokenIndex < tokenCount)
      {
        auto& token = tokens[tokenIndex];
        if (token.m_iType != xiiTokenType::Whitespace && token.m_iType != xiiTokenType::Newline)
        {
          break;
        }
        tokenIndex++;
      }
    }

    if (XII_TEST_BOOL_MSG(tokenIndex == tokenCount - 1, "Not all tokens have been consumed"))
    {
      XII_TEST_BOOL_MSG(tokens[tokenIndex].m_iType == xiiTokenType::EndOfFile, "Last token must be end of file token");
    }

    XII_TEST_BOOL_MSG(expectedIndex == expectedCount, "Not all expected values have been consumed");
  }
} // namespace

XII_CREATE_SIMPLE_TEST(CodeUtils, Tokenizer)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Token Types")
  {
    const char*  stringLiteral = R"(
float f=10.3f + 100'000.0;
int i=100'000*12345;
// line comment
/*
block comment
*/
char c='f';
const char* bla =  "blup";
)";
    xiiTokenizer tokenizer(xiiFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(stringLiteral), xiiStringUtils::GetStringElementCount(stringLiteral)), xiiLog::GetThreadLocalLogSystem());

    xiiDynamicArray<ExpectedToken> expectedResult;
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::Identifier, "float"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Identifier, "f"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::Float, "10.3f"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "+"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Float, "100'000.0"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::Identifier, "int"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Identifier, "i"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::Integer, "100'000"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({xiiTokenType::Integer, "12345"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::LineComment, "// line comment"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::BlockComment, "/*\nblock comment\n*/"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::Identifier, "char"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Identifier, "c"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::String2, "'f'"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    expectedResult.PushBack({xiiTokenType::Identifier, "const"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Identifier, "char"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::Identifier, "bla"});
    expectedResult.PushBack({xiiTokenType::Whitespace, " "});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::Whitespace, "  "});
    expectedResult.PushBack({xiiTokenType::String1, "\"blup\""});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({xiiTokenType::Newline, "\n"});

    CompareResults(expectedResult, tokenizer, false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Raw string literal")
  {
    const char* stringLiteral = R"token(
const char* test = R"(
eins
zwei)";
const char* test2 = R"foo(
vier,
fuenf
)foo";
)token";

    xiiTokenizer tokenizer(xiiFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(stringLiteral), xiiStringUtils::GetStringElementCount(stringLiteral)), xiiLog::GetThreadLocalLogSystem());

    xiiDynamicArray<ExpectedToken> expectedResult;
    expectedResult.PushBack({xiiTokenType::Identifier, "const"});
    expectedResult.PushBack({xiiTokenType::Identifier, "char"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({xiiTokenType::Identifier, "test"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::RawString1Prefix, "R\"("});
    expectedResult.PushBack({xiiTokenType::RawString1, "\neins\nzwei"});
    expectedResult.PushBack({xiiTokenType::RawString1Postfix, ")\""});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({xiiTokenType::Identifier, "const"});
    expectedResult.PushBack({xiiTokenType::Identifier, "char"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({xiiTokenType::Identifier, "test2"});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, "="});
    expectedResult.PushBack({xiiTokenType::RawString1Prefix, "R\"foo("});
    expectedResult.PushBack({xiiTokenType::RawString1, "\nvier,\nfuenf\n"});
    expectedResult.PushBack({xiiTokenType::RawString1Postfix, ")foo\""});
    expectedResult.PushBack({xiiTokenType::NonIdentifier, ";"});

    CompareResults(expectedResult, tokenizer, true);
  }
}
