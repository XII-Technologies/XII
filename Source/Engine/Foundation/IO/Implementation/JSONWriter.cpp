/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

xiiJSONWriter::xiiJSONWriter()  = default;
xiiJSONWriter::~xiiJSONWriter() = default;

void xiiJSONWriter::AddVariableBool(xiiStringView sName, bool value)
{
  BeginVariable(sName);
  WriteBool(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableInt32(xiiStringView sName, xiiInt32 value)
{
  BeginVariable(sName);
  WriteInt32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUInt32(xiiStringView sName, xiiUInt32 value)
{
  BeginVariable(sName);
  WriteUInt32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableInt64(xiiStringView sName, xiiInt64 value)
{
  BeginVariable(sName);
  WriteInt64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUInt64(xiiStringView sName, xiiUInt64 value)
{
  BeginVariable(sName);
  WriteUInt64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableFloat(xiiStringView sName, float value)
{
  BeginVariable(sName);
  WriteFloat(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableDouble(xiiStringView sName, double value)
{
  BeginVariable(sName);
  WriteDouble(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableString(xiiStringView sName, xiiStringView value)
{
  BeginVariable(sName);
  WriteString(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableNULL(xiiStringView sName)
{
  BeginVariable(sName);
  WriteNULL();
  EndVariable();
}

void xiiJSONWriter::AddVariableTime(xiiStringView sName, xiiTime value)
{
  BeginVariable(sName);
  WriteTime(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableUuid(xiiStringView sName, xiiUuid value)
{
  BeginVariable(sName);
  WriteUuid(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableAngle(xiiStringView sName, xiiAngle value)
{
  BeginVariable(sName);
  WriteAngle(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableAngle(xiiStringView sName, xiiAngled value)
{
  BeginVariable(sName);
  WriteAngle(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableColor(xiiStringView sName, const xiiColor& value)
{
  BeginVariable(sName);
  WriteColor(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableColorGamma(xiiStringView sName, const xiiColorGammaUB& value)
{
  BeginVariable(sName);
  WriteColorGamma(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2(xiiStringView sName, const xiiVec2& value)
{
  BeginVariable(sName);
  WriteVec2(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2d(xiiStringView sName, const xiiVec2d& value)
{
  BeginVariable(sName);
  WriteVec2d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3(xiiStringView sName, const xiiVec3& value)
{
  BeginVariable(sName);
  WriteVec3(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3d(xiiStringView sName, const xiiVec3d& value)
{
  BeginVariable(sName);
  WriteVec3d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4(xiiStringView sName, const xiiVec4& value)
{
  BeginVariable(sName);
  WriteVec4(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4d(xiiStringView sName, const xiiVec4d& value)
{
  BeginVariable(sName);
  WriteVec4d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2I32(xiiStringView sName, const xiiVec2I32& value)
{
  BeginVariable(sName);
  WriteVec2I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2I64(xiiStringView sName, const xiiVec2I64& value)
{
  BeginVariable(sName);
  WriteVec2I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3I32(xiiStringView sName, const xiiVec3I32& value)
{
  BeginVariable(sName);
  WriteVec3I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3I64(xiiStringView sName, const xiiVec3I64& value)
{
  BeginVariable(sName);
  WriteVec3I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4I32(xiiStringView sName, const xiiVec4I32& value)
{
  BeginVariable(sName);
  WriteVec4I32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4I64(xiiStringView sName, const xiiVec4I64& value)
{
  BeginVariable(sName);
  WriteVec4I64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2U32(xiiStringView sName, const xiiVec2U32& value)
{
  BeginVariable(sName);
  WriteVec2U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec2U64(xiiStringView sName, const xiiVec2U64& value)
{
  BeginVariable(sName);
  WriteVec2U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3U32(xiiStringView sName, const xiiVec3U32& value)
{
  BeginVariable(sName);
  WriteVec3U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec3U64(xiiStringView sName, const xiiVec3U64& value)
{
  BeginVariable(sName);
  WriteVec3U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4U32(xiiStringView sName, const xiiVec4U32& value)
{
  BeginVariable(sName);
  WriteVec4U32(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVec4U64(xiiStringView sName, const xiiVec4U64& value)
{
  BeginVariable(sName);
  WriteVec4U64(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableQuat(xiiStringView sName, const xiiQuat& value)
{
  BeginVariable(sName);
  WriteQuat(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableQuatd(xiiStringView sName, const xiiQuatd& value)
{
  BeginVariable(sName);
  WriteQuatd(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat3(xiiStringView sName, const xiiMat3& value)
{
  BeginVariable(sName);
  WriteMat3(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat3d(xiiStringView sName, const xiiMat3d& value)
{
  BeginVariable(sName);
  WriteMat3d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat4(xiiStringView sName, const xiiMat4& value)
{
  BeginVariable(sName);
  WriteMat4(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableMat4d(xiiStringView sName, const xiiMat4d& value)
{
  BeginVariable(sName);
  WriteMat4d(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableTransform(xiiStringView sName, const xiiTransform& value)
{
  BeginVariable(sName);
  WriteTransform(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableTransformd(xiiStringView sName, const xiiTransformd& value)
{
  BeginVariable(sName);
  WriteTransformd(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableDataBuffer(xiiStringView sName, const xiiDataBuffer& value)
{
  BeginVariable(sName);
  WriteDataBuffer(value);
  EndVariable();
}

void xiiJSONWriter::AddVariableVariant(xiiStringView sName, const xiiVariant& value)
{
  BeginVariable(sName);
  WriteVariant(value);
  EndVariable();
}

void xiiJSONWriter::WriteColor(const xiiColor& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiColor is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteColorGamma(const xiiColorGammaUB& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiColorGammaUB is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2(const xiiVec2& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2d(const xiiVec2d& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3(const xiiVec3& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3d(const xiiVec3d& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4(const xiiVec4& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4d(const xiiVec4d& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2I32(const xiiVec2I32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2I64(const xiiVec2I64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3I32(const xiiVec3I32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3I64(const xiiVec3I64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4I32(const xiiVec4I32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4I32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4I64(const xiiVec4I64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4I64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2U32(const xiiVec2U32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec2U64(const xiiVec2U64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec2U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3U32(const xiiVec3U32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec3U64(const xiiVec3U64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec3U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4U32(const xiiVec4U32& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4U32 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteVec4U64(const xiiVec4U64& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiVec4U64 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteQuat(const xiiQuat& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiQuat is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteQuatd(const xiiQuatd& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiQuatd is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat3(const xiiMat3& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiMat3 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat3d(const xiiMat3d& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiMat3d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat4(const xiiMat4& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiMat4 is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteMat4d(const xiiMat4d& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiMat4d is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteTransform(const xiiTransform& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiTransform is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteTransformd(const xiiTransformd& value)
{
  XII_IGNORE_UNUSED(value);

  XII_REPORT_FAILURE("The complex data type xiiTransformd is not supported by this JSON writer.");
}

void xiiJSONWriter::WriteDataBuffer(const xiiDataBuffer& value)
{
  XII_IGNORE_UNUSED(value);

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
      WriteString(value.Get<xiiString>());
      return;
    case xiiVariant::Type::StringView:
    {
      xiiStringBuilder s = value.Get<xiiStringView>();
      WriteString(s);
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
    case xiiVariant::Type::Angled:
      WriteAngle(value.Get<xiiAngled>());
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
      return;
    }
    case xiiVariant::Type::VariantDictionary:
    {
      BeginObject();

      const auto& dictionary = value.Get<xiiVariantDictionary>();

      for (auto& element : dictionary)
      {
        AddVariableVariant(element.Key(), element.Value());
      }
      EndObject();
      return;
    }

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
