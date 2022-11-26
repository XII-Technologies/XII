#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Memory/CommonAllocators.h>

const char* xiiTokenType::EnumNames[xiiTokenType::ENUM_COUNT] = {
  "Unknown",
  "Whitespace",
  "Identifier",
  "NonIdentifier",
  "Newline",
  "LineComment",
  "BlockComment",
  "String1",
  "String2",
  "Integer",
  "Float",
};

namespace
{
  // This allocator is used to get rid of some of the memory allocation tracking
  // that would otherwise occur for allocations made by the tokenizer.
  thread_local xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation, xiiMemoryTrackingFlags::None> s_ClassAllocator("xiiTokenizer", xiiFoundation::GetDefaultAllocator());
} // namespace


xiiTokenizer::xiiTokenizer(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator != nullptr ? pAllocator : &s_ClassAllocator), m_Tokens(pAllocator != nullptr ? pAllocator : &s_ClassAllocator)
{
}

xiiTokenizer::~xiiTokenizer() = default;

void xiiTokenizer::NextChar()
{
  m_uiCurChar      = m_uiNextChar;
  m_szCurCharStart = m_szNextCharStart;
  ++m_uiCurColumn;

  if (m_uiCurChar == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }

  if (!m_sIterator.IsValid())
  {
    m_szNextCharStart = m_sIterator.GetEndPointer();
    m_uiNextChar      = '\0';
    return;
  }

  m_uiNextChar      = m_sIterator.GetCharacter();
  m_szNextCharStart = m_sIterator.GetStartPointer();

  ++m_sIterator;
}

void xiiTokenizer::AddToken()
{
  const char* szEnd = m_szCurCharStart;

  xiiToken t;
  t.m_uiLine   = m_uiLastLine;
  t.m_uiColumn = m_uiLastColumn;
  t.m_iType    = m_CurMode;
  t.m_DataView = xiiStringView(m_szTokenStart, szEnd);

  m_uiLastLine   = m_uiCurLine;
  m_uiLastColumn = m_uiCurColumn;

  m_Tokens.PushBack(t);

  m_szTokenStart = szEnd;

  m_CurMode = xiiTokenType::Unknown;
}

void xiiTokenizer::Tokenize(xiiArrayPtr<const xiiUInt8> Data, xiiLogInterface* pLog)
{
  if (Data.GetCount() >= 3)
  {
    const char* dataStart = reinterpret_cast<const char*>(Data.GetPtr());

    if (xiiUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      xiiLog::Error(pLog, "Data to tokenize contains a Utf-8 BOM.");

      // although the tokenizer should get data without a BOM, it's easy enough to work around that here
      // that's what the tokenizer does in other error cases as well - complain, but continue
      Data = xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)dataStart, Data.GetCount() - 3);
    }
  }

  m_Data.Clear();
  m_Data.Reserve(m_Data.GetCount() + 1);
  m_Data = Data;

  if (m_Data.IsEmpty() || m_Data[m_Data.GetCount() - 1] != 0)
    m_Data.PushBack('\0'); // make sure the string is zero terminated

  m_Tokens.Clear();
  m_pLog = pLog;

  {
    m_CurMode      = xiiTokenType::Unknown;
    m_uiCurLine    = 1;
    m_uiCurColumn  = -1;
    m_uiCurChar    = '\0';
    m_uiNextChar   = '\0';
    m_uiLastLine   = 1;
    m_uiLastColumn = 1;

    m_szCurCharStart  = nullptr;
    m_szNextCharStart = nullptr;
    m_szTokenStart    = nullptr;
  }

  m_sIterator = xiiStringView((const char*)&m_Data[0], (const char*)&m_Data[0] + m_Data.GetCount() - 1);

  if (!m_sIterator.IsValid())
  {
    xiiToken t;
    t.m_uiLine = 1;
    t.m_iType  = xiiTokenType::EndOfFile;
    m_Tokens.PushBack(t);
    return;
  }

  NextChar();
  NextChar();

  m_szTokenStart = m_szCurCharStart;

  while (m_szTokenStart != nullptr && *m_szTokenStart != '\0')
  {
    switch (m_CurMode)
    {
      case xiiTokenType::Unknown:
        HandleUnknown();
        break;

      case xiiTokenType::String1:
        HandleString('\"');
        break;

      case xiiTokenType::String2:
        HandleString('\'');
        break;

      case xiiTokenType::Integer:
      case xiiTokenType::Float:
        HandleNumber();
        break;

      case xiiTokenType::LineComment:
        HandleLineComment();
        break;

      case xiiTokenType::BlockComment:
        HandleBlockComment();
        break;

      case xiiTokenType::Whitespace:
        HandleWhitespace();
        break;

      case xiiTokenType::Identifier:
        HandleIdentifier();
        break;

      case xiiTokenType::NonIdentifier:
        HandleNonIdentifier();
        break;

      case xiiTokenType::Newline:
      case xiiTokenType::EndOfFile:
      case xiiTokenType::ENUM_COUNT:
        break;
    }
  }

  xiiToken t;
  t.m_uiLine = m_uiCurLine;
  t.m_iType  = xiiTokenType::EndOfFile;
  m_Tokens.PushBack(t);
}

void xiiTokenizer::HandleUnknown()
{
  m_szTokenStart = m_szCurCharStart;

  if ((m_uiCurChar == '/') && (m_uiNextChar == '/'))
  {
    m_CurMode = xiiTokenType::LineComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_bHashSignIsLineComment && (m_uiCurChar == '#'))
  {
    m_CurMode = xiiTokenType::LineComment;
    NextChar();
    return;
  }

  if ((m_uiCurChar == '/') && (m_uiNextChar == '*'))
  {
    m_CurMode = xiiTokenType::BlockComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\"')
  {
    m_CurMode = xiiTokenType::String1;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\'')
  {
    m_CurMode = xiiTokenType::String2;
    NextChar();
    return;
  }

  if ((m_uiCurChar == ' ') || (m_uiCurChar == '\t'))
  {
    m_CurMode = xiiTokenType::Whitespace;
    NextChar();
    return;
  }

  if (xiiStringUtils::IsDecimalDigit(m_uiCurChar) || (m_uiCurChar == '.' && xiiStringUtils::IsDecimalDigit(m_uiNextChar)))
  {
    m_CurMode = m_uiCurChar == '.' ? xiiTokenType::Float : xiiTokenType::Integer;
    // Do not advance to next char here since we need the first character in HandleNumber
    return;
  }

  if (!xiiStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
  {
    m_CurMode = xiiTokenType::Identifier;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\n')
  {
    m_CurMode = xiiTokenType::Newline;
    NextChar();
    AddToken();
    return;
  }

  if ((m_uiCurChar == '\r') && (m_uiNextChar == '\n'))
  {
    NextChar();
    NextChar();
    m_CurMode = xiiTokenType::Newline;
    AddToken();
    return;
  }

  // else
  m_CurMode = xiiTokenType::NonIdentifier;
  NextChar();
}

void xiiTokenizer::HandleString(char terminator)
{
  while (m_uiCurChar != '\0')
  {
    // Escaped quote \"
    if ((m_uiCurChar == '\\') && (m_uiNextChar == terminator))
    {
      // skip this one
      NextChar();
      NextChar();
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\n'))
    {
      AddToken();

      // skip this one entirely
      NextChar();
      NextChar();

      m_CurMode      = terminator == '\"' ? xiiTokenType::String1 : xiiTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\r'))
    {
      // this might be a 3 character sequence of \\ \r \n -> skip them all
      AddToken();

      // skip \\ and \r
      NextChar();
      NextChar();

      // skip \n
      if (m_uiCurChar == '\n')
        NextChar();

      m_CurMode      = terminator == '\"' ? xiiTokenType::String1 : xiiTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped backslash
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\\'))
    {
      // Skip
      NextChar();
      NextChar();
    }
    // not-escaped line break in string
    else if (m_uiCurChar == '\n')
    {
      xiiLog::Error(m_pLog, "Unescaped Newline in string line {0} column {1}", m_uiCurLine, m_uiCurColumn);
      // NextChar(); // not sure whether to include the newline in the string or not
      AddToken();
      return;
    }
    // end of string
    else if (m_uiCurChar == terminator)
    {
      NextChar();
      AddToken();
      return;
    }
    else
    {
      NextChar();
    }
  }

  xiiLog::Error(m_pLog, "String not closed at end of file");
  AddToken();
}

void xiiTokenizer::HandleNumber()
{
  if (m_uiCurChar == '0' && (m_uiNextChar == 'x' || m_uiNextChar == 'X'))
  {
    NextChar();
    NextChar();

    xiiUInt32 uiDigitsRead = 0;
    while (xiiStringUtils::IsHexDigit(m_uiCurChar))
    {
      NextChar();
      ++uiDigitsRead;
    }

    if (uiDigitsRead < 1)
    {
      xiiLog::Error(m_pLog, "Invalid hex literal");
    }
  }
  else
  {
    NextChar();

    while (xiiStringUtils::IsDecimalDigit(m_uiCurChar))
    {
      NextChar();
    }

    if (m_CurMode != xiiTokenType::Float && (m_uiCurChar == '.' || m_uiCurChar == 'e' || m_uiCurChar == 'E'))
    {
      m_CurMode           = xiiTokenType::Float;
      bool bAllowExponent = true;

      if (m_uiCurChar == '.')
      {
        NextChar();

        xiiUInt32 uiDigitsRead = 0;
        while (xiiStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        bAllowExponent = uiDigitsRead > 0;
      }

      if ((m_uiCurChar == 'e' || m_uiCurChar == 'E') && bAllowExponent)
      {
        NextChar();
        if (m_uiCurChar == '+' || m_uiCurChar == '-')
        {
          NextChar();
        }

        xiiUInt32 uiDigitsRead = 0;
        while (xiiStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        if (uiDigitsRead < 1)
        {
          xiiLog::Error(m_pLog, "Invalid float literal");
        }
      }

      if (m_uiCurChar == 'f') // skip float suffix
      {
        NextChar();
      }
    }
  }

  AddToken();
}

void xiiTokenizer::HandleLineComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '\r') || (m_uiCurChar == '\n'))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // comment at end of file
  AddToken();
}

void xiiTokenizer::HandleBlockComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '*') && (m_uiNextChar == '/'))
    {
      NextChar();
      NextChar();
      AddToken();
      return;
    }

    NextChar();
  }

  xiiLog::Error(m_pLog, "Block comment not closed at end of file.");
  AddToken();
}

void xiiTokenizer::HandleWhitespace()
{
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar != ' ' && m_uiCurChar != '\t')
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // whitespace at end of file
  AddToken();
}

void xiiTokenizer::HandleIdentifier()
{
  while (m_uiCurChar != '\0')
  {
    if (xiiStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // identifier at end of file
  AddToken();
}

void xiiTokenizer::HandleNonIdentifier()
{
  AddToken();
}

void xiiTokenizer::GetAllLines(xiiHybridArray<const xiiToken*, 32>& Tokens) const
{
  Tokens.Clear();
  Tokens.Reserve(m_Tokens.GetCount());

  for (const xiiToken& curToken : m_Tokens)
  {
    if (curToken.m_iType != xiiTokenType::Newline)
    {
      Tokens.PushBack(&curToken);
    }
  }
}

xiiResult xiiTokenizer::GetNextLine(xiiUInt32& uiFirstToken, xiiHybridArray<xiiToken*, 32>& Tokens)
{
  Tokens.Clear();

  xiiHybridArray<const xiiToken*, 32> Tokens0;
  xiiResult                           r = GetNextLine(uiFirstToken, Tokens0);

  Tokens.SetCountUninitialized(Tokens0.GetCount());
  for (xiiUInt32 i = 0; i < Tokens0.GetCount(); ++i)
    Tokens[i] = const_cast<xiiToken*>(Tokens0[i]); // soo evil !

  return r;
}

xiiResult xiiTokenizer::GetNextLine(xiiUInt32& uiFirstToken, xiiHybridArray<const xiiToken*, 32>& Tokens) const
{
  Tokens.Clear();

  const xiiUInt32 uiMaxTokens = m_Tokens.GetCount() - 1;

  while (uiFirstToken < uiMaxTokens)
  {
    const xiiToken& tCur = m_Tokens[uiFirstToken];

    // found a backslash
    if (tCur.m_iType == xiiTokenType::NonIdentifier && tCur.m_DataView == "\\")
    {
      const xiiToken& tNext = m_Tokens[uiFirstToken + 1];

      // and a newline!
      if (tNext.m_iType == xiiTokenType::Newline)
      {
        /// \todo Theoretically, if the line ends with an identifier, and the next directly starts with one again,
        // we would need to merge the two into one identifier name, because the \ \n combo means it is not a
        // real line break
        // for now we ignore this and assume there is a 'whitespace' between such identifiers

        // we could maybe at least output a warning, if we detect it
        if (uiFirstToken > 0 && m_Tokens[uiFirstToken - 1].m_iType == xiiTokenType::Identifier && uiFirstToken + 2 < uiMaxTokens && m_Tokens[uiFirstToken + 2].m_iType == xiiTokenType::Identifier)
        {
          xiiStringBuilder s1 = m_Tokens[uiFirstToken - 1].m_DataView;
          xiiStringBuilder s2 = m_Tokens[uiFirstToken + 2].m_DataView;
          xiiLog::Warning("Line {0}: The \\ at the line end is in the middle of an identifier name ('{1}' and '{2}'). However, merging identifier "
                          "names is currently not supported.",
                          m_Tokens[uiFirstToken].m_uiLine, s1, s2);
        }

        // ignore this
        uiFirstToken += 2;
        continue;
      }
    }

    Tokens.PushBack(&tCur);

    if (m_Tokens[uiFirstToken].m_iType == xiiTokenType::Newline)
    {
      ++uiFirstToken;
      return XII_SUCCESS;
    }

    ++uiFirstToken;
  }

  if (Tokens.IsEmpty())
    return XII_FAILURE;

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Tokenizer);
