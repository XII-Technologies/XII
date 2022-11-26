#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

xiiStandardJSONWriter::JSONState::JSONState()
{
  m_State            = Invalid;
  m_bRequireComma    = false;
  m_bValueWasWritten = false;
}

xiiStandardJSONWriter::CommaWriter::CommaWriter(xiiStandardJSONWriter* pWriter)
{
  const xiiStandardJSONWriter::State state = pWriter->m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV(state == xiiStandardJSONWriter::Array || state == xiiStandardJSONWriter::NamedArray || state == xiiStandardJSONWriter::Variable,
                 "Values can only be written inside BeginVariable() / EndVariable() and BeginArray() / EndArray().");

  m_pWriter = pWriter;

  if (m_pWriter->m_StateStack.PeekBack().m_bRequireComma)
  {
    // we are writing the comma now, so it is not required anymore
    m_pWriter->m_StateStack.PeekBack().m_bRequireComma = false;

    if (m_pWriter->m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::Array ||
        m_pWriter->m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::NamedArray)
    {
      if (pWriter->m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
      {
        if (pWriter->m_ArrayMode == xiiJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(",");
        else
          m_pWriter->OutputString(",\n");
      }
      else
      {
        if (pWriter->m_ArrayMode == xiiJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(", ");
        else
        {
          m_pWriter->OutputString(",\n");
          m_pWriter->OutputIndentation();
        }
      }
    }
    else
    {
      if (pWriter->m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::None)
        m_pWriter->OutputString(",");
      else
        m_pWriter->OutputString(",\n");

      m_pWriter->OutputIndentation();
    }
  }
}

xiiStandardJSONWriter::CommaWriter::~CommaWriter()
{
  m_pWriter->m_StateStack.PeekBack().m_bRequireComma    = true;
  m_pWriter->m_StateStack.PeekBack().m_bValueWasWritten = true;
}

xiiStandardJSONWriter::xiiStandardJSONWriter()
{
  m_iIndentation = 0;
  m_pOutput      = nullptr;
  JSONState s;
  s.m_State = xiiStandardJSONWriter::Empty;
  m_StateStack.PushBack(s);
}

xiiStandardJSONWriter::~xiiStandardJSONWriter()
{
  if (!HadWriteError())
  {
    XII_ASSERT_DEV(m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::Empty, "The JSON stream must be closed properly.");
  }
}

void xiiStandardJSONWriter::SetOutputStream(xiiStreamWriter* pOutput)
{
  m_pOutput = pOutput;
}

void xiiStandardJSONWriter::OutputString(const char* sz)
{
  XII_ASSERT_DEBUG(m_pOutput != nullptr, "No output stream has been set yet.");

  if (m_pOutput->WriteBytes(sz, xiiStringUtils::GetStringElementCount(sz)).Failed())
  {
    SetWriteErrorState();
  }
}

void xiiStandardJSONWriter::OutputEscapedString(const char* sz)
{
  xiiStringBuilder sEscaped = sz;
  sEscaped.ReplaceAll("\\", "\\\\");
  // sEscaped.ReplaceAll("/", "\\/"); // this is not necessary to escape
  sEscaped.ReplaceAll("\"", "\\\"");
  sEscaped.ReplaceAll("\b", "\\b");
  sEscaped.ReplaceAll("\r", "\\r");
  sEscaped.ReplaceAll("\f", "\\f");
  sEscaped.ReplaceAll("\n", "\\n");
  sEscaped.ReplaceAll("\t", "\\t");

  OutputString("\"");
  OutputString(sEscaped.GetData());
  OutputString("\"");
}

void xiiStandardJSONWriter::OutputIndentation()
{
  if (m_WhitespaceMode >= WhitespaceMode::NoIndentation)
    return;

  xiiInt32 iIndentation = m_iIndentation * 2;

  if (m_WhitespaceMode == WhitespaceMode::LessIndentation)
    iIndentation = m_iIndentation;

  xiiStringBuilder s;
  s.Printf("%*s", iIndentation, "");

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteBool(bool value)
{
  CommaWriter cw(this);

  if (value)
    OutputString("true");
  else
    OutputString("false");
}

void xiiStandardJSONWriter::WriteInt32(xiiInt32 value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteUInt32(xiiUInt32 value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteInt64(xiiInt64 value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteUInt64(xiiUInt64 value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  xiiStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void xiiStandardJSONWriter::WriteString(const char* value)
{
  CommaWriter cw(this);

  OutputEscapedString(value);
}

void xiiStandardJSONWriter::WriteNULL()
{
  CommaWriter cw(this);

  OutputString("null");
}

void xiiStandardJSONWriter::WriteTime(xiiTime value)
{
  WriteDouble(value.GetSeconds());
}

void xiiStandardJSONWriter::WriteColor(const xiiColor& value)
{
  xiiVec4 temp(value.r, value.g, value.b, value.a);

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", xiiArgF(value.r, 4), xiiArgF(value.g, 4), xiiArgF(value.b, 4), xiiArgF(value.a, 4));
  else
    s.Format("({0}, {1}, {2}, {3})", xiiArgF(value.r, 4), xiiArgF(value.g, 4), xiiArgF(value.b, 4), xiiArgF(value.a, 4));

  WriteBinaryData("color", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteColorGamma(const xiiColorGammaUB& value)
{
  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", value.r, value.g, value.b, value.a);
  else
    s.Format("({0}, {1}, {2}, {3})", value.r, value.g, value.b, value.a);

  WriteBinaryData("gamma", value.GetData(), sizeof(xiiColorGammaUB), s.GetData());
}

void xiiStandardJSONWriter::WriteVec2(const xiiVec2& value)
{
  xiiVec2 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1})", xiiArgF(value.x, 4), xiiArgF(value.y, 4));
  else
    s.Format("({0}, {1})", xiiArgF(value.x, 4), xiiArgF(value.y, 4));

  WriteBinaryData("vec2", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteVec3(const xiiVec3& value)
{
  xiiVec3 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2})", xiiArgF(value.x, 4), xiiArgF(value.y, 4), xiiArgF(value.z, 4));
  else
    s.Format("({0}, {1}, {2})", xiiArgF(value.x, 4), xiiArgF(value.y, 4), xiiArgF(value.z, 4));

  WriteBinaryData("vec3", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteVec4(const xiiVec4& value)
{
  xiiVec4 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", xiiArgF(value.x, 4), xiiArgF(value.y, 4), xiiArgF(value.z, 4), xiiArgF(value.w, 4));
  else
    s.Format("({0}, {1}, {2}, {3})", xiiArgF(value.x, 4), xiiArgF(value.y, 4), xiiArgF(value.z, 4), xiiArgF(value.w, 4));

  WriteBinaryData("vec4", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteVec2I32(const xiiVec2I32& value)
{
  CommaWriter cw(this);

  xiiVec2I32 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(xiiInt32));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1})", value.x, value.y);
  else
    s.Format("({0}, {1})", value.x, value.y);

  WriteBinaryData("vec2i", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteVec3I32(const xiiVec3I32& value)
{
  CommaWriter cw(this);

  xiiVec3I32 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(xiiInt32));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2})", value.x, value.y, value.z);
  else
    s.Format("({0}, {1}, {2})", value.x, value.y, value.z);

  WriteBinaryData("vec3i", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteVec4I32(const xiiVec4I32& value)
{
  CommaWriter cw(this);

  xiiVec4I32 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(xiiInt32));

  xiiStringBuilder s;

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", value.x, value.y, value.z, value.w);
  else
    s.Format("({0}, {1}, {2}, {3})", value.x, value.y, value.z, value.w);

  WriteBinaryData("vec4i", &temp, sizeof(temp), s.GetData());
}

void xiiStandardJSONWriter::WriteQuat(const xiiQuat& value)
{
  xiiQuat temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("quat", &temp, sizeof(temp));
}

void xiiStandardJSONWriter::WriteMat3(const xiiMat3& value)
{
  xiiMat3 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat3", &temp, sizeof(temp));
}

void xiiStandardJSONWriter::WriteMat4(const xiiMat4& value)
{
  xiiMat4 temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat4", &temp, sizeof(temp));
}

void xiiStandardJSONWriter::WriteUuid(const xiiUuid& value)
{
  CommaWriter cw(this);

  xiiUuid temp = value;

  xiiEndianHelper::NativeToLittleEndian((xiiUInt64*)&temp, sizeof(temp) / sizeof(xiiUInt64));

  WriteBinaryData("uuid", &temp, sizeof(temp));
}

void xiiStandardJSONWriter::WriteAngle(xiiAngle value)
{
  WriteFloat(value.GetDegree());
}

void xiiStandardJSONWriter::WriteDataBuffer(const xiiDataBuffer& value)
{
  WriteBinaryData("data", value.GetData(), value.GetCount());
}

void xiiStandardJSONWriter::BeginVariable(const char* szName)
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV(state == xiiStandardJSONWriter::Empty || state == xiiStandardJSONWriter::Object || state == xiiStandardJSONWriter::NamedObject,
                 "Variables can only be written inside objects.");

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  OutputEscapedString(szName);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString(":");
  else
    OutputString(" : ");

  JSONState s;
  s.m_State = xiiStandardJSONWriter::Variable;
  m_StateStack.PushBack(s);
}

void xiiStandardJSONWriter::EndVariable()
{
  XII_ASSERT_DEV(m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::Variable, "EndVariable() must be called in sync with BeginVariable().");
  XII_ASSERT_DEV(m_StateStack.PeekBack().m_bValueWasWritten, "EndVariable() cannot be called without writing any value in between.");

  End();
}

void xiiStandardJSONWriter::BeginArray(const char* szName)
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV((state == xiiStandardJSONWriter::Empty) ||
                   ((state == xiiStandardJSONWriter::Object || state == xiiStandardJSONWriter::NamedObject) && !xiiStringUtils::IsNullOrEmpty(szName)) ||
                   ((state == xiiStandardJSONWriter::Array || state == xiiStandardJSONWriter::NamedArray) && szName == nullptr) ||
                   (state == xiiStandardJSONWriter::Variable && szName == nullptr),
                 "Inside objects you can only begin arrays when also giving them a (non-empty) name.\n"
                 "Inside arrays you can only nest anonymous arrays, so names are forbidden.\n"
                 "Inside variables you cannot specify a name again.");

  if (szName != nullptr)
    BeginVariable(szName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString(",");
    else
      OutputString(", ");
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("[");
  else
    OutputString("[ ");

  JSONState s;
  s.m_State = (szName == nullptr) ? xiiStandardJSONWriter::Array : xiiStandardJSONWriter::NamedArray;
  m_StateStack.PushBack(s);
  ++m_iIndentation;
}

void xiiStandardJSONWriter::EndArray()
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV(
    state == xiiStandardJSONWriter::Array || state == xiiStandardJSONWriter::NamedArray, "EndArray() must be called in sync with BeginArray().");


  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == xiiStandardJSONWriter::NamedArray)
    EndVariable();
}

void xiiStandardJSONWriter::BeginObject(const char* szName)
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV((state == xiiStandardJSONWriter::Empty) ||
                   ((state == xiiStandardJSONWriter::Object || state == xiiStandardJSONWriter::NamedObject) && !xiiStringUtils::IsNullOrEmpty(szName)) ||
                   ((state == xiiStandardJSONWriter::Array || state == xiiStandardJSONWriter::NamedArray) && szName == nullptr) ||
                   (state == xiiStandardJSONWriter::Variable && szName == nullptr),
                 "Inside objects you can only begin objects when also giving them a (non-empty) name.\n"
                 "Inside arrays you can only nest anonymous objects, so names are forbidden.\n"
                 "Inside variables you cannot specify a name again.");

  if (szName != nullptr)
    BeginVariable(szName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  if (m_WhitespaceMode >= xiiJSONWriter::WhitespaceMode::None)
    OutputString("{");
  else
    OutputString("{\n");

  JSONState s;
  s.m_State = (szName == nullptr) ? xiiStandardJSONWriter::Object : xiiStandardJSONWriter::NamedObject;
  m_StateStack.PushBack(s);
  ++m_iIndentation;

  OutputIndentation();
}

void xiiStandardJSONWriter::EndObject()
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  XII_IGNORE_UNUSED(state);
  XII_ASSERT_DEV(
    state == xiiStandardJSONWriter::Object || state == xiiStandardJSONWriter::NamedObject, "EndObject() must be called in sync with BeginObject().");

  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == xiiStandardJSONWriter::NamedObject)
    EndVariable();
}

void xiiStandardJSONWriter::End()
{
  const xiiStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;

  if (m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::Array || m_StateStack.PeekBack().m_State == xiiStandardJSONWriter::NamedArray)
  {
    --m_iIndentation;

    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("]");
    else
      OutputString(" ]");
  }


  m_StateStack.PopBack();
  m_StateStack.PeekBack().m_bRequireComma = true;

  if (state == xiiStandardJSONWriter::Object || state == xiiStandardJSONWriter::NamedObject)
  {
    --m_iIndentation;

    if (m_WhitespaceMode < xiiJSONWriter::WhitespaceMode::None)
      OutputString("\n");

    OutputIndentation();
    OutputString("}");
  }
}


void xiiStandardJSONWriter::WriteBinaryData(const char* szDataType, const void* pData, xiiUInt32 uiBytes, const char* szValueString)
{
  CommaWriter cw(this);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{\"$t\":\"");
  else
    OutputString("{ \"$t\" : \"");

  OutputString(szDataType);

  if (szValueString != nullptr)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("\",\"$v\":\"");
    else
      OutputString("\", \"$v\" : \"");

    OutputString(szValueString);
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\",\"$b\":\"0x");
  else
    OutputString("\", \"$b\" : \"0x");

  xiiStringBuilder s;

  xiiUInt8* pBytes = (xiiUInt8*)pData;

  for (xiiUInt32 i = 0; i < uiBytes; ++i)
  {
    s.Format("{0}", xiiArgU((xiiUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    OutputString(s.GetData());
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\"}");
  else
    OutputString("\" }");
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StandardJSONWriter);
