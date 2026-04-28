/// Copyright (c) Theophilus Eriata. All Rights Reserved.

inline bool xiiExpressionParser::AcceptStatementTerminator()
{
  return xiiTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Newline) ||
    xiiTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, ";");
}

inline xiiResult xiiExpressionParser::Expect(xiiStringView sToken, const xiiToken** pExpectedToken)
{
  xiiUInt32 uiAcceptedToken = 0;
  if (xiiTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, sToken, &uiAcceptedToken) == false)
  {
    const xiiUInt32 uiErrorToken = xiiMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto            pToken       = m_TokenStream[uiErrorToken];
    ReportError(pToken, xiiFmt("Syntax error, expected {} but got {}", sToken, pToken->m_DataView));
    return XII_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return XII_SUCCESS;
}

inline xiiResult xiiExpressionParser::Expect(xiiTokenType::Enum Type, const xiiToken** pExpectedToken /*= nullptr*/)
{
  xiiUInt32 uiAcceptedToken = 0;
  if (xiiTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, Type, &uiAcceptedToken) == false)
  {
    const xiiUInt32 uiErrorToken = xiiMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto            pToken       = m_TokenStream[uiErrorToken];
    ReportError(pToken, xiiFmt("Syntax error, expected token type {} but got {}", xiiTokenType::EnumNames[Type], xiiTokenType::EnumNames[pToken->m_iType]));
    return XII_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return XII_SUCCESS;
}

inline void xiiExpressionParser::ReportError(const xiiToken* pToken, const xiiFormatString& message0)
{
  xiiStringBuilder tmp;
  xiiStringView    message = message0.GetText(tmp);
  xiiLog::Error("{}({},{}): {}", pToken->m_File, pToken->m_uiLine, pToken->m_uiColumn, message);
}
