/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <MCPPlugin/Json.h>

namespace
{
  class xiiMCPBoundedJsonStream final : public xiiStreamWriter
  {
  public:
    explicit xiiMCPBoundedJsonStream(xiiUInt32 uiMaximumBytes) :
      m_uiMaximumBytes(uiMaximumBytes)
    {
    }

    xiiResult WriteBytes(const void* pData, xiiUInt64 uiBytes) override
    {
      if (uiBytes > m_uiMaximumBytes - m_Bytes.GetCount())
      {
        return XII_FAILURE;
      }
      if (uiBytes != 0)
      {
        m_Bytes.PushBackRange(xiiArrayPtr(static_cast<const char*>(pData), static_cast<xiiUInt32>(uiBytes)));
      }
      return XII_SUCCESS;
    }

    xiiDynamicArray<char> m_Bytes;

  private:
    xiiUInt32 m_uiMaximumBytes;
  };
} // namespace

xiiResult xiiMCPJson::Parse(xiiStringView sJson, xiiVariant& out_value, xiiUInt32 uiMaximumBytes)
{
  if (sJson.IsEmpty() || sJson.GetElementCount() > uiMaximumBytes)
  {
    return XII_FAILURE;
  }

  xiiRawMemoryStreamReader stream(sJson.GetStartPointer(), sJson.GetElementCount());
  xiiMuteLog               log;
  xiiJSONReader            reader;
  reader.SetLogInterface(&log);
  XII_SUCCEED_OR_RETURN(reader.ParseStrict(stream, MaximumDepth));
  switch (reader.GetTopLevelElementType())
  {
    case xiiJSONReader::ElementType::Dictionary:
      out_value = reader.GetTopLevelObject();
      return XII_SUCCESS;
    case xiiJSONReader::ElementType::Array:
      out_value = reader.GetTopLevelArray();
      return XII_SUCCESS;
    case xiiJSONReader::ElementType::Scalar:
      out_value = reader.GetTopLevelScalar();
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

xiiResult xiiMCPJson::Write(const xiiVariant& value, xiiStringBuilder& out_sJson, xiiUInt32 uiMaximumBytes)
{
  xiiMCPBoundedJsonStream stream(uiMaximumBytes);
  xiiStandardJSONWriter   writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(xiiJSONWriter::WhitespaceMode::None);
  XII_SUCCEED_OR_RETURN(writer.WritePlainVariant(value, MaximumDepth));
  out_sJson.Set(xiiStringView(stream.m_Bytes.GetData(), stream.m_Bytes.GetCount()));
  return XII_SUCCESS;
}

xiiStringView xiiMCPJson::GetString(const xiiVariantDictionary& object, xiiStringView sKey)
{
  const auto* pValue = object.GetValue(sKey);
  return pValue && pValue->IsA<xiiString>() ? pValue->Get<xiiString>().GetView() : xiiStringView();
}

const xiiVariantDictionary* xiiMCPJson::GetObject(const xiiVariantDictionary& object, xiiStringView sKey)
{
  const auto* pValue = object.GetValue(sKey);
  return pValue && pValue->IsA<xiiVariantDictionary>() ? &pValue->Get<xiiVariantDictionary>() : nullptr;
}

bool xiiMCPJson::ReadUInt(const xiiVariant& value, xiiUInt64& out_uiValue)
{
  switch (value.GetType())
  {
    case xiiVariantType::Int8:
    case xiiVariantType::Int16:
    case xiiVariantType::Int32:
    case xiiVariantType::Int64:
      if (value.ConvertTo<xiiInt64>() < 0)
      {
        return false;
      }
      [[fallthrough]];
    case xiiVariantType::UInt8:
    case xiiVariantType::UInt16:
    case xiiVariantType::UInt32:
    case xiiVariantType::UInt64:
      out_uiValue = value.ConvertTo<xiiUInt64>();
      return true;
    default:
      return false;
  }
}
