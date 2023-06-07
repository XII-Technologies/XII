#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace xiiTokenParseUtils
{
  using TokenStream = xiiHybridArray<const xiiToken*, 32>;

  XII_FOUNDATION_DLL void SkipWhitespace(const TokenStream& tokens, xiiUInt32& ref_uiCurToken);
  XII_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& tokens, xiiUInt32& ref_uiCurToken);
  XII_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& tokens, xiiUInt32 uiCurToken, bool bIgnoreWhitespace);
  XII_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& source, xiiUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines);

  XII_FOUNDATION_DLL bool Accept(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, const char* szToken, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool Accept(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, xiiTokenType::Enum type, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool Accept(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted = nullptr);
  XII_FOUNDATION_DLL bool AcceptUnless(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted = nullptr);

  XII_FOUNDATION_DLL void CombineTokensToString(const TokenStream& tokens, xiiUInt32 uiCurToken, xiiStringBuilder& ref_sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  XII_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& tokens, xiiUInt32 uiCurToken, xiiStringBuilder& ref_sResult);
  XII_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& tokens, xiiUInt32 uiCurToken, TokenStream& ref_destination, bool bKeepComments);
} // namespace xiiTokenParseUtils
