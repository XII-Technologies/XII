#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

xiiStreamReader::xiiStreamReader()  = default;
xiiStreamReader::~xiiStreamReader() = default;

xiiResult xiiStreamReader::ReadString(xiiStringBuilder& ref_sBuilder)
{
  if (auto context = xiiStringDeduplicationReadContext::GetContext())
  {
    ref_sBuilder = context->DeserializeString(*this);
  }
  else
  {
    xiiUInt32 uiCount = 0;
    XII_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      ref_sBuilder.m_Data.Reserve(uiCount + 1);
      ref_sBuilder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(ref_sBuilder.m_Data.GetData(), uiCount);
      ref_sBuilder.m_uiCharacterCount = uiCount;
      ref_sBuilder.AppendTerminator();
    }
    else
    {
      ref_sBuilder.Clear();
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiStreamReader::ReadString(xiiString& ref_sString)
{
  xiiStringBuilder tmp;
  const xiiResult  res = ReadString(tmp);
  ref_sString          = tmp;

  return res;
}

xiiStreamWriter::xiiStreamWriter()  = default;
xiiStreamWriter::~xiiStreamWriter() = default;

xiiResult xiiStreamWriter::WriteString(const xiiStringView sStringView)
{
  const xiiUInt32 uiCount = sStringView.GetElementCount();

  if (auto context = xiiStringDeduplicationWriteContext::GetContext())
  {
    context->SerializeString(sStringView, *this);
  }
  else
  {
    XII_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      XII_SUCCEED_OR_RETURN(WriteBytes(sStringView.GetStartPointer(), uiCount));
    }
  }

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_Stream);
