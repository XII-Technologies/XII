/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

XII_CREATE_SIMPLE_TEST(CodeUtils, TokenParseUtils)
{
  const char* stringLiteral = R"(
// Some comment
/* A block comment
Some block
*/
Identifier
)";

  xiiTokenizer tokenizer(xiiFoundation::GetDefaultAllocator());
  tokenizer.Tokenize(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(stringLiteral), xiiStringUtils::GetStringElementCount(stringLiteral)), xiiLog::GetThreadLocalLogSystem(), false);

  xiiTokenParseUtils::TokenStream tokens;
  tokenizer.GetAllTokens(tokens);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipWhitespace / IsEndOfLine")
  {
    xiiUInt32 uiCurToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    XII_TEST_BOOL(!xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    xiiTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    XII_TEST_INT(uiCurToken, 2);
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    XII_TEST_BOOL(!xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    xiiTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    XII_TEST_INT(uiCurToken, 4);
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    XII_TEST_BOOL(!xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    XII_TEST_BOOL(!xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    XII_TEST_INT(tokens[uiCurToken]->m_iType, xiiTokenType::Identifier);
    uiCurToken++;
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    XII_TEST_INT(tokens[uiCurToken]->m_iType, xiiTokenType::EndOfFile);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipWhitespaceAndNewline")
  {
    xiiUInt32 uiCurToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    xiiTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    XII_TEST_INT(uiCurToken, 5);
    XII_TEST_INT(tokens[uiCurToken]->m_iType, xiiTokenType::Identifier);
    uiCurToken++;
    XII_TEST_BOOL(xiiTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    xiiTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    XII_TEST_INT(tokens[uiCurToken]->m_iType, xiiTokenType::EndOfFile);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CopyRelevantTokens")
  {
    xiiUInt32                       uiCurToken = 0;
    xiiTokenParseUtils::TokenStream relevantTokens;
    xiiTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, true);

    XII_TEST_INT(relevantTokens.GetCount(), 5);
    for (xiiUInt32 i = 0; i < relevantTokens.GetCount(); ++i)
    {
      if (i == 3)
      {
        XII_TEST_INT(relevantTokens[i]->m_iType, xiiTokenType::Identifier);
      }
      else
      {
        XII_TEST_INT(relevantTokens[i]->m_iType, xiiTokenType::Newline);
      }
    }

    relevantTokens.Clear();
    xiiTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, false);
    XII_TEST_INT(relevantTokens.GetCount(), 1);
    XII_TEST_INT(relevantTokens[0]->m_iType, xiiTokenType::Identifier);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Accept")
  {
    xiiUInt32 uiCurToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, "\n"_xiisv, nullptr));
    XII_TEST_INT(uiCurToken, 1);
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, xiiTokenType::Newline, nullptr));
    XII_TEST_INT(uiCurToken, 3);
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, "\n"_xiisv, nullptr));
    XII_TEST_INT(uiCurToken, 5);

    xiiUInt32 uiIdentifierToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, xiiTokenType::Identifier, &uiIdentifierToken));
    XII_TEST_INT(uiIdentifierToken, 5);
    XII_TEST_INT(uiCurToken, 6);

    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, xiiTokenType::Newline, nullptr));

    XII_TEST_BOOL(!xiiTokenParseUtils::Accept(tokens, uiCurToken, xiiTokenType::Newline, nullptr));
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, xiiTokenType::EndOfFile, nullptr));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Accept2")
  {
    xiiUInt32 uiCurToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, "\n"_xiisv, "// Some comment"_xiisv, nullptr));
    XII_TEST_INT(uiCurToken, 2);
    xiiUInt32 uiTouple1Token = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, "\n"_xiisv, "/* A block comment\nSome block\n*/"_xiisv, &uiTouple1Token));
    XII_TEST_INT(uiTouple1Token, 2);
    XII_TEST_INT(uiCurToken, 4);
    XII_TEST_BOOL(!xiiTokenParseUtils::AcceptUnless(tokens, uiCurToken, "\n"_xiisv, "Identifier"_xiisv, nullptr));
    uiCurToken++;
    xiiUInt32 uiIdentifierToken = 0;
    XII_TEST_BOOL(xiiTokenParseUtils::AcceptUnless(tokens, uiCurToken, "Identifier"_xiisv, "ScaryString"_xiisv, &uiIdentifierToken));
    XII_TEST_INT(uiIdentifierToken, 5);
    XII_TEST_INT(uiCurToken, 6);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Accept3")
  {
    xiiUInt32                      uiCurToken        = 0;
    xiiTokenParseUtils::TokenMatch templatePattern[] = {xiiTokenType::Newline, xiiTokenType::Newline, "Identifier"_xiisv};
    xiiHybridArray<xiiUInt32, 8>   acceptedTokens;
    XII_TEST_BOOL(!xiiTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));
    uiCurToken++;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));

    XII_TEST_INT(acceptedTokens.GetCount(), XII_ARRAY_SIZE(templatePattern));
    XII_TEST_INT(acceptedTokens[0], 2);
    XII_TEST_INT(acceptedTokens[1], 4);
    XII_TEST_INT(acceptedTokens[2], 5);
    XII_TEST_INT(uiCurToken, 6);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Accept4")
  {
    const char* vectorString = "Vec2(2.2, 1.1)";

    xiiTokenizer tokenizer2(xiiFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(vectorString), xiiStringUtils::GetStringElementCount(vectorString)), xiiLog::GetThreadLocalLogSystem(), false);

    xiiTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    xiiUInt32                      uiCurToken        = 0;
    xiiTokenParseUtils::TokenMatch templatePattern[] = {"Vec2"_xiisv, "("_xiisv, xiiTokenType::Float, ","_xiisv, xiiTokenType::Float, ")"_xiisv};
    xiiHybridArray<xiiUInt32, 6>   acceptedTokens;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(tokens2, uiCurToken, templatePattern, &acceptedTokens));
    XII_TEST_INT(uiCurToken, 7);
    XII_TEST_INT(acceptedTokens.GetCount(), XII_ARRAY_SIZE(templatePattern));
    XII_TEST_INT(acceptedTokens[2], 2);
    XII_TEST_INT(acceptedTokens[4], 5);
    XII_TEST_STRING(tokens2[acceptedTokens[2]]->m_DataView, "2.2");
    XII_TEST_STRING(tokens2[acceptedTokens[4]]->m_DataView, "1.1");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CombineTokensToString")
  {
    xiiUInt32        uiCurToken = 0;
    xiiStringBuilder sResult;
    xiiTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult);
    XII_TEST_STRING(sResult, stringLiteral);

    xiiTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult, false, true);
    XII_TEST_STRING(sResult, "\n\n\nIdentifier\n");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CombineRelevantTokensToString")
  {
    xiiUInt32        uiCurToken = 0;
    xiiStringBuilder sResult;
    xiiTokenParseUtils::CombineRelevantTokensToString(tokens, uiCurToken, sResult);
    XII_TEST_STRING(sResult, "Identifier");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateCleanTokenStream")
  {
    const char* stringLiteralWithRedundantStuff = "\n\nID1 \nID2";

    xiiTokenizer tokenizer2(xiiFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(stringLiteralWithRedundantStuff), xiiStringUtils::GetStringElementCount(stringLiteralWithRedundantStuff)), xiiLog::GetThreadLocalLogSystem(), false);

    xiiTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    xiiUInt32                       uiCurToken = 0;
    xiiTokenParseUtils::TokenStream result;
    xiiTokenParseUtils::CreateCleanTokenStream(tokens2, uiCurToken, result);

    XII_TEST_INT(result.GetCount(), 5);

    xiiTokenParseUtils::TokenMatch templatePattern[] = {xiiTokenType::Newline, "ID1"_xiisv, xiiTokenType::Newline, "ID2"_xiisv, xiiTokenType::EndOfFile};
    xiiHybridArray<xiiUInt32, 8>   acceptedTokens;
    XII_TEST_BOOL(xiiTokenParseUtils::Accept(result, uiCurToken, templatePattern, nullptr));
    XII_TEST_INT(uiCurToken, 5);
  }
}
