#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Types/Variant.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace xiiRmlUiConversionUtils
{
  xiiVariant ToVariant(const Rml::Variant& value, xiiVariant::Type::Enum targetType /*= xiiVariant::Type::Invalid*/)
  {
    xiiVariant result;

    switch (value.GetType())
    {
      case Rml::Variant::BOOL:
        result = value.Get<bool>();
        break;

      case Rml::Variant::CHAR:
        result = value.Get<char>();
        break;

      case Rml::Variant::BYTE:
        result = value.Get<Rml::byte>();
        break;

      case Rml::Variant::INT:
        result = value.Get<int>();
        break;

      case Rml::Variant::INT64:
        result = value.Get<xiiInt64>();
        break;

      case Rml::Variant::FLOAT:
        result = value.Get<float>();
        break;

      case Rml::Variant::DOUBLE:
        result = value.Get<double>();
        break;

      case Rml::Variant::STRING:
        result = value.Get<Rml::String>().c_str();
        break;

      default:
        break;
    }

    if (targetType != xiiVariant::Type::Invalid && result.IsValid())
    {
      xiiResult conversionResult = XII_SUCCESS;
      result                     = result.ConvertTo(targetType, &conversionResult);

      if (conversionResult.Failed())
      {
        xiiLog::Warning("Failed to convert rml variant to target type '{}'", targetType);
      }
    }

    return result;
  }

  Rml::Variant ToVariant(const xiiVariant& value)
  {
    switch (value.GetType())
    {
      case xiiVariant::Type::Invalid:
        return Rml::Variant("<Invalid>");

      case xiiVariant::Type::Bool:
        return Rml::Variant(value.Get<bool>());

      case xiiVariant::Type::Int8:
        return Rml::Variant(value.Get<xiiInt8>());

      case xiiVariant::Type::UInt8:
        return Rml::Variant(value.Get<xiiUInt8>());

      case xiiVariant::Type::Int16:
      case xiiVariant::Type::UInt16:
      case xiiVariant::Type::Int32:
        return Rml::Variant(value.ConvertTo<int>());

      case xiiVariant::Type::UInt32:
      case xiiVariant::Type::Int64:
        return Rml::Variant(static_cast<int64_t>(value.ConvertTo<xiiInt64>()));

      case xiiVariant::Type::Float:
        return Rml::Variant(value.Get<float>());

      case xiiVariant::Type::Double:
        return Rml::Variant(value.Get<double>());

      case xiiVariant::Type::String:
        return Rml::Variant(value.Get<xiiString>());

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return Rml::Variant();
    }
  }

} // namespace xiiRmlUiConversionUtils
