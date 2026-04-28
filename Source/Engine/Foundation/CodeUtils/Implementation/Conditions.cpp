/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace xiiTokenParseUtils;

xiiResult xiiPreprocessor::CopyTokensAndEvaluateDefined(const TokenStream& Source, xiiUInt32 uiFirstSourceToken, TokenStream& Destination)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    xiiUInt32 uiCurToken = uiFirstSourceToken;
    SkipWhitespace(Source, uiCurToken);

    // add all the relevant tokens to the definition
    while (uiCurToken < Source.GetCount())
    {
      if (Source[uiCurToken]->m_iType == xiiTokenType::BlockComment || Source[uiCurToken]->m_iType == xiiTokenType::LineComment || Source[uiCurToken]->m_iType == xiiTokenType::EndOfFile || Source[uiCurToken]->m_iType == xiiTokenType::Newline)
      {
        ++uiCurToken;
        continue;
      }

      if (Source[uiCurToken]->m_DataView.IsEqual("defined"))
      {
        ++uiCurToken;

        const bool bParenthesis = Accept(Source, uiCurToken, "(");

        xiiUInt32 uiIdentifier = uiCurToken;
        if (Expect(Source, uiCurToken, xiiTokenType::Identifier, &uiIdentifier).Failed())
          return XII_FAILURE;

        xiiToken* pReplacement = nullptr;

        const bool bDefined = m_Macros.Find(Source[uiIdentifier]->m_DataView).IsValid();

        // broadcast that 'defined' is being evaluated
        {
          ProcessingEvent pe;
          pe.m_pToken = Source[uiIdentifier];
          pe.m_Type   = ProcessingEvent::CheckDefined;
          pe.m_sInfo  = bDefined ? "defined" : "undefined";
          m_ProcessingEvents.Broadcast(pe);
        }

        pReplacement = AddCustomToken(Source[uiIdentifier], bDefined ? "1" : "0");

        Destination.PushBack(pReplacement);

        if (bParenthesis)
        {
          if (Expect(Source, uiCurToken, ")").Failed())
            return XII_FAILURE;
        }
      }
      else
      {
        Destination.PushBack(Source[uiCurToken]);
        ++uiCurToken;
      }
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == xiiTokenType::Whitespace)
    Destination.PopBack();

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::EvaluateCondition(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  iResult = 0;

  TokenStream Copied(&m_ClassAllocator);
  if (CopyTokensAndEvaluateDefined(Tokens, uiCurToken, Copied).Failed())
    return XII_FAILURE;

  TokenStream Expanded(&m_ClassAllocator);

  if (Expand(Copied, Expanded).Failed())
    return XII_FAILURE;

  if (Expanded.IsEmpty())
  {
    PP_LOG0(Error, "After expansion the condition is empty", Tokens[uiCurToken]);
    return XII_FAILURE;
  }

  xiiUInt32 uiCurToken2 = 0;
  if (ParseExpressionOr(Expanded, uiCurToken2, iResult).Failed())
    return XII_FAILURE;

  return ExpectEndOfLine(Expanded, uiCurToken2);
}

xiiResult xiiPreprocessor::ParseFactor(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  while (Accept(Tokens, uiCurToken, "+"))
  {
  }

  if (Accept(Tokens, uiCurToken, "-"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return XII_FAILURE;

    iResult = -iResult;
    return XII_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "~"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return XII_FAILURE;

    iResult = ~iResult;
    return XII_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "!"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return XII_FAILURE;

    iResult = (iResult != 0) ? 0 : 1;
    return XII_SUCCESS;
  }

  xiiUInt32 uiValueToken = uiCurToken;
  if (Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiValueToken) || Accept(Tokens, uiCurToken, xiiTokenType::Integer, &uiValueToken))
  {
    const xiiString sVal = Tokens[uiValueToken]->m_DataView;

    xiiInt32 iResult32 = 0;

    if (sVal == "true")
    {
      iResult32 = 1;
    }
    else if (sVal == "false")
    {
      iResult32 = 0;
    }
    else if (xiiConversionUtils::StringToInt(sVal, iResult32).Failed())
    {
      // this is not an error, all unknown identifiers are assumed to be zero

      // broadcast that we encountered this unknown identifier
      ProcessingEvent pe;
      pe.m_pToken = Tokens[uiValueToken];
      pe.m_Type   = ProcessingEvent::EvaluateUnknown;
      m_ProcessingEvents.Broadcast(pe);
    }

    iResult = (xiiInt64)iResult32;

    return XII_SUCCESS;
  }
  else if (Accept(Tokens, uiCurToken, "("))
  {
    if (ParseExpressionOr(Tokens, uiCurToken, iResult).Failed())
      return XII_FAILURE;

    return Expect(Tokens, uiCurToken, ")");
  }

  uiCurToken = xiiMath::Min(uiCurToken, Tokens.GetCount() - 1);
  PP_LOG0(Error, "Syntax error, expected identifier, number or '('", Tokens[uiCurToken]);

  return XII_FAILURE;
}

xiiResult xiiPreprocessor::ParseExpressionPlus(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionMul(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "+"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult += iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "-"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult -= iNextValue;
    }
    else
      break;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionShift(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionPlus(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, ">", ">"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult >>= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "<", "<"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult <<= iNextValue;
    }
    else
      break;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionOr(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionAnd(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (Accept(Tokens, uiCurToken, "|", "|"))
  {
    xiiInt64 iNextValue = 0;
    if (ParseExpressionAnd(Tokens, uiCurToken, iNextValue).Failed())
      return XII_FAILURE;

    iResult = (iResult != 0 || iNextValue != 0) ? 1 : 0;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionAnd(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionBitOr(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (Accept(Tokens, uiCurToken, "&", "&"))
  {
    xiiInt64 iNextValue = 0;
    if (ParseExpressionBitOr(Tokens, uiCurToken, iNextValue).Failed())
      return XII_FAILURE;

    iResult = (iResult != 0 && iNextValue != 0) ? 1 : 0;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionBitOr(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionBitXor(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "|", "|"))
  {
    xiiInt64 iNextValue = 0;
    if (ParseExpressionBitXor(Tokens, uiCurToken, iNextValue).Failed())
      return XII_FAILURE;

    iResult |= iNextValue;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionBitAnd(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseCondition(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "&", "&"))
  {
    xiiInt64 iNextValue = 0;
    if (ParseCondition(Tokens, uiCurToken, iNextValue).Failed())
      return XII_FAILURE;

    iResult &= iNextValue;
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ParseExpressionBitXor(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseExpressionBitAnd(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (Accept(Tokens, uiCurToken, "^"))
  {
    xiiInt64 iNextValue = 0;
    if (ParseExpressionBitAnd(Tokens, uiCurToken, iNextValue).Failed())
      return XII_FAILURE;

    iResult ^= iNextValue;
  }

  return XII_SUCCESS;
}
xiiResult xiiPreprocessor::ParseExpressionMul(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
    return XII_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "*"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult *= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "/"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult /= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "%"))
    {
      xiiInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return XII_FAILURE;

      iResult %= iNextValue;
    }
    else
      break;
  }

  return XII_SUCCESS;
}

enum class Comparison
{
  None,
  Equal,
  Unequal,
  LessThan,
  GreaterThan,
  LessThanEqual,
  GreaterThanEqual
};

xiiResult xiiPreprocessor::ParseCondition(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult)
{
  xiiInt64 iResult1 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult1).Failed())
    return XII_FAILURE;

  Comparison Operator = Comparison::None;

  if (Accept(Tokens, uiCurToken, "=", "="))
    Operator = Comparison::Equal;
  else if (Accept(Tokens, uiCurToken, "!", "="))
    Operator = Comparison::Unequal;
  else if (Accept(Tokens, uiCurToken, ">", "="))
    Operator = Comparison::GreaterThanEqual;
  else if (Accept(Tokens, uiCurToken, "<", "="))
    Operator = Comparison::LessThanEqual;
  else if (AcceptUnless(Tokens, uiCurToken, ">", ">"))
    Operator = Comparison::GreaterThan;
  else if (AcceptUnless(Tokens, uiCurToken, "<", "<"))
    Operator = Comparison::LessThan;
  else
  {
    iResult = iResult1;
    return XII_SUCCESS;
  }

  xiiInt64 iResult2 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult2).Failed())
    return XII_FAILURE;

  switch (Operator)
  {
    case Comparison::Equal:
      iResult = (iResult1 == iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::GreaterThan:
      iResult = (iResult1 > iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::GreaterThanEqual:
      iResult = (iResult1 >= iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::LessThan:
      iResult = (iResult1 < iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::LessThanEqual:
      iResult = (iResult1 <= iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::Unequal:
      iResult = (iResult1 != iResult2) ? 1 : 0;
      return XII_SUCCESS;
    case Comparison::None:
      xiiLog::Error(m_pLog, "Unknown operator");
      return XII_FAILURE;
  }

  return XII_FAILURE;
}



XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Conditions);
