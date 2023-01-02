#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

xiiJSONWriter::xiiJSONWriter()  = default;
xiiJSONWriter::~xiiJSONWriter() = default;

void xiiJSONWriter::AddVariableBool(const char* szName, bool value)
{
  BeginVariable(szName);
  WriteBool(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableInt32(const char* szName, xiiInt32 value)
{
  BeginVariable(szName);
  WriteInt32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUInt32(const char* szName, xiiUInt32 value)
{
  BeginVariable(szName);
  WriteUInt32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableInt64(const char* szName, xiiInt64 value)
{
  BeginVariable(szName);
  WriteInt64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUInt64(const char* szName, xiiUInt64 value)
{
  BeginVariable(szName);
  WriteUInt64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableFloat(const char* szName, float value)
{
  BeginVariable(szName);
  WriteFloat(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableDouble(const char* szName, double value)
{
  BeginVariable(szName);
  WriteDouble(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableString(const char* szName, const char* value)
{
  BeginVariable(szName);
  WriteString(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableNULL(const char* szName)
{
  BeginVariable(szName);
  WriteNULL();
  EndVariable();
}

void xiiJSONWriter::AddVariableTime(const char* szName, xiiTime value)
{
  BeginVariable(szName);
  WriteTime(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUuid(const char* szName, xiiUuid value)
{
  BeginVariable(szName);
  WriteUuid(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableAngle(const char* szName, xiiAngle value)
{
  BeginVariable(szName);
  WriteAngle(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableColor(const char* szName, const xiiColor& value)
{
  BeginVariable(szName);
  WriteColor(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableColorGamma(const char* szName, const xiiColorGammaUB& value)
{
  BeginVariable(szName);
  WriteColorGamma(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2(const char* szName, const xiiVec2& value)
{
  BeginVariable(szName);
  WriteVec2(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2d(const char* szName, const xiiVec2d& value)
{
  BeginVariable(szName);
  WriteVec2d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3(const char* szName, const xiiVec3& value)
{
  BeginVariable(szName);
  WriteVec3(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3d(const char* szName, const xiiVec3d& value)
{
  BeginVariable(szName);
  WriteVec3d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4(const char* szName, const xiiVec4& value)
{
  BeginVariable(szName);
  WriteVec4(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4d(const char* szName, const xiiVec4d& value)
{
  BeginVariable(szName);
  WriteVec4d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2I32(const char* szName, const xiiVec2I32& value)
{
  BeginVariable(szName);
  WriteVec2I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2I64(const char* szName, const xiiVec2I64& value)
{
  BeginVariable(szName);
  WriteVec2I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3I32(const char* szName, const xiiVec3I32& value)
{
  BeginVariable(szName);
  WriteVec3I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3I64(const char* szName, const xiiVec3I64& value)
{
  BeginVariable(szName);
  WriteVec3I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4I32(const char* szName, const xiiVec4I32& value)
{
  BeginVariable(szName);
  WriteVec4I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4I64(const char* szName, const xiiVec4I64& value)
{
  BeginVariable(szName);
  WriteVec4I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2U32(const char* szName, const xiiVec2U32& value)
{
  BeginVariable(szName);
  WriteVec2U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2U64(const char* szName, const xiiVec2U64& value)
{
  BeginVariable(szName);
  WriteVec2U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3U32(const char* szName, const xiiVec3U32& value)
{
  BeginVariable(szName);
  WriteVec3U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3U64(const char* szName, const xiiVec3U64& value)
{
  BeginVariable(szName);
  WriteVec3U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4U32(const char* szName, const xiiVec4U32& value)
{
  BeginVariable(szName);
  WriteVec4U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4U64(const char* szName, const xiiVec4U64& value)
{
  BeginVariable(szName);
  WriteVec4U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableQuat(const char* szName, const xiiQuat& value)
{
  BeginVariable(szName);
  WriteQuat(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableQuatd(const char* szName, const xiiQuatd& value)
{
  BeginVariable(szName);
  WriteQuatd(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat3(const char* szName, const xiiMat3& value)
{
  BeginVariable(szName);
  WriteMat3(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat3d(const char* szName, const xiiMat3d& value)
{
  BeginVariable(szName);
  WriteMat3d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat4(const char* szName, const xiiMat4& value)
{
  BeginVariable(szName);
  WriteMat4(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat4d(const char* szName, const xiiMat4d& value)
{
  BeginVariable(szName);
  WriteMat4d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableTransform(const char* szName, const xiiTransform& value)
{
  BeginVariable(szName);
  WriteTransform(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableTransformd(const char* szName, const xiiTransformd& value)
{
  BeginVariable(szName);
  WriteTransformd(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableDataBuffer(const char* szName, const xiiDataBuffer& value)
{
  BeginVariable(szName);
  WriteDataBuffer(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVariant(const char* szName, const xiiVariant& value)
{
  BeginVariable(szName);
  WriteVariant(value);
  EndVariable();
}

void xiiJSONWriter::WriteColor(const xiiColor& value)
{
  XII_REPORT_FAILURE("The complex data type xiiColor is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteColorGamma(const xiiColorGammaUB& value)
{
  XII_REPORT_FAILURE("The complex data type xiiColorGammaUB is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2(const xiiVec2& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2d(const xiiVec2d& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3(const xiiVec3& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3d(const xiiVec3d& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4(const xiiVec4& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4d(const xiiVec4d& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2I32(const xiiVec2I32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2I64(const xiiVec2I64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3I32(const xiiVec3I32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3I64(const xiiVec3I64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4I32(const xiiVec4I32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4I64(const xiiVec4I64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2U32(const xiiVec2U32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2U64(const xiiVec2U64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec2U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3U32(const xiiVec3U32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3U64(const xiiVec3U64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec3U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4U32(const xiiVec4U32& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4U64(const xiiVec4U64& value)
{
  XII_REPORT_FAILURE("The complex data type xiiVec4U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteQuat(const xiiQuat& value)
{
  XII_REPORT_FAILURE("The complex data type xiiQuat is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteQuatd(const xiiQuatd& value)
{
  XII_REPORT_FAILURE("The complex data type xiiQuatd is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat3(const xiiMat3& value)
{
  XII_REPORT_FAILURE("The complex data type xiiMat3 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat3d(const xiiMat3d& value)
{
  XII_REPORT_FAILURE("The complex data type xiiMat3d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat4(const xiiMat4& value)
{
  XII_REPORT_FAILURE("The complex data type xiiMat4 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat4d(const xiiMat4d& value)
{
  XII_REPORT_FAILURE("The complex data type xiiMat4d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteTransform(const xiiTransform& value)
{
  XII_REPORT_FAILURE("The complex data type xiiTransform is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteTransformd(const xiiTransformd& value)
{
  XII_REPORT_FAILURE("The complex data type xiiTransformd is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteDataBuffer(const xiiDataBuffer& value)
{
  XII_REPORT_FAILURE("The complex data type xiiDateBuffer is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVariant(const xiiVariant& value)
{
  switch (value.GetType())
  {
    case xiiVariant::Type::Invalid:
      // XII_REPORT_FAILURE("Variant of Type 'Invalid' cannot be written as JSON.");
      WriteNULL();
      return;
    case xiiVariant::Type::Bool:
      WriteBool(value.Get<bool>());
      return;
    case xiiVariant::Type::Int8:
      WriteInt32(value.Get<xiiInt8>());
      return;
    case xiiVariant::Type::UInt8:
      WriteUInt32(value.Get<xiiUInt8>());
      return;
    case xiiVariant::Type::Int16:
      WriteInt32(value.Get<xiiInt16>());
      return;
    case xiiVariant::Type::UInt16:
      WriteUInt32(value.Get<xiiUInt16>());
      return;
    case xiiVariant::Type::Int32:
      WriteInt32(value.Get<xiiInt32>());
      return;
    case xiiVariant::Type::UInt32:
      WriteUInt32(value.Get<xiiUInt32>());
      return;
    case xiiVariant::Type::Int64:
      WriteInt64(value.Get<xiiInt64>());
      return;
    case xiiVariant::Type::UInt64:
      WriteUInt64(value.Get<xiiUInt64>());
      return;
    case xiiVariant::Type::Float:
      WriteFloat(value.Get<float>());
      return;
    case xiiVariant::Type::Double:
      WriteDouble(value.Get<double>());
      return;
    case xiiVariant::Type::Color:
      WriteColor(value.Get<xiiColor>());
      return;
    case xiiVariant::Type::ColorGamma:
      WriteColorGamma(value.Get<xiiColorGammaUB>());
      return;
    case xiiVariant::Type::Vector2:
      WriteVec2(value.Get<xiiVec2>());
      return;
    case xiiVariant::Type::Vector2d:
      WriteVec2d(value.Get<xiiVec2d>());
      return;
    case xiiVariant::Type::Vector3:
      WriteVec3(value.Get<xiiVec3>());
      return;
    case xiiVariant::Type::Vector3d:
      WriteVec3d(value.Get<xiiVec3d>());
      return;
    case xiiVariant::Type::Vector4:
      WriteVec4(value.Get<xiiVec4>());
      return;
    case xiiVariant::Type::Vector4d:
      WriteVec4d(value.Get<xiiVec4d>());
      return;
    case xiiVariant::Type::Vector2I:
      WriteVec2I32(value.Get<xiiVec2I32>());
      return;
    case xiiVariant::Type::Vector2I64:
      WriteVec2I64(value.Get<xiiVec2I64>());
      return;
    case xiiVariant::Type::Vector3I:
      WriteVec3I32(value.Get<xiiVec3I32>());
      return;
    case xiiVariant::Type::Vector3I64:
      WriteVec3I64(value.Get<xiiVec3I64>());
      return;
    case xiiVariant::Type::Vector4I:
      WriteVec4I32(value.Get<xiiVec4I32>());
      return;
    case xiiVariant::Type::Vector4I64:
      WriteVec4I64(value.Get<xiiVec4I64>());
      return;
    case xiiVariant::Type::Vector2U:
      WriteVec2U32(value.Get<xiiVec2U32>());
      return;
    case xiiVariant::Type::Vector2U64:
      WriteVec2U64(value.Get<xiiVec2U64>());
      return;
    case xiiVariant::Type::Vector3U:
      WriteVec3U32(value.Get<xiiVec3U32>());
      return;
    case xiiVariant::Type::Vector3U64:
      WriteVec3U64(value.Get<xiiVec3U64>());
      return;
    case xiiVariant::Type::Vector4U:
      WriteVec4U32(value.Get<xiiVec4U32>());
      return;
    case xiiVariant::Type::Vector4U64:
      WriteVec4U64(value.Get<xiiVec4U64>());
      return;
    case xiiVariant::Type::Quaternion:
      WriteQuat(value.Get<xiiQuat>());
      return;
    case xiiVariant::Type::Quaterniond:
      WriteQuatd(value.Get<xiiQuatd>());
      return;
    case xiiVariant::Type::Matrix3:
      WriteMat3(value.Get<xiiMat3>());
      return;
    case xiiVariant::Type::Matrix3d:
      WriteMat3d(value.Get<xiiMat3d>());
      return;
    case xiiVariant::Type::Matrix4:
      WriteMat4(value.Get<xiiMat4>());
      return;
    case xiiVariant::Type::Matrix4d:
      WriteMat4d(value.Get<xiiMat4d>());
      return;
    case xiiVariant::Type::Transform:
      WriteTransform(value.Get<xiiTransform>());
      return;
    case xiiVariant::Type::Transformd:
      WriteTransformd(value.Get<xiiTransformd>());
      return;
    case xiiVariant::Type::String:
      WriteString(value.Get<xiiString>().GetData());
      return;
    case xiiVariant::Type::StringView:
    {
      xiiStringBuilder s = value.Get<xiiStringView>();
      WriteString(s.GetData());
      return;
    }
    case xiiVariant::Type::Time:
      WriteTime(value.Get<xiiTime>());
      return;
    case xiiVariant::Type::Uuid:
      WriteUuid(value.Get<xiiUuid>());
      return;
    case xiiVariant::Type::Angle:
      WriteAngle(value.Get<xiiAngle>());
      return;
    case xiiVariant::Type::DataBuffer:
      WriteDataBuffer(value.Get<xiiDataBuffer>());
      return;
    case xiiVariant::Type::VariantArray:
    {
      BeginArray();

      const auto& ar = value.Get<xiiVariantArray>();

      for (const auto& val : ar)
      {
        WriteVariant(val);
      }

      EndArray();
    }
      return;

    default:
      break;
  }

  XII_REPORT_FAILURE("The Variant Type {0} is not supported by xiiJSONWriter::WriteVariant.", value.GetType());
}


bool xiiJSONWriter::HadWriteError() const
{
  return m_bHadWriteError;
}

void xiiJSONWriter::SetWriteErrorState()
{
  m_bHadWriteError = true;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONWriter);
