#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

xiiStreamReader::xiiStreamReader()  = default;
xiiStreamReader::~xiiStreamReader() = default;

xiiResult xiiStreamReader::ReadString(xiiStringBuilder& builder)
{
  if (auto context = xiiStringDeduplicationReadContext::GetContext())
  {
    builder = context->DeserializeString(*this);
  }
  else
  {
    xiiUInt32 uiCount = 0;
    XII_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      builder.m_Data.Reserve(uiCount + 1);
      builder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(builder.m_Data.GetData(), uiCount);
      builder.m_uiCharacterCount = uiCount;
      builder.AppendTerminator();
    }
    else
    {
      builder.Clear();
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiStreamReader::ReadString(xiiString& string)
{
  xiiStringBuilder tmp;
  const xiiResult  res = ReadString(tmp);
  string               = tmp;

  return res;
}

xiiStreamWriter::xiiStreamWriter()  = default;
xiiStreamWriter::~xiiStreamWriter() = default;

xiiResult xiiStreamWriter::WriteString(const xiiStringView szStringView)
{
  const xiiUInt32 uiCount = szStringView.GetElementCount();

  if (auto context = xiiStringDeduplicationWriteContext::GetContext())
  {
    context->SerializeString(szStringView, *this);
  }
  else
  {
    XII_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      XII_SUCCEED_OR_RETURN(WriteBytes(szStringView.GetStartPointer(), uiCount));
    }
  }

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_Stream);
