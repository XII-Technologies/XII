/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

void xiiOpenDdlWriter::OutputEscapedString(const xiiStringView& string)
{
  m_sTemp = string;
  m_sTemp.ReplaceAll("\\", "\\\\");
  m_sTemp.ReplaceAll("\"", "\\\"");
  m_sTemp.ReplaceAll("\b", "\\b");
  m_sTemp.ReplaceAll("\r", "\\r");
  m_sTemp.ReplaceAll("\f", "\\f");
  m_sTemp.ReplaceAll("\n", "\\n");
  m_sTemp.ReplaceAll("\t", "\\t");

  OutputString("\"", 1);
  OutputString(m_sTemp.GetData());
  OutputString("\"", 1);
}

void xiiOpenDdlWriter::OutputIndentation()
{
  if (m_bCompactMode)
    return;

  xiiInt32 iIndentation = m_iIndentation;

  // I need my space!
  const char* szIndentation = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

  while (iIndentation >= 16)
  {
    OutputString(szIndentation, 16);
    iIndentation -= 16;
  }

  if (iIndentation > 0)
  {
    OutputString(szIndentation, iIndentation);
  }
}

void xiiOpenDdlWriter::OutputPrimitiveTypeNameCompliant(xiiOpenDdlPrimitiveType type)
{
  switch (type)
  {
    case xiiOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case xiiOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case xiiOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case xiiOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case xiiOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case xiiOpenDdlPrimitiveType::UInt8:
      OutputString("unsigned_int8", 13);
      break;
    case xiiOpenDdlPrimitiveType::UInt16:
      OutputString("unsigned_int16", 14);
      break;
    case xiiOpenDdlPrimitiveType::UInt32:
      OutputString("unsigned_int32", 14);
      break;
    case xiiOpenDdlPrimitiveType::UInt64:
      OutputString("unsigned_int64", 14);
      break;
    case xiiOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case xiiOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case xiiOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      XII_REPORT_FAILURE("Unknown DDL primitive type {0}", (xiiUInt32)type);
      break;
  }
}
void xiiOpenDdlWriter::OutputPrimitiveTypeNameShort(xiiOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write uint8 etc. instead of unsigned_int

  switch (type)
  {
    case xiiOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case xiiOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case xiiOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case xiiOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case xiiOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case xiiOpenDdlPrimitiveType::UInt8:
      OutputString("uint8", 5);
      break;
    case xiiOpenDdlPrimitiveType::UInt16:
      OutputString("uint16", 6);
      break;
    case xiiOpenDdlPrimitiveType::UInt32:
      OutputString("uint32", 6);
      break;
    case xiiOpenDdlPrimitiveType::UInt64:
      OutputString("uint64", 6);
      break;
    case xiiOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case xiiOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case xiiOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      XII_REPORT_FAILURE("Unknown DDL primitive type {0}", (xiiUInt32)type);
      break;
  }
}

void xiiOpenDdlWriter::OutputPrimitiveTypeNameShortest(xiiOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write super short type strings

  switch (type)
  {
    case xiiOpenDdlPrimitiveType::Bool:
      OutputString("b", 1);
      break;
    case xiiOpenDdlPrimitiveType::Int8:
      OutputString("i1", 2);
      break;
    case xiiOpenDdlPrimitiveType::Int16:
      OutputString("i2", 2);
      break;
    case xiiOpenDdlPrimitiveType::Int32:
      OutputString("i3", 2);
      break;
    case xiiOpenDdlPrimitiveType::Int64:
      OutputString("i4", 2);
      break;
    case xiiOpenDdlPrimitiveType::UInt8:
      OutputString("u1", 2);
      break;
    case xiiOpenDdlPrimitiveType::UInt16:
      OutputString("u2", 2);
      break;
    case xiiOpenDdlPrimitiveType::UInt32:
      OutputString("u3", 2);
      break;
    case xiiOpenDdlPrimitiveType::UInt64:
      OutputString("u4", 2);
      break;
    case xiiOpenDdlPrimitiveType::Float:
      OutputString("f", 1);
      break;
    case xiiOpenDdlPrimitiveType::Double:
      OutputString("d", 1);
      break;
    case xiiOpenDdlPrimitiveType::String:
      OutputString("s", 1);
      break;

    default:
      XII_REPORT_FAILURE("Unknown DDL primitive type {0}", (xiiUInt32)type);
      break;
  }
}

xiiOpenDdlWriter::xiiOpenDdlWriter()
{
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesBool == (xiiInt32)xiiOpenDdlPrimitiveType::Bool);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesInt8 == (xiiInt32)xiiOpenDdlPrimitiveType::Int8);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesInt16 == (xiiInt32)xiiOpenDdlPrimitiveType::Int16);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesInt32 == (xiiInt32)xiiOpenDdlPrimitiveType::Int32);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesInt64 == (xiiInt32)xiiOpenDdlPrimitiveType::Int64);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesUInt8 == (xiiInt32)xiiOpenDdlPrimitiveType::UInt8);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesUInt16 == (xiiInt32)xiiOpenDdlPrimitiveType::UInt16);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesUInt32 == (xiiInt32)xiiOpenDdlPrimitiveType::UInt32);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesUInt64 == (xiiInt32)xiiOpenDdlPrimitiveType::UInt64);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesFloat == (xiiInt32)xiiOpenDdlPrimitiveType::Float);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesDouble == (xiiInt32)xiiOpenDdlPrimitiveType::Double);
  static_assert((xiiInt32)xiiOpenDdlWriter::State::PrimitivesString == (xiiInt32)xiiOpenDdlPrimitiveType::String);

  m_StateStack.ExpandAndGetRef().m_State = State::Invalid;
  m_StateStack.ExpandAndGetRef().m_State = State::Empty;
}

// All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
// LessIndentation,  ///< Saves some space by using less space for indentation
// NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
// NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
// None,             ///< No whitespace, not even newlines, is output. This should be used when DDL is used for data exchange, but probably not read
// by humans.

void xiiOpenDdlWriter::BeginObject(xiiStringView sType, xiiStringView sName /*= {}*/, bool bGlobalName /*= false*/, bool bSingleLine /*= false*/)
{
  {
    const auto state = m_StateStack.PeekBack().m_State;
    XII_IGNORE_UNUSED(state);
    XII_ASSERT_DEBUG(state == State::Empty || state == State::ObjectMultiLine || state == State::ObjectStart,
                     "DDL Writer is in a state where no further objects may be created");
  }

  OutputObjectBeginning();

  {
    const auto state = m_StateStack.PeekBack().m_State;
    XII_IGNORE_UNUSED(state);
    XII_ASSERT_DEBUG(state != State::ObjectSingleLine, "Cannot put an object into another single-line object");
    XII_ASSERT_DEBUG(state != State::ObjectStart, "Object beginning should have been written");
  }

  OutputIndentation();
  OutputString(sType);

  OutputObjectName(sName, bGlobalName);

  if (bSingleLine)
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectSingleLine;
  }
  else
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectMultiLine;
  }

  m_StateStack.ExpandAndGetRef().m_State = State::ObjectStart;
}


void xiiOpenDdlWriter::OutputObjectBeginning()
{
  if (m_StateStack.PeekBack().m_State != State::ObjectStart)
    return;

  m_StateStack.PopBack();

  const auto state = m_StateStack.PeekBack().m_State;

  if (state == State::ObjectSingleLine)
  {
    // if (m_bCompactMode)
    OutputString("{", 1); // more compact
    // else
    // OutputString(" { ", 3);
  }
  else if (state == State::ObjectMultiLine)
  {
    if (m_bCompactMode)
    {
      OutputString("{", 1);
    }
    else
    {
      OutputString("\n", 1);
      OutputIndentation();
      OutputString("{\n", 2);
    }
  }

  m_iIndentation++;
}

bool IsDdlIdentifierCharacter(xiiUInt32 uiByte);

void xiiOpenDdlWriter::OutputObjectName(xiiStringView sName, bool bGlobalName)
{
  if (!sName.IsEmpty())
  {
    // XII_ASSERT_DEBUG(xiiStringUtils::FindSubString(szName, " ") == nullptr, "Spaces are not allowed in DDL object names: '{0}'", szName);


    /// \test This code path is untested
    bool bEscape = false;
    for (auto nameIt = sName.GetIteratorFront(); nameIt.IsValid(); ++nameIt)
    {
      if (!IsDdlIdentifierCharacter(nameIt.GetCharacter()))
      {
        bEscape = true;
        break;
      }
    }

    if (m_bCompactMode)
    {
      // even remove the whitespace between type and name

      if (bGlobalName)
        OutputString("$", 1);
      else
        OutputString("%", 1);
    }
    else
    {
      if (bGlobalName)
        OutputString(" $", 2);
      else
        OutputString(" %", 2);
    }

    if (bEscape)
      OutputString("\'", 1);

    OutputString(sName);

    if (bEscape)
      OutputString("\'", 1);
  }
}

void xiiOpenDdlWriter::EndObject()
{
  const auto state = m_StateStack.PeekBack().m_State;
  XII_ASSERT_DEBUG(state == State::ObjectSingleLine || state == State::ObjectMultiLine || state == State::ObjectStart, "No object is open");

  if (state == State::ObjectStart)
  {
    // object is empty

    OutputString("{}\n", 3);
    m_StateStack.PopBack();

    const auto newState = m_StateStack.PeekBack().m_State;
    XII_IGNORE_UNUSED(newState);
    XII_ASSERT_DEBUG(newState == State::ObjectSingleLine || newState == State::ObjectMultiLine, "No object is open");
  }
  else
  {
    m_iIndentation--;

    if (m_bCompactMode)
      OutputString("}", 1);
    else
    {
      if (state == State::ObjectMultiLine)
      {
        OutputIndentation();
        OutputString("}\n", 2);
      }
      else
      {
        // OutputString(" }\n", 3);
        OutputString("}\n", 2); // more compact
      }
    }
  }

  m_StateStack.PopBack();
}

void xiiOpenDdlWriter::BeginPrimitiveList(xiiOpenDdlPrimitiveType type, xiiStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  OutputObjectBeginning();

  const auto state = m_StateStack.PeekBack().m_State;
  XII_ASSERT_DEBUG(state == State::Empty || state == State::ObjectSingleLine || state == State::ObjectMultiLine,
                   "DDL Writer is in a state where no primitive list may be created");

  if (state == State::ObjectMultiLine)
  {
    OutputIndentation();
  }

  if (m_TypeStringMode == TypeStringMode::Shortest)
    OutputPrimitiveTypeNameShortest(type);
  else if (m_TypeStringMode == TypeStringMode::ShortenedUnsignedInt)
    OutputPrimitiveTypeNameShort(type);
  else
    OutputPrimitiveTypeNameCompliant(type);

  OutputObjectName(sName, bGlobalName);

  // more compact
  // if (m_bCompactMode)
  OutputString("{", 1);
  // else
  // OutputString(" {", 2);

  m_StateStack.ExpandAndGetRef().m_State = static_cast<State>(type);
}

void xiiOpenDdlWriter::EndPrimitiveList()
{
  const auto state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEBUG(state >= State::PrimitivesBool && state <= State::PrimitivesString, "No primitive list is open");

  m_StateStack.PopBack();

  if (m_bCompactMode)
    OutputString("}", 1);
  else
  {
    if (m_StateStack.PeekBack().m_State == State::ObjectSingleLine)
      OutputString("}", 1);
    else
      OutputString("}\n", 2);
  }
}

void xiiOpenDdlWriter::WritePrimitiveType(xiiOpenDdlWriter::State exp)
{
  XII_IGNORE_UNUSED(exp);

  auto& state = m_StateStack.PeekBack();
  XII_ASSERT_DEBUG(state.m_State == exp, "Cannot write thie primitive type without have the correct primitive list open");

  if (state.m_bPrimitivesWritten)
  {
    // already wrote some primitives, so append a comma
    OutputString(",", 1);
  }

  state.m_bPrimitivesWritten = true;
}


void xiiOpenDdlWriter::WriteBinaryAsHex(const void* pData, xiiUInt32 uiBytes)
{
  char tmp[4];

  xiiUInt8* pBytes = (xiiUInt8*)pData;

  for (xiiUInt32 i = 0; i < uiBytes; ++i)
  {
    xiiStringUtils::snprintf(tmp, 4, "%02X", (xiiUInt32)*pBytes);
    ++pBytes;

    OutputString(tmp, 2);
  }
}

void xiiOpenDdlWriter::WriteBool(const bool* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesBool);

  if (m_bCompactMode || m_TypeStringMode == TypeStringMode::Shortest)
  {
    // Extension to OpenDDL: We write only '1' or '0' in compact mode

    if (pValues[0])
      OutputString("1", 1);
    else
      OutputString("0", 1);

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",1", 2);
      else
        OutputString(",0", 2);
    }
  }
  else
  {
    if (pValues[0])
      OutputString("true", 4);
    else
      OutputString("false", 5);

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",true", 5);
      else
        OutputString(",false", 6);
    }
  }
}

void xiiOpenDdlWriter::WriteInt8(const xiiInt8* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt8);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteInt16(const xiiInt16* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt16);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteInt32(const xiiInt32* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt32);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteInt64(const xiiInt64* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt64);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}


void xiiOpenDdlWriter::WriteUInt8(const xiiUInt8* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt8);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteUInt16(const xiiUInt16* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt16);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteUInt32(const xiiUInt32* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt32);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteUInt64(const xiiUInt64* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt64);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void xiiOpenDdlWriter::WriteFloat(const float* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesFloat);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.SetFormat("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.SetFormat(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 4);
    }

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 4);
      }
    }
  }
}

void xiiOpenDdlWriter::WriteDouble(const double* pValues, xiiUInt32 uiCount /*= 1*/)
{
  XII_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  XII_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesDouble);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.SetFormat("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.SetFormat(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 8);
    }

    for (xiiUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 8);
      }
    }
  }
}

void xiiOpenDdlWriter::WriteString(const xiiStringView& sString)
{
  WritePrimitiveType(State::PrimitivesString);

  OutputEscapedString(sString);
}

void xiiOpenDdlWriter::WriteBinaryAsString(const void* pData, xiiUInt32 uiBytes)
{
  /// \test xiiOpenDdlWriter::WriteBinaryAsString

  WritePrimitiveType(State::PrimitivesString);

  OutputString("\"", 1);
  WriteBinaryAsHex(pData, uiBytes);
  OutputString("\"", 1);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlWriter);
