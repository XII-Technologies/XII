#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace xiiTokenParseUtils;

void xiiPreprocessor::CopyTokensReplaceParams(const TokenStream& Source, xiiUInt32 uiFirstSourceToken, TokenStream& Destination, const xiiArrayPtr<xiiString>& parameters)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    xiiUInt32 i = uiFirstSourceToken;
    SkipWhitespace(Source, i);

    // add all the relevant tokens to the definition
    for (; i < Source.GetCount(); ++i)
    {
      if (Source[i]->m_iType == xiiTokenType::BlockComment || Source[i]->m_iType == xiiTokenType::LineComment || Source[i]->m_iType == xiiTokenType::EndOfFile || Source[i]->m_iType == xiiTokenType::Newline)
        continue;

      if (Source[i]->m_iType == xiiTokenType::Identifier)
      {
        for (xiiUInt32 p = 0; p < parameters.GetCount(); ++p)
        {
          if (Source[i]->m_DataView == parameters[p])
          {
            // create a custom token for the parameter, for better error messages
            xiiToken* pParamToken = AddCustomToken(Source[i], parameters[p]);
            pParamToken->m_iType  = s_iMacroParameter0 + p;

            Destination.PushBack(pParamToken);
            goto tokenfound;
          }
        }
      }

      Destination.PushBack(Source[i]);

    tokenfound:
      continue;
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == xiiTokenType::Whitespace)
    Destination.PopBack();
}

xiiResult xiiPreprocessor::ExtractParameterName(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiString& sIdentifierName)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 2 < Tokens.GetCount() && Tokens[uiCurToken + 0]->m_DataView == "." && Tokens[uiCurToken + 1]->m_DataView == "." && Tokens[uiCurToken + 2]->m_DataView == ".")
  {
    sIdentifierName = "...";
    uiCurToken += 3;
  }
  else
  {
    xiiUInt32 uiParamToken = uiCurToken;

    if (Expect(Tokens, uiCurToken, xiiTokenType::Identifier, &uiParamToken).Failed())
      return XII_FAILURE;

    sIdentifierName = Tokens[uiParamToken]->m_DataView;
  }

  // skip a trailing comma
  if (Accept(Tokens, uiCurToken, ","))
    SkipWhitespace(Tokens, uiCurToken);

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::ExtractAllMacroParameters(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiDeque<TokenStream>& AllParameters)
{
  if (Expect(Tokens, uiCurToken, "(").Failed())
    return XII_FAILURE;

  do
  {
    // add one parameter
    // note: we always add one extra parameter value, because MACRO() is actually a macro call with one empty parameter
    // the same for MACRO(a,) is a macro with two parameters, the second being empty
    AllParameters.SetCount(AllParameters.GetCount() + 1);

    if (ExtractParameterValue(Tokens, uiCurToken, AllParameters.PeekBack()).Failed())
      return XII_FAILURE;

    // reached the end of the parameter list
    if (Accept(Tokens, uiCurToken, ")"))
      return XII_SUCCESS;
  } while (Accept(Tokens, uiCurToken, ",")); // continue with the next parameter

  xiiString s = Tokens[uiCurToken]->m_DataView;
  PP_LOG(Error, "',' or ')' expected, got '{0}' instead", Tokens[uiCurToken], s);

  return XII_FAILURE;
}

xiiResult xiiPreprocessor::ExtractParameterValue(const TokenStream& Tokens, xiiUInt32& uiCurToken, TokenStream& ParamTokens)
{
  SkipWhitespaceAndNewline(Tokens, uiCurToken);
  const xiiUInt32 uiFirstToken = xiiMath::Min(uiCurToken, Tokens.GetCount() - 1);

  xiiInt32 iParenthesis = 0;

  // get all tokens up until a comma or the last closing parenthesis
  // ignore commas etc. as long as they are surrounded with parenthesis
  for (; uiCurToken < Tokens.GetCount(); ++uiCurToken)
  {
    if (Tokens[uiCurToken]->m_iType == xiiTokenType::BlockComment || Tokens[uiCurToken]->m_iType == xiiTokenType::LineComment || Tokens[uiCurToken]->m_iType == xiiTokenType::Newline)
      continue;

    if (Tokens[uiCurToken]->m_iType == xiiTokenType::EndOfFile)
      break; // outputs an error

    if (iParenthesis == 0)
    {
      if (Tokens[uiCurToken]->m_DataView == "," || Tokens[uiCurToken]->m_DataView == ")")
      {
        if (!ParamTokens.IsEmpty() && ParamTokens.PeekBack()->m_iType == xiiTokenType::Whitespace)
        {
          ParamTokens.PopBack();
        }
        return XII_SUCCESS;
      }
    }

    if (Tokens[uiCurToken]->m_DataView == "(")
      ++iParenthesis;
    else if (Tokens[uiCurToken]->m_DataView == ")")
      --iParenthesis;

    ParamTokens.PushBack(Tokens[uiCurToken]);
  }

  // reached the end of the stream without encountering the closing parenthesis first
  PP_LOG0(Error, "Unexpected end of file during macro parameter extraction", Tokens[uiFirstToken]);
  return XII_FAILURE;
}

void xiiPreprocessor::StringifyTokens(const TokenStream& Tokens, xiiStringBuilder& sResult, bool bSurroundWithQuotes)
{
  xiiUInt32 uiCurToken = 0;

  sResult.Clear();

  if (bSurroundWithQuotes)
    sResult = "\"";

  xiiStringBuilder sTemp;

  SkipWhitespace(Tokens, uiCurToken);

  xiiUInt32 uiLastNonWhitespace = Tokens.GetCount();

  while (uiLastNonWhitespace > 0)
  {
    if (Tokens[uiLastNonWhitespace - 1]->m_iType != xiiTokenType::Whitespace && Tokens[uiLastNonWhitespace - 1]->m_iType != xiiTokenType::Newline && Tokens[uiLastNonWhitespace - 1]->m_iType != xiiTokenType::BlockComment && Tokens[uiLastNonWhitespace - 1]->m_iType != xiiTokenType::LineComment)
      break;

    --uiLastNonWhitespace;
  }

  for (xiiUInt32 t = uiCurToken; t < uiLastNonWhitespace; ++t)
  {
    // comments, newlines etc. are stripped out
    if ((Tokens[t]->m_iType == xiiTokenType::LineComment) || (Tokens[t]->m_iType == xiiTokenType::BlockComment) || (Tokens[t]->m_iType == xiiTokenType::Newline) || (Tokens[t]->m_iType == xiiTokenType::EndOfFile))
      continue;

    sTemp = Tokens[t]->m_DataView;

    // all whitespace becomes a single white space
    if (Tokens[t]->m_iType == xiiTokenType::Whitespace)
      sTemp = " ";

    // inside strings, all backslashes and double quotes are escaped
    if ((Tokens[t]->m_iType == xiiTokenType::String1) || (Tokens[t]->m_iType == xiiTokenType::String2))
    {
      sTemp.ReplaceAll("\\", "\\\\");
      sTemp.ReplaceAll("\"", "\\\"");
    }

    sResult.Append(sTemp.GetView());
  }

  if (bSurroundWithQuotes)
    sResult.Append("\"");
}



XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Macros);
