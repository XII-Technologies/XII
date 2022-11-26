#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace xiiTokenParseUtils
{
  using TokenStream = xiiHybridArray<const xiiToken*, 32>;

  XII_FOUNDATION_DLL void SkipWhitespace(const TokenStream& Tokens, xiiUInt32& uiCurToken);
  XII_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& Tokens, xiiUInt32& uiCurToken);
  XII_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& Tokens, xiiUInt32 uiCurToken, bool bIgnoreWhitespace);
  XII_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& Source, xiiUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines);

  XII_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiTokenType::Enum Type, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool AcceptUnless(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted = nullptr);

  XII_FOUNDATION_DLL void CombineTokensToString(const TokenStream& Tokens, xiiUInt32 uiCurToken, xiiStringBuilder& sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  XII_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& Tokens, xiiUInt32 uiCurToken, xiiStringBuilder& sResult);
  XII_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& Tokens, xiiUInt32 uiCurToken, TokenStream& Destination, bool bKeepComments);
} // namespace xiiTokenParseUtils
