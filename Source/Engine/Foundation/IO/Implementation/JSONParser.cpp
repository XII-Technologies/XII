/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONParser.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

#include <charconv>
#include <cmath>

xiiJSONParser::xiiJSONParser()
{
  m_uiCurByte     = '\0';
  m_uiNextByte    = '\0';
  m_pInput        = nullptr;
  m_bSkippingMode = false;
  m_pLogInterface = nullptr;
  m_uiCurLine     = 1;
  m_uiCurColumn   = 0;
}

void xiiJSONParser::SetInputStream(xiiStreamReader& stream, xiiUInt32 uiFirstLineOffset)
{
  m_StateStack.Clear();
  m_uiCurByte = '\0';
  m_TempString.Clear();
  m_bSkippingMode = false;
  m_bReadNullByte = false;
  m_uiContainerDepth = 0;
  m_uiCurLine     = 1 + uiFirstLineOffset;
  m_uiCurColumn   = 0;

  m_pInput = &stream;

  m_uiNextByte = ' ';
  ReadCharacter(true);

  // go to the start of the document
  SkipWhitespace();

  // put the NotStarted state onto the stack
  {
    JSONState s;
    s.m_State = NotStarted;
    m_StateStack.PushBack(s);
  }
}

void xiiJSONParser::StartParsing()
{
  // remove the NotStarted state
  m_StateStack.PopBack();

  // put the Finished state onto the stack
  {
    JSONState s;
    s.m_State = Finished;
    m_StateStack.PushBack(s);
  }

  switch (m_uiCurByte)
  {
    case '\0':
      // Document is empty.
      return;

    case '{':
    {
      if (!BeginContainer(ReadingObject))
      {
        return;
      }

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginObject();

      return;
    }

    case '[':
    {
      if (!BeginContainer(ReadingArray))
      {
        return;
      }

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginArray();

      return;
    }

    default:
    {
      if (IsStrict())
      {
        m_StateStack.ExpandAndGetRef().m_State = ReadingValue;
        return;
      }

      // Document is malformed

      xiiStringBuilder s;
      s.SetFormat("Start of document: Expected a { or [ or an empty document. Got '{0}' instead.", xiiArgC(m_uiCurByte));
      ParsingError(s.GetData(), true);

      return;
    }
  }
}

void xiiJSONParser::ParseAll()
{
  while (ContinueParsing())
  {
  }
}

void xiiJSONParser::ParsingError(xiiStringView sMessage, bool bFatal)
{
  if (bFatal)
  {
    // prevent further error messages
    m_uiCurByte = '\0';
    m_StateStack.Clear();
  }

  if (bFatal)
    xiiLog::Error(m_pLogInterface, "Line {0} ({1}): {2}", m_uiCurLine, m_uiCurColumn, sMessage);
  else
    xiiLog::Warning(m_pLogInterface, sMessage);

  OnParsingError(sMessage, bFatal, m_uiCurLine, m_uiCurColumn);
}

void xiiJSONParser::SkipObject()
{
  SkipStack(ReadingObject);
}

void xiiJSONParser::SkipArray()
{
  SkipStack(ReadingArray);
}

void xiiJSONParser::SkipStack(State s)
{
  m_bSkippingMode = true;

  xiiUInt32 iSkipToStackHeight = m_StateStack.GetCount();

  for (xiiUInt32 top = m_StateStack.GetCount(); top > 1; --top)
  {
    if (m_StateStack[top - 1].m_State == s)
    {
      iSkipToStackHeight = top - 1;
      break;
    }
  }

  while (m_StateStack.GetCount() > iSkipToStackHeight)
    ContinueParsing();

  m_bSkippingMode = false;
}

bool xiiJSONParser::ContinueParsing()
{
  if (IsStrict() && m_bReadNullByte)
  {
    ParsingError("JSON input contains a null byte.", true);
    return false;
  }

  if (m_StateStack.IsEmpty())
  {
    return false;
  }

  if (m_uiCurByte == '\0')
  {
    // there's always the 'finished' state on the top of the stack when everything went fine
    if (m_StateStack.GetCount() > 1 || (IsStrict() && m_StateStack.PeekBack().m_State != Finished))
    {
      ParsingError("End of the document reached without closing all objects.", true);
    }

    return false;
  }

  switch (m_StateStack.PeekBack().m_State)
  {
    case NotStarted:
      StartParsing();
      break;

    case Finished:
      if (IsStrict())
      {
        if (IsWhiteSpace(m_uiCurByte))
        {
          SkipWhitespace();
          return true;
        }
        ParsingError("Unexpected characters after the JSON value.", true);
      }
      return false;

    case ReadingObject:
      ContinueObject();
      break;

    case ReadingArray:
      ContinueArray();
      break;

    case ReadingVariable:
      ContinueVariable();
      break;

    case ReadingValue:
      ContinueValue();
      break;

    case ExpectSeparator:
      ContinueSeparator();
      break;

    default:
      XII_REPORT_FAILURE("Unknown State in JSON parser state machine.");
      break;
  }

  return true;
}

void xiiJSONParser::ContinueObject()
{
  if (IsStrict() && (m_uiCurByte == ',' || (m_uiCurByte == '}' && m_StateStack.PeekBack().m_bAfterComma)))
  {
    ParsingError("Unexpected comma in JSON object.", true);
    return;
  }

  switch (m_uiCurByte)
  {
    case '\"':
    {
      JSONState s;
      s.m_State = ReadingVariable;
      m_StateStack.PushBack(s);
    }
      return;

    case '}':
      SkipWhitespace();

      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnEndObject();
      --m_uiContainerDepth;
      return;

    case ',': // ignore superfluous commas
      SkipWhitespace();
      return;

    default:
    {
      xiiStringBuilder s;
      s.SetFormat("While parsing object: Expected \" to begin a new variable, or } to close the object. Got '{0}' instead.", xiiArgC(m_uiCurByte));
      ParsingError(s.GetData(), true);
    }
      return;
  }
}

void xiiJSONParser::ContinueArray()
{
  if (IsStrict() && m_uiCurByte == ']' && m_StateStack.PeekBack().m_bAfterComma)
  {
    ParsingError("Trailing comma in JSON array.", true);
    return;
  }

  switch (m_uiCurByte)
  {
    case ']':
    {
      SkipWhitespace();

      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnEndArray();
      --m_uiContainerDepth;
    }
      return;

    default:
    {
      JSONState s;

      s.m_State = ExpectSeparator;
      m_StateStack.PushBack(s);

      s.m_State = ReadingValue;
      m_StateStack.PushBack(s);
    }
      return;
  }
}

void xiiJSONParser::ContinueVariable()
{
  if (!m_bSkippingMode)
    ReadString();
  else
    SkipString();

  if (m_StateStack.IsEmpty())
  {
    return;
  }

  SkipWhitespace();

  if (m_uiCurByte != ':')
  {
    xiiStringBuilder s;
    s.SetFormat("After parsing variable name: Expected : to separate variable and value, Got '{0}' instead.", xiiArgC(m_uiCurByte));
    ParsingError(s.GetData(), false);
  }
  else
    SkipWhitespace();

  // remove ReadingVariable from the stack
  m_StateStack.PopBack();

  JSONState s;

  s.m_State = ExpectSeparator;
  m_StateStack.PushBack(s);

  s.m_State = ReadingValue;
  m_StateStack.PushBack(s);

  if (!m_bSkippingMode)
  {
    if (!OnVariable((const char*)&m_TempString[0]))
      SkipStack(ExpectSeparator);
  }
}


void xiiJSONParser::ContinueValue()
{
  switch (m_uiCurByte)
  {
    case '\0':
      return;

    case '\"':
    {
      if (!m_bSkippingMode)
        ReadString();
      else
        SkipString();

      if (m_StateStack.IsEmpty())
      {
        return;
      }

      SkipWhitespace();

      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnReadValue(xiiStringView((const char*)&m_TempString[0]));
    }
      return;

    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    {
      m_StateStack.PopBack();
      ReadNumber();
    }
      return;

    case 't':
    case 'f':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      ReadWord();

      bool bRes = false;
      const xiiStringView sWord(reinterpret_cast<const char*>(m_TempString.GetData()));
      if (IsStrict())
      {
        if (sWord != "true" && sWord != "false")
        {
          ParsingError("Expected a JSON boolean.", true);
          return;
        }
        bRes = sWord == "true";
      }
      else if (xiiConversionUtils::StringToBool((const char*)&m_TempString[0], bRes) == XII_FAILURE)
      {
        xiiStringBuilder s;
        s.SetFormat("Parsing value: Expected 'true' or 'false', Got '{0}' instead.", (const char*)&m_TempString[0]);
        ParsingError(s.GetData(), false);
      }

      if (!m_bSkippingMode)
        OnReadValue(bRes);
    }
      return;

    case 'n':
    case 'N':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      ReadWord();

      bool bIsNull = false;

      // if it's 'null' but with the wrong casing, output an error, but it is not fatal
      if (xiiStringUtils::IsEqual_NoCase((const char*)&m_TempString[0], "null"))
        bIsNull = true;

      if (!xiiStringUtils::IsEqual((const char*)&m_TempString[0], "null"))
      {
        xiiStringBuilder s;
        s.SetFormat("Parsing value: Expected 'null', Got '{0}' instead.", (const char*)&m_TempString[0]);
        ParsingError(s.GetData(), !bIsNull);
        if (IsStrict() || !bIsNull)
        {
          return;
        }
      }

      if (!m_bSkippingMode)
        OnReadValueNULL();
    }
      return;

    case '[':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      if (!BeginContainer(ReadingArray))
      {
        return;
      }

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginArray();
    }
      return;

    case '{':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      if (!BeginContainer(ReadingObject))
      {
        return;
      }

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginObject();
    }
      return;

    default:
    {
      xiiStringBuilder s;
      s.SetFormat("Parsing value: Expected [, {, f, t, \", 0-1, ., +, -, or even 'e'. Got '{0}' instead", xiiArgC(m_uiCurByte));
      ParsingError(s.GetData(), true);
    }
      return;
  }
}

void xiiJSONParser::ContinueSeparator()
{
  if (IsWhiteSpace(m_uiCurByte))
    SkipWhitespace();

  // remove ExpectSeparator from the stack
  m_StateStack.PopBack();

  switch (m_uiCurByte)
  {
    case '\0':
      return;

    case ',':
      m_StateStack.PeekBack().m_bAfterComma = true;
      SkipWhitespace();
      return;

    case ']':
    case '}':
      m_StateStack.PeekBack().m_bAfterComma = false;
      return;

    default:
    {
      xiiStringBuilder s;
      s.SetFormat("After parsing value: Expected a comma or closing brackets/braces (], }). Got '{0}' instead.", xiiArgC(m_uiCurByte));
      ParsingError(s.GetData(), true);
    }
      return;
  }
}

void xiiJSONParser::ReadNextByte()
{
  if (m_pInput->ReadBytes(&m_uiNextByte, sizeof(xiiUInt8)) != 0 && m_uiNextByte == '\0')
  {
    m_bReadNullByte = true;
  }

  if (m_uiNextByte == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }
  else
    ++m_uiCurColumn;
}

bool xiiJSONParser::ReadCharacter(bool bSkipComments)
{
  m_uiCurByte = m_uiNextByte;

  m_uiNextByte = '\0';
  ReadNextByte();

  // skip comments
  if (m_uiCurByte == '/' && bSkipComments && !IsStrict())
  {
    // line comment, read till line break
    if (m_uiNextByte == '/')
    {
      while (m_uiNextByte != '\0' && m_uiNextByte != '\n')
      {
        m_uiNextByte = '\0';
        ReadNextByte();
      }

      ReadCharacter(true);
    }
    else if (m_uiNextByte == '*') // block comment, read till */
    {
      m_uiNextByte = ' ';

      while (m_uiNextByte != '\0' && (m_uiCurByte != '*' || m_uiNextByte != '/'))
      {
        m_uiCurByte = m_uiNextByte;

        m_uiNextByte = '\0';
        ReadNextByte();
      }

      // replace the current end-comment by whitespace
      m_uiCurByte  = ' ';
      m_uiNextByte = ' ';

      ReadCharacter(true);
      ReadCharacter(true); // might trigger another comment skipping
    }
  }

  return m_uiCurByte != '\0';
}

void xiiJSONParser::SkipWhitespace()
{
  XII_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  do
  {
    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      return; // stop when end of stream is encountered
  } while (IsWhiteSpace(m_uiCurByte));
}

void xiiJSONParser::SkipString()
{
  XII_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();
  m_TempString.PushBack('\0');

  bool bEscapeSequence = false;

  do
  {
    bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter(false))
    {
      ParsingError("While skipping string: Reached end of document before end of string was found.", true);

      return; // stop when end of stream is encountered
    }
  } while (bEscapeSequence || m_uiCurByte != '\"');
}

void xiiJSONParser::ReadString()
{
  XII_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  bool bEscapeSequence = false;

  while (true)
  {
    bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter(false))
    {
      ParsingError("While reading string: Reached end of document before end of string was found.", true);

      break; // Stop when end of stream is encountered
    }

    if (!bEscapeSequence && m_uiCurByte == '\"')
      break;

    if (bEscapeSequence)
    {
      switch (m_uiCurByte)
      {
        case '\"':
          m_TempString.PushBack('\"');
          break;
        case '\\':
          m_TempString.PushBack('\\');
          m_uiCurByte = '\0'; // Make sure the next character isn't interpreted as an escape sequence
          break;
        case '/':
          m_TempString.PushBack('/');
          break;
        case 'b':
          m_TempString.PushBack('\b');
          break;
        case 'f':
          m_TempString.PushBack('\f');
          break;
        case 'n':
          m_TempString.PushBack('\n');
          break;
        case 'r':
          m_TempString.PushBack('\r');
          break;
        case 't':
          m_TempString.PushBack('\t');
          break;
        case 'u':
        {
          xiiUInt16 cpt[2];
          auto      ReadUtf16CodePoint = [&](xiiUInt16& ref_uiCodePoint) -> bool {
            ref_uiCodePoint = 0;

            // Unicode literal are utf16 in the format \uFFFF. The hex number FFFF can be upper or lower case but must be 4 characters long.
            xiiUInt8  unicodeLiteral[5] = {0, 0, 0, 0, 0};
            xiiUInt32 i                 = 0;
            for (; i < 4; i++)
            {
              if (m_uiNextByte == '\0' || m_uiNextByte == '\"')
              {
                ParsingError("Unicode literal is too short, must be 4 HEX characters.", false);
                return false;
              }
              if ((m_uiNextByte < '0' || m_uiNextByte > '9') && (m_uiNextByte < 'A' || m_uiNextByte > 'F') && (m_uiNextByte < 'a' || m_uiNextByte > 'f'))
              {
                ParsingError("Unicode literal contains an invalid character.", false);
                return false;
              }
              ReadCharacter(false);

              unicodeLiteral[i] = m_uiCurByte;
            }

            xiiUInt32 uiHexValue = 0;
            if (xiiConversionUtils::ConvertHexStringToUInt32((const char*)&unicodeLiteral[0], uiHexValue).Succeeded())
            {
              ref_uiCodePoint = static_cast<xiiUInt16>(uiHexValue);
            }
            else
            {
              ParsingError("Unicode HEX literal is malformed.", false);
            }

            return true;
          };
          if (ReadUtf16CodePoint(cpt[0]))
          {
            if (cpt[0] >= 0xDC00 && cpt[0] <= 0xDFFF)
            {
              ParsingError("A Unicode low surrogate requires a preceding high surrogate.", false);
              break;
            }
            if (cpt[0] >= 0xD800 && cpt[0] <= 0xDBFF)
            {
              if (m_uiNextByte != '\\' || !ReadCharacter(false))
              {
                ParsingError("Unicode surrogate must be followed by another unicode escape sequence", false);
                break;
              }
              if (m_uiNextByte != 'u' || !ReadCharacter(false))
              {
                ParsingError("Unicode surrogate must be followed by another unicode escape sequence", false);
                break;
              }
              if (!ReadUtf16CodePoint(cpt[1]))
              {
                break;
              }
              if (cpt[1] < 0xDC00 || cpt[1] > 0xDFFF)
              {
                ParsingError("A Unicode high surrogate requires a following low surrogate.", false);
                break;
              }
            }
            if (IsStrict() && cpt[0] == 0)
            {
              ParsingError("JSON strings cannot contain U+0000 in the engine string representation.", false);
              break;
            }
            const xiiUInt16* pStart = &cpt[0];
            const xiiUInt32 uiCodePoint = xiiUnicodeUtils::DecodeUtf16ToUtf32(pStart);
            xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<xiiUInt8, 4096>> tempInserter(&m_TempString);
            xiiUnicodeUtils::EncodeUtf32ToUtf8(uiCodePoint, tempInserter);
          }
          break;
        }
        default:
        {
          xiiStringBuilder s;
          s.SetFormat("Unknown escape-sequence '\\{0}'", xiiArgC(m_uiCurByte));
          ParsingError(s, false);
        }
        break;
      }
    }
    else if (m_uiCurByte != '\\')
    {
      if (IsStrict() && m_uiCurByte < 0x20)
      {
        ParsingError("Unescaped control character in JSON string.", false);
      }
      m_TempString.PushBack(m_uiCurByte);
    }
  }

  m_TempString.PushBack('\0');
  const char* pText = reinterpret_cast<const char*>(m_TempString.GetData());
  if (IsStrict() && !m_StateStack.IsEmpty() && !xiiUnicodeUtils::ValidateUtf8(pText, pText + m_TempString.GetCount() - 1))
  {
    ParsingError("Invalid UTF-8 in JSON string.", true);
  }
}

void xiiJSONParser::ReadWord()
{
  XII_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  do
  {
    m_TempString.PushBack(m_uiCurByte);

    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      break; // Stop when end of stream is encountered
  } while (!IsWhiteSpace(m_uiCurByte) && m_uiCurByte != ',' && m_uiCurByte != ']' && m_uiCurByte != '}');

  m_TempString.PushBack('\0');
}

void xiiJSONParser::ReadNumber()
{
  XII_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  do
  {
    m_TempString.PushBack(m_uiCurByte);

    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      break; // Stop when end of stream is encountered
  } while ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || m_uiCurByte == '.' || m_uiCurByte == 'e' || m_uiCurByte == 'E' || m_uiCurByte == '-' || m_uiCurByte == '+');

  m_TempString.PushBack('\0');

  if (IsStrict())
  {
    ReadStrictNumber(xiiStringView(reinterpret_cast<const char*>(m_TempString.GetData()), m_TempString.GetCount() - 1));
    return;
  }

  double fResult = 0;
  if (xiiConversionUtils::StringToFloat((const char*)&m_TempString[0], fResult) == XII_FAILURE)
  {
    xiiStringBuilder s;
    s.SetFormat("Reading number failed: Could not convert '{0}' to a floating point value.", (const char*)&m_TempString[0]);
    ParsingError(s.GetData(), true);
    return;
  }

  if (!m_bSkippingMode)
  {
    OnReadValue(fResult);
  }
}

void xiiJSONParser::ReadStrictNumber(xiiStringView sNumber)
{
  const char* pBegin = sNumber.GetStartPointer();
  const char* pEnd = sNumber.GetEndPointer();
  const bool bNegative = *pBegin == '-';
  const char* pInteger = pBegin + (bNegative ? 1 : 0);
  const auto IsDigit = [](char character) { return character >= '0' && character <= '9'; };
  if (pInteger == pEnd || !IsDigit(*pInteger) || (*pInteger == '0' && pInteger + 1 < pEnd && IsDigit(pInteger[1])))
  {
    ParsingError("Invalid JSON number.", true);
    return;
  }

  if (sNumber.FindSubString(".") == nullptr && sNumber.FindSubString_NoCase("e") == nullptr)
  {
    if (bNegative)
    {
      xiiInt64 iValue = 0;
      const auto result = std::from_chars(pBegin, pEnd, iValue);
      if (result.ec == std::errc() && result.ptr == pEnd)
      {
        if (!m_bSkippingMode)
        {
          OnReadInteger(iValue);
        }
        return;
      }
    }
    else
    {
      xiiUInt64 uiValue = 0;
      const auto result = std::from_chars(pBegin, pEnd, uiValue);
      if (result.ec == std::errc() && result.ptr == pEnd)
      {
        if (!m_bSkippingMode)
        {
          OnReadUnsignedInteger(uiValue);
        }
        return;
      }
    }
  }

  const char* pDecimal = sNumber.FindSubString(".");
  if (pDecimal != nullptr && (pDecimal + 1 == pEnd || !IsDigit(pDecimal[1])))
  {
    ParsingError("Expected a digit after the JSON decimal point.", true);
    return;
  }

  double fValue = 0.0;
  const auto result = std::from_chars(pBegin, pEnd, fValue);
  if (result.ec != std::errc() || result.ptr != pEnd || !std::isfinite(fValue))
  {
    ParsingError("Invalid or out-of-range JSON number.", true);
    return;
  }

  if (!m_bSkippingMode)
  {
    OnReadValue(fValue);
  }
}

bool xiiJSONParser::BeginContainer(State state)
{
  if (IsStrict() && m_uiContainerDepth >= m_uiMaximumStrictDepth)
  {
    ParsingError("JSON nesting limit exceeded.", true);
    return false;
  }

  ++m_uiContainerDepth;
  m_StateStack.ExpandAndGetRef().m_State = state;
  return true;
}

bool xiiJSONParser::IsWhiteSpace(xiiUInt8 uiByte) const
{
  return IsStrict() ? (uiByte == ' ' || uiByte == '\t' || uiByte == '\r' || uiByte == '\n') : xiiStringUtils::IsWhiteSpace(uiByte);
}
