#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace xiiTokenParseUtils;

xiiResult xiiPreprocessor::Expect(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiStringView sToken, xiiUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    xiiLog::Error(m_pLog, "Expected token '{0}', got empty token stream", sToken);
    return XII_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken, pAccepted))
    return XII_SUCCESS;

  const xiiUInt32 uiErrorToken = xiiMath::Min(Tokens.GetCount() - 1, uiCurToken);
  xiiString       sErrorToken  = Tokens[uiErrorToken]->m_DataView;
  PP_LOG(Error, "Expected token '{0}' got '{1}'", Tokens[uiErrorToken], sToken, sErrorToken);

  return XII_FAILURE;
}

xiiResult xiiPreprocessor::Expect(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiTokenType::Enum Type, xiiUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    xiiLog::Error(m_pLog, "Expected token of type '{0}', got empty token stream", xiiTokenType::EnumNames[Type]);
    return XII_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, Type, pAccepted))
    return XII_SUCCESS;

  const xiiUInt32 uiErrorToken = xiiMath::Min(Tokens.GetCount() - 1, uiCurToken);
  PP_LOG(Error, "Expected token of type '{0}' got type '{1}' instead", Tokens[uiErrorToken], xiiTokenType::EnumNames[Type], xiiTokenType::EnumNames[Tokens[uiErrorToken]->m_iType]);

  return XII_FAILURE;
}

xiiResult xiiPreprocessor::Expect(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiStringView sToken1, xiiStringView sToken2, xiiUInt32* pAccepted)
{
  if (Tokens.GetCount() < 2)
  {
    xiiLog::Error(m_pLog, "Expected tokens '{0}{1}', got empty token stream", sToken1, sToken2);
    return XII_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken1, sToken2, pAccepted))
    return XII_SUCCESS;

  const xiiUInt32 uiErrorToken = xiiMath::Min(Tokens.GetCount() - 2, uiCurToken);
  xiiString       sErrorToken1 = Tokens[uiErrorToken]->m_DataView;
  xiiString       sErrorToken2 = Tokens[uiErrorToken + 1]->m_DataView;
  PP_LOG(Error, "Expected tokens '{0}{1}', got '{2}{3}'", Tokens[uiErrorToken], sToken1, sToken2, sErrorToken1, sErrorToken2);

  return XII_FAILURE;
}

xiiResult xiiPreprocessor::ExpectEndOfLine(const TokenStream& Tokens, xiiUInt32 uiCurToken)
{
  if (!IsEndOfLine(Tokens, uiCurToken, true))
  {
    xiiString sToken = Tokens[uiCurToken]->m_DataView;
    PP_LOG(Warning, "Expected end-of-line, found token '{0}'", Tokens[uiCurToken], sToken);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_PreprocessorParseHelper);
