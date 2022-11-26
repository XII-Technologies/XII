#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace xiiTokenParseUtils
{
  void SkipWhitespace(const TokenStream& Tokens, xiiUInt32& uiCurToken)
  {
    while (uiCurToken < Tokens.GetCount() && ((Tokens[uiCurToken]->m_iType == xiiTokenType::Whitespace) || (Tokens[uiCurToken]->m_iType == xiiTokenType::BlockComment) || (Tokens[uiCurToken]->m_iType == xiiTokenType::LineComment)))
      ++uiCurToken;
  }

  void SkipWhitespaceAndNewline(const TokenStream& Tokens, xiiUInt32& uiCurToken)
  {
    while (uiCurToken < Tokens.GetCount() && ((Tokens[uiCurToken]->m_iType == xiiTokenType::Whitespace) || (Tokens[uiCurToken]->m_iType == xiiTokenType::BlockComment) || (Tokens[uiCurToken]->m_iType == xiiTokenType::Newline) || (Tokens[uiCurToken]->m_iType == xiiTokenType::LineComment)))
      ++uiCurToken;
  }

  bool IsEndOfLine(const TokenStream& Tokens, xiiUInt32 uiCurToken, bool bIgnoreWhitespace)
  {
    if (bIgnoreWhitespace)
      SkipWhitespace(Tokens, uiCurToken);

    if (uiCurToken >= Tokens.GetCount())
      return true;

    return Tokens[uiCurToken]->m_iType == xiiTokenType::Newline || Tokens[uiCurToken]->m_iType == xiiTokenType::EndOfFile;
  }

  void CopyRelevantTokens(const TokenStream& Source, xiiUInt32 uiFirstSourceToken, TokenStream& Destination, bool bPreserveNewLines)
  {
    Destination.Reserve(Destination.GetCount() + Source.GetCount() - uiFirstSourceToken);

    {
      // skip all whitespace at the start of the replacement string
      xiiUInt32 i = uiFirstSourceToken;
      SkipWhitespace(Source, i);

      // add all the relevant tokens to the definition
      for (; i < Source.GetCount(); ++i)
      {
        if (Source[i]->m_iType == xiiTokenType::BlockComment || Source[i]->m_iType == xiiTokenType::LineComment || Source[i]->m_iType == xiiTokenType::EndOfFile || (!bPreserveNewLines && Source[i]->m_iType == xiiTokenType::Newline))
          continue;

        Destination.PushBack(Source[i]);
      }
    }

    // remove whitespace at end of macro
    while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == xiiTokenType::Whitespace)
      Destination.PopBack();
  }

  bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken, xiiUInt32* pAccepted)
  {
    SkipWhitespace(Tokens, uiCurToken);

    if (uiCurToken >= Tokens.GetCount())
      return false;

    if (Tokens[uiCurToken]->m_DataView == szToken)
    {
      if (pAccepted)
        *pAccepted = uiCurToken;

      uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiTokenType::Enum Type, xiiUInt32* pAccepted)
  {
    SkipWhitespace(Tokens, uiCurToken);

    if (uiCurToken >= Tokens.GetCount())
      return false;

    if (Tokens[uiCurToken]->m_iType == Type)
    {
      if (pAccepted)
        *pAccepted = uiCurToken;

      uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted)
  {
    SkipWhitespace(Tokens, uiCurToken);

    if (uiCurToken + 1 >= Tokens.GetCount())
      return false;

    if (Tokens[uiCurToken]->m_DataView == szToken1 && Tokens[uiCurToken + 1]->m_DataView == szToken2)
    {
      if (pAccepted)
        *pAccepted = uiCurToken;

      uiCurToken += 2;
      return true;
    }

    return false;
  }

  bool AcceptUnless(const TokenStream& Tokens, xiiUInt32& uiCurToken, const char* szToken1, const char* szToken2, xiiUInt32* pAccepted)
  {
    SkipWhitespace(Tokens, uiCurToken);

    if (uiCurToken + 1 >= Tokens.GetCount())
      return false;

    if (Tokens[uiCurToken]->m_DataView == szToken1 && Tokens[uiCurToken + 1]->m_DataView != szToken2)
    {
      if (pAccepted)
        *pAccepted = uiCurToken;

      uiCurToken += 1;
      return true;
    }

    return false;
  }

  void CombineRelevantTokensToString(const TokenStream& Tokens, xiiUInt32 uiCurToken, xiiStringBuilder& sResult)
  {
    sResult.Clear();
    xiiStringBuilder sTemp;

    for (xiiUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      if ((Tokens[t]->m_iType == xiiTokenType::LineComment) || (Tokens[t]->m_iType == xiiTokenType::BlockComment) || (Tokens[t]->m_iType == xiiTokenType::Newline) || (Tokens[t]->m_iType == xiiTokenType::EndOfFile))
        continue;

      sTemp = Tokens[t]->m_DataView;
      sResult.Append(sTemp.GetView());
    }
  }

  void CreateCleanTokenStream(const TokenStream& Tokens, xiiUInt32 uiCurToken, TokenStream& Destination, bool bKeepComments)
  {
    SkipWhitespace(Tokens, uiCurToken);

    for (xiiUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      if (Tokens[t]->m_iType == xiiTokenType::Newline)
      {
        // remove all whitespace before a newline
        while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == xiiTokenType::Whitespace)
          Destination.PopBack();

        // if there is already a newline stored, discard the new one
        if (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == xiiTokenType::Newline)
          continue;
      }

      Destination.PushBack(Tokens[t]);
    }
  }

  void CombineTokensToString(const TokenStream& Tokens0, xiiUInt32 uiCurToken, xiiStringBuilder& sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
  {
    TokenStream Tokens;

    if (bRemoveRedundantWhitespace)
    {
      CreateCleanTokenStream(Tokens0, uiCurToken, Tokens, bKeepComments);
      uiCurToken = 0;
    }
    else
      Tokens = Tokens0;

    sResult.Clear();
    xiiStringBuilder sTemp;

    xiiUInt32       uiCurLine = 0xFFFFFFFF;
    xiiHashedString sCurFile;

    for (xiiUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      // skip all comments, if not desired
      if ((Tokens[t]->m_iType == xiiTokenType::BlockComment || Tokens[t]->m_iType == xiiTokenType::LineComment) && !bKeepComments)
        continue;

      if (Tokens[t]->m_iType == xiiTokenType::EndOfFile)
        return;

      if (bInsertLine)
      {
        if (sResult.IsEmpty())
        {
          sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile  = Tokens[t]->m_File;
        }

        if (Tokens[t]->m_iType == xiiTokenType::Newline)
        {
          ++uiCurLine;
        }

        if (t > 0 && Tokens[t - 1]->m_iType == xiiTokenType::Newline)
        {
          if (Tokens[t]->m_uiLine != uiCurLine || Tokens[t]->m_File != sCurFile)
          {
            sResult.AppendFormat("\n#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
            uiCurLine = Tokens[t]->m_uiLine;
            sCurFile  = Tokens[t]->m_File;
          }
        }
      }

      sTemp = Tokens[t]->m_DataView;
      sResult.Append(sTemp.GetView());
    }
  }
} // namespace xiiTokenParseUtils

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_TokenParseUtils);
