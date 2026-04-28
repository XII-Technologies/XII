/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace xiiTokenParseUtils;

bool xiiPreprocessor::RemoveDefine(xiiStringView sName)
{
  auto it = m_Macros.Find(sName);

  if (it.IsValid())
  {
    m_Macros.Remove(it);
    return true;
  }

  return false;
}


xiiResult xiiPreprocessor::StoreDefine(const xiiToken* pMacroNameToken, const TokenStream* pReplacementTokens, xiiUInt32 uiFirstReplacementToken, xiiInt32 iNumParameters, bool bUsesVarArgs)
{
  if ((pMacroNameToken->m_DataView.IsEqual("defined")) || (pMacroNameToken->m_DataView.IsEqual("__FILE__")) || (pMacroNameToken->m_DataView.IsEqual("__LINE__")))
  {
    PP_LOG(Error, "Macro name '{0}' is reserved", pMacroNameToken, pMacroNameToken->m_DataView);
    return XII_FAILURE;
  }

  MacroDefinition md;
  md.m_MacroIdentifier = pMacroNameToken;
  md.m_bIsFunction     = iNumParameters >= 0;
  md.m_uiNumParameters = xiiMath::Max(0, iNumParameters);
  md.m_bHasVarArgs     = bUsesVarArgs;

  // removes whitespace at start and end, skips comments, newlines, etc.
  if (pReplacementTokens)
    CopyRelevantTokens(*pReplacementTokens, uiFirstReplacementToken, md.m_Replacement, false);

  if (!md.m_Replacement.IsEmpty() && md.m_Replacement.PeekBack()->m_DataView == "#")
  {
    PP_LOG(Error, "Macro '{0}' ends with invalid character '#'", md.m_Replacement.PeekBack(), pMacroNameToken->m_DataView);
    return XII_FAILURE;
  }

  /* make sure all replacements are not empty
  {
    xiiToken Whitespace;
    Whitespace.m_File = pMacroNameToken->m_File;
    Whitespace.m_iType = xiiTokenType::Whitespace;
    Whitespace.m_uiColumn = pMacroNameToken->m_uiColumn + sMacroName.GetCharacterCount() + 1;
    Whitespace.m_uiLine = pMacroNameToken->m_uiLine;

    md.m_Replacement.PushBack(AddCustomToken(&Whitespace, ""));
  }*/

  bool bExisted = false;
  auto it       = m_Macros.FindOrAdd(pMacroNameToken->m_DataView, &bExisted);

  ProcessingEvent pe;
  pe.m_Type   = bExisted ? ProcessingEvent::Redefine : ProcessingEvent::Define;
  pe.m_pToken = pMacroNameToken;
  m_ProcessingEvents.Broadcast(pe);

  if (bExisted)
  {
    PP_LOG(Warning, "Redefinition of macro '{0}'", pMacroNameToken, pMacroNameToken->m_DataView);
    // return XII_FAILURE;
  }

  it.Value() = md;
  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::HandleDefine(const TokenStream& Tokens, xiiUInt32& uiCurToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  xiiUInt32 uiNameToken = uiCurToken;

  if (Expect(Tokens, uiCurToken, xiiTokenType::Identifier, &uiNameToken).Failed())
    return XII_FAILURE;

  // check if we got an empty macro definition
  if (IsEndOfLine(Tokens, uiCurToken, true))
  {
    xiiStringBuilder sDefine = Tokens[uiNameToken]->m_DataView;

    return StoreDefine(Tokens[uiNameToken], nullptr, 0, -1, false);
  }

  // first determine whether this is a function macro (before skipping whitespace)
  if (Tokens[uiCurToken]->m_DataView != "(")
  {
    // add the rest of the macro definition as the replacement
    return StoreDefine(Tokens[uiNameToken], &Tokens, uiCurToken, -1, false);
  }

  // extract parameter names
  {
    bool bVarArgsFounds = false;

    // skip the opening parenthesis (
    if (Expect(Tokens, uiCurToken, "(").Failed())
      return XII_FAILURE;

    xiiTemporaryHybridArray<xiiString, 16> parameters;

    while (!Accept(Tokens, uiCurToken, ")"))
    {
      if (uiCurToken >= Tokens.GetCount())
      {
        PP_LOG(Error, "Could not extract macro parameter {0}, reached end of token stream first", Tokens[Tokens.GetCount() - 1], parameters.GetCount());
        return XII_FAILURE;
      }

      const xiiUInt32 uiCurParamToken = uiCurToken;

      xiiString sParam;
      if (ExtractParameterName(Tokens, uiCurToken, sParam) == XII_FAILURE)
      {
        PP_LOG(Error, "Could not extract macro parameter {0}", Tokens[uiCurParamToken], parameters.GetCount());
        return XII_FAILURE;
      }

      if (bVarArgsFounds)
      {
        PP_LOG0(Error, "No additional parameters are allowed after '...'", Tokens[uiCurParamToken]);
        return XII_FAILURE;
      }

      /// \todo Make sure the same parameter name is not used twice

      if (sParam == "...")
      {
        bVarArgsFounds = true;
        sParam         = "__VA_ARGS__";
      }

      parameters.PushBack(sParam);
    }

    TokenStream ReplacementTokens;
    CopyTokensReplaceParams(Tokens, uiCurToken, ReplacementTokens, parameters);

    XII_SUCCEED_OR_RETURN(StoreDefine(Tokens[uiNameToken], &ReplacementTokens, 0, parameters.GetCount(), bVarArgsFounds));
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::AddCustomDefine(xiiStringView sDefinition)
{
  m_CustomDefines.PushBack();
  m_CustomDefines.PeekBack().m_Content.SetCountUninitialized(sDefinition.GetElementCount());
  xiiMemoryUtils::Copy(&m_CustomDefines.PeekBack().m_Content[0], (xiiUInt8*)sDefinition.GetStartPointer(), m_CustomDefines.PeekBack().m_Content.GetCount());
  m_CustomDefines.PeekBack().m_Tokenized.Tokenize(m_CustomDefines.PeekBack().m_Content, m_pLog);

  xiiUInt32                                    uiFirstToken = 0;
  xiiTemporaryHybridArray<const xiiToken*, 32> Tokens;

  if (m_CustomDefines.PeekBack().m_Tokenized.GetNextLine(uiFirstToken, Tokens).Failed())
    return XII_FAILURE;

  xiiDeque<xiiToken>& NewTokens = m_CustomDefines.PeekBack().m_Tokenized.GetTokens();

  xiiHashedString sFile;
  sFile.Assign("<CustomDefines>");

  xiiUInt32 uiColumn = 1;
  for (xiiUInt32 t = 0; t < NewTokens.GetCount(); ++t)
  {
    NewTokens[t].m_File     = sFile;
    NewTokens[t].m_uiLine   = m_CustomDefines.GetCount();
    NewTokens[t].m_uiColumn = uiColumn;

    uiColumn += xiiStringUtils::GetCharacterCount(NewTokens[t].m_DataView.GetStartPointer(), NewTokens[t].m_DataView.GetEndPointer());
  }

  xiiUInt32 uiCurToken = 0;
  return HandleDefine(Tokens, uiCurToken);
}

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Defines);
