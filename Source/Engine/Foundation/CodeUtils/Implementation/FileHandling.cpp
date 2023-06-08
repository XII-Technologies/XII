#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace xiiTokenParseUtils;

xiiMap<xiiString, xiiTokenizedFileCache::FileData>::ConstIterator xiiTokenizedFileCache::Lookup(const xiiString& sFileName) const
{
  XII_LOCK(m_Mutex);
  auto it = m_Cache.Find(sFileName);
  return it;
}

void xiiTokenizedFileCache::Remove(const xiiString& sFileName)
{
  XII_LOCK(m_Mutex);
  m_Cache.Remove(sFileName);
}

void xiiTokenizedFileCache::Clear()
{
  XII_LOCK(m_Mutex);
  m_Cache.Clear();
}

void xiiTokenizedFileCache::SkipWhitespace(xiiDeque<xiiToken>& Tokens, xiiUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken].m_iType == xiiTokenType::BlockComment || Tokens[uiCurToken].m_iType == xiiTokenType::LineComment || Tokens[uiCurToken].m_iType == xiiTokenType::Newline || Tokens[uiCurToken].m_iType == xiiTokenType::Whitespace))
    ++uiCurToken;
}

const xiiTokenizer* xiiTokenizedFileCache::Tokenize(const xiiString& sFileName, xiiArrayPtr<const xiiUInt8> fileContent, const xiiTimestamp& fileTimeStamp, xiiLogInterface* pLog)
{
  XII_LOCK(m_Mutex);

  auto& data = m_Cache[sFileName];

  data.m_Timestamp         = fileTimeStamp;
  xiiTokenizer* pTokenizer = &data.m_Tokens;
  pTokenizer->Tokenize(fileContent, pLog);

  xiiDeque<xiiToken>& Tokens = pTokenizer->GetTokens();

  xiiHashedString sFile;
  sFile.Assign(sFileName);

  xiiInt32 iLineOffset = 0;

  for (xiiUInt32 i = 0; i + 1 < Tokens.GetCount(); ++i)
  {
    const xiiUInt32 uiCurLine = Tokens[i].m_uiLine;

    Tokens[i].m_File = sFile;
    Tokens[i].m_uiLine += iLineOffset;

    if (Tokens[i].m_iType == xiiTokenType::NonIdentifier && Tokens[i].m_DataView.IsEqual("#"))
    {
      xiiUInt32 uiNext = i + 1;

      SkipWhitespace(Tokens, uiNext);

      if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == xiiTokenType::Identifier && Tokens[uiNext].m_DataView.IsEqual("line"))
      {
        ++uiNext;
        SkipWhitespace(Tokens, uiNext);

        if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == xiiTokenType::Integer)
        {
          xiiInt32 iNextLine = 0;

          const xiiString sNumber = Tokens[uiNext].m_DataView;
          if (xiiConversionUtils::StringToInt(sNumber, iNextLine).Succeeded())
          {
            iLineOffset = (iNextLine - uiCurLine) - 1;

            ++uiNext;
            SkipWhitespace(Tokens, uiNext);

            if (uiNext < Tokens.GetCount())
            {
              if (Tokens[uiNext].m_iType == xiiTokenType::String1)
              {
                xiiStringBuilder sFileName2 = Tokens[uiNext].m_DataView;
                sFileName2.Shrink(1, 1); // remove surrounding "

                sFile.Assign(sFileName2);
              }
            }
          }
        }
      }
    }
  }

  return pTokenizer;
}


void xiiPreprocessor::SetLogInterface(xiiLogInterface* pLog)
{
  m_pLog = pLog;
}

void xiiPreprocessor::SetFileOpenFunction(FileOpenCB openAbsFileCB)
{
  m_FileOpenCallback = openAbsFileCB;
}

void xiiPreprocessor::SetFileLocatorFunction(FileLocatorCB locateAbsFileCB)
{
  m_FileLocatorCallback = locateAbsFileCB;
}

xiiResult xiiPreprocessor::DefaultFileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, xiiPreprocessor::IncludeType incType, xiiStringBuilder& out_sAbsoluteFilePath)
{
  xiiStringBuilder& s = out_sAbsoluteFilePath;

  if (incType == xiiPreprocessor::RelativeInclude)
  {
    s = szCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(szIncludeFile);
    s.MakeCleanPath();
  }
  else
  {
    s = szIncludeFile;
    s.MakeCleanPath();
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::DefaultFileOpen(const char* szAbsoluteFile, xiiDynamicArray<xiiUInt8>& ref_fileContent, xiiTimestamp& out_fileModification)
{
  xiiFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return XII_FAILURE;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stats;
  if (xiiFileSystem::GetFileStats(szAbsoluteFile, stats).Succeeded())
    out_fileModification = stats.m_LastModificationTime;
#endif

  xiiUInt8 Temp[4096];

  while (xiiUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    ref_fileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));
  }

  return XII_SUCCESS;
}

xiiResult xiiPreprocessor::OpenFile(const char* szFile, const xiiTokenizer** pTokenizer)
{
  XII_ASSERT_DEV(m_FileOpenCallback.IsValid(), "OpenFile callback has not been set");
  XII_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_pUsedFileCache->Lookup(szFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value().m_Tokens;
    return XII_SUCCESS;
  }

  xiiTimestamp stamp;

  xiiDynamicArray<xiiUInt8> Content;
  if (m_FileOpenCallback(szFile, Content, stamp).Failed())
  {
    xiiLog::Error(m_pLog, "Could not open file '{0}'", szFile);
    return XII_FAILURE;
  }

  xiiArrayPtr<const xiiUInt8> ContentView = Content;

  // the file open callback gives us raw data for the opened file
  // the tokenizer doesn't like the Utf8 BOM, so skip it here, if we detect it
  if (ContentView.GetCount() >= 3) // length of a BOM
  {
    const char* dataStart = reinterpret_cast<const char*>(ContentView.GetPtr());

    if (xiiUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      ContentView = xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)dataStart, Content.GetCount() - 3);
    }
  }

  *pTokenizer = m_pUsedFileCache->Tokenize(szFile, ContentView, stamp, m_pLog);

  return XII_SUCCESS;
}


xiiResult xiiPreprocessor::HandleInclude(const TokenStream& Tokens0, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken, TokenStream& TokenOutput)
{
  XII_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  TokenStream Tokens;
  if (Expand(Tokens0, Tokens).Failed())
    return XII_FAILURE;

  SkipWhitespace(Tokens, uiCurToken);

  xiiStringBuilder sPath;

  IncludeType IncType = IncludeType::GlobalInclude;


  xiiUInt32 uiAccepted;
  if (Accept(Tokens, uiCurToken, xiiTokenType::String1, &uiAccepted))
  {
    IncType = IncludeType::RelativeInclude;
    sPath   = Tokens[uiAccepted]->m_DataView;
    sPath.Shrink(1, 1); // remove " at start and end
  }
  else
  {
    // in global include paths (ie. <bla/blub.h>) we need to handle line comments special
    // because a path with two slashes will be a comment token, although it could be a valid path
    // so we concatenate just everything and then make sure it ends with a >

    if (Expect(Tokens, uiCurToken, "<", &uiAccepted).Failed())
      return XII_FAILURE;

    TokenStream PathTokens;

    while (uiCurToken < Tokens.GetCount())
    {
      if (Tokens[uiCurToken]->m_iType == xiiTokenType::Newline)
      {
        break;
      }

      PathTokens.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
    }

    CombineTokensToString(PathTokens, 0, sPath, false);

    // remove all whitespace at the end (this could be part of a comment, so not tokenized as whitespace)
    while (sPath.EndsWith(" ") || sPath.EndsWith("\t"))
      sPath.Shrink(0, 1);

    // there must always be a > at the end, although it could be a separate token or part of a comment
    // so we check the string, instead of the tokens
    if (sPath.EndsWith(">"))
      sPath.Shrink(0, 1);
    else
    {
      PP_LOG(Error, "Invalid include path '{0}'", Tokens[uiAccepted], sPath);
      return XII_FAILURE;
    }
  }

  if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
  {
    PP_LOG0(Error, "Expected end-of-line", Tokens[uiCurToken]);
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(!m_CurrentFileStack.IsEmpty(), "Implementation error.");

  xiiStringBuilder sOtherFile;

  if (m_FileLocatorCallback(m_CurrentFileStack.PeekBack().m_sFileName.GetData(), sPath, IncType, sOtherFile).Failed())
  {
    PP_LOG(Error, "#include file '{0}' could not be located", Tokens[uiAccepted], sPath);
    return XII_FAILURE;
  }

  const xiiTempHashedString sOtherFileHashed(sOtherFile);

  // if this has been included before, and contains a #pragma once, do not include it again
  if (m_PragmaOnce.Find(sOtherFileHashed).IsValid())
    return XII_SUCCESS;

  if (ProcessFile(sOtherFile, TokenOutput).Failed())
    return XII_FAILURE;

  if (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken]->m_iType == xiiTokenType::Newline || Tokens[uiCurToken]->m_iType == xiiTokenType::EndOfFile))
    TokenOutput.PushBack(Tokens[uiCurToken]);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_FileHandling);
