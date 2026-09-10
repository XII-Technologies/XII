/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#define CALL_FUNCTOR(functor, type) return functor.template operator()<type>(std::forward<Args>(args)...)

template <typename Functor, class... Args>
auto xiiVariant::DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(ref_functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(ref_functor, xiiInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(ref_functor, xiiUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(ref_functor, xiiInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(ref_functor, xiiUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(ref_functor, xiiInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(ref_functor, xiiUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(ref_functor, xiiInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(ref_functor, xiiUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(ref_functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(ref_functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(ref_functor, xiiColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(ref_functor, xiiColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(ref_functor, xiiVec2);
      break;

    case Type::Vector2d:
      CALL_FUNCTOR(ref_functor, xiiVec2d);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(ref_functor, xiiVec3);
      break;

    case Type::Vector3d:
      CALL_FUNCTOR(ref_functor, xiiVec3d);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(ref_functor, xiiVec4);
      break;

    case Type::Vector4d:
      CALL_FUNCTOR(ref_functor, xiiVec4d);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(ref_functor, xiiVec2I32);
      break;

    case Type::Vector2I64:
      CALL_FUNCTOR(ref_functor, xiiVec2I64);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(ref_functor, xiiVec3I32);
      break;

    case Type::Vector3I64:
      CALL_FUNCTOR(ref_functor, xiiVec3I64);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(ref_functor, xiiVec4I32);
      break;

    case Type::Vector4I64:
      CALL_FUNCTOR(ref_functor, xiiVec4I64);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(ref_functor, xiiVec2U32);
      break;

    case Type::Vector2U64:
      CALL_FUNCTOR(ref_functor, xiiVec2U64);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(ref_functor, xiiVec3U32);
      break;

    case Type::Vector3U64:
      CALL_FUNCTOR(ref_functor, xiiVec3U64);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(ref_functor, xiiVec4U32);
      break;

    case Type::Vector4U64:
      CALL_FUNCTOR(ref_functor, xiiVec4U64);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(ref_functor, xiiQuat);
      break;

    case Type::Quaterniond:
      CALL_FUNCTOR(ref_functor, xiiQuatd);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(ref_functor, xiiMat3);
      break;

    case Type::Matrix3d:
      CALL_FUNCTOR(ref_functor, xiiMat3d);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(ref_functor, xiiMat4);
      break;

    case Type::Matrix4d:
      CALL_FUNCTOR(ref_functor, xiiMat4d);
      break;

    case Type::Transform:
      CALL_FUNCTOR(ref_functor, xiiTransform);
      break;

    case Type::Transformd:
      CALL_FUNCTOR(ref_functor, xiiTransformd);
      break;

    case Type::String:
      CALL_FUNCTOR(ref_functor, xiiString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(ref_functor, xiiStringView);
      break;

    case Type::HashedString:
      CALL_FUNCTOR(ref_functor, xiiHashedString);
      break;

    case Type::TempHashedString:
      CALL_FUNCTOR(ref_functor, xiiTempHashedString);
      break;


    case Type::DataBuffer:
      CALL_FUNCTOR(ref_functor, xiiDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(ref_functor, xiiTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(ref_functor, xiiUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(ref_functor, xiiAngle);
      break;

    case Type::Angled:
      CALL_FUNCTOR(ref_functor, xiiAngled);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(ref_functor, xiiVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(ref_functor, xiiVariantDictionary);
      break;

    case Type::TypedObject:
      CALL_FUNCTOR(ref_functor, xiiTypedObject);
      break;

    default:
      XII_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      // Intended fall through to disable warning.
      [[fallthrough]];
    case Type::TypedPointer:
      CALL_FUNCTOR(ref_functor, xiiTypedPointer);
      break;
  }
}

#undef CALL_FUNCTOR

class xiiVariantHelper
{
  friend class xiiVariant;
  friend struct ConvertFunc;

  static void To(const xiiVariant& value, bool& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<xiiInt32>() != 0;
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      if (xiiConversionUtils::StringToBool(s, result) == XII_FAILURE)
      {
        result      = false;
        bSuccessful = false;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      if (xiiConversionUtils::StringToBool(value.Cast<xiiStringView>(), result) == XII_FAILURE)
      {
        result      = false;
        bSuccessful = false;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to bool failed");
    }
  }

  static void To(const xiiVariant& value, xiiInt8& result, bool& bSuccessful)
  {
    xiiInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (xiiInt8)tempResult;
  }

  static void To(const xiiVariant& value, xiiUInt8& result, bool& bSuccessful)
  {
    xiiUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (xiiUInt8)tempResult;
  }

  static void To(const xiiVariant& value, xiiInt16& result, bool& bSuccessful)
  {
    xiiInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (xiiInt16)tempResult;
  }

  static void To(const xiiVariant& value, xiiUInt16& result, bool& bSuccessful)
  {
    xiiUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (xiiUInt16)tempResult;
  }

  static void To(const xiiVariant& value, xiiInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<xiiInt32>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      if (xiiConversionUtils::StringToInt(s, result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      if (xiiConversionUtils::StringToInt(value.Cast<xiiStringView>(), result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to int failed");
    }
  }

  static void To(const xiiVariant& value, xiiUInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<xiiUInt32>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s   = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      xiiInt64      tmp = result;
      if (xiiConversionUtils::StringToInt64(s, tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
      {
        result = (xiiUInt32)tmp;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      xiiInt64 tmp = result;
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiStringView>(), tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
      {
        result = (xiiUInt32)tmp;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to uint failed");
    }
  }

  static void To(const xiiVariant& value, xiiInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<xiiInt64>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      if (xiiConversionUtils::StringToInt64(s, result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiStringView>(), result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to int64 failed");
    }
  }

  static void To(const xiiVariant& value, xiiUInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<xiiUInt64>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s   = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      xiiInt64      tmp = result;
      if (xiiConversionUtils::StringToInt64(s, tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
      {
        result = (xiiUInt64)tmp;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      xiiInt64 tmp = result;
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiStringView>(), tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
      {
        result = (xiiUInt64)tmp;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to uint64 failed");
    }
  }

  static void To(const xiiVariant& value, float& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<float>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s   = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      double        tmp = result;
      if (xiiConversionUtils::StringToFloat(s, tmp) == XII_FAILURE)
      {
        result      = 0.0f;
        bSuccessful = false;
      }
      else
      {
        result = (float)tmp;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      double tmp = result;
      if (xiiConversionUtils::StringToFloat(value.Cast<xiiStringView>(), tmp) == XII_FAILURE)
      {
        result      = 0.0f;
        bSuccessful = false;
      }
      else
      {
        result = (float)tmp;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to float failed");
    }
  }

  static void To(const xiiVariant& value, double& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
    {
      result = value.ConvertNumber<double>();
    }
    else if (value.GetType() == xiiVariant::Type::String || value.GetType() == xiiVariant::Type::HashedString)
    {
      xiiStringView s = value.IsA<xiiString>() ? value.Cast<xiiString>().GetView() : value.Cast<xiiHashedString>().GetView();
      if (xiiConversionUtils::StringToFloat(s, result) == XII_FAILURE)
      {
        result      = 0.0;
        bSuccessful = false;
      }
    }
    else if (value.GetType() == xiiVariant::Type::StringView)
    {
      if (xiiConversionUtils::StringToFloat(value.Cast<xiiStringView>(), result) == XII_FAILURE)
      {
        result      = 0.0;
        bSuccessful = false;
      }
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to double failed");
    }
  }

  static void To(const xiiVariant& value, xiiString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsValid() == false)
    {
      result = "<Invalid>";
      return;
    }

    ToStringFunc toStringFunc;
    toStringFunc.m_pThis   = &value;
    toStringFunc.m_pResult = &result;

    xiiVariant::DispatchTo(toStringFunc, value.GetType());
  }

  static void To(const xiiVariant& value, xiiStringView& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<xiiStringView>())
      result = value.Get<xiiStringView>();
    else
      result = value.IsA<xiiString>() ? value.Get<xiiString>().GetView() : value.Get<xiiHashedString>().GetView();
  }

  static void To(const xiiVariant& value, xiiTypedPointer& result, bool& bSuccessful)
  {
    bSuccessful = true;

    XII_ASSERT_DEBUG(value.GetType() == xiiVariant::Type::TypedPointer, "Only ptr can be converted to void*!");

    result = value.Cast<xiiTypedPointer>();
  }

  static void To(const xiiVariant& value, xiiColor& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariant::Type::ColorGamma)
    {
      result = value.Cast<xiiColorGammaUB>();
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiColor failed");
    }
  }

  static void To(const xiiVariant& value, xiiColorGammaUB& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariant::Type::Color)
    {
      result = value.Cast<xiiColor>();
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiColorGammaUB failed");
    }
  }

  template <typename T, typename V1, typename V2, typename V3, typename V4, typename V5>
  static void ToVec2X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V3>())
    {
      const V3& v = value.Cast<V3>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V4>())
    {
      const V4& v = value.Cast<V4>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V5>())
    {
      const V5& v = value.Cast<V5>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec2X failed");

      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec2& result, bool& bSuccessful) { ToVec2X<xiiVec2, xiiVec2I32, xiiVec2U32, xiiVec2d, xiiVec2I64, xiiVec2U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2I32& result, bool& bSuccessful) { ToVec2X<xiiVec2I32, xiiVec2, xiiVec2U32, xiiVec2d, xiiVec2I64, xiiVec2U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2U32& result, bool& bSuccessful) { ToVec2X<xiiVec2U32, xiiVec2I32, xiiVec2, xiiVec2d, xiiVec2I64, xiiVec2U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2d& result, bool& bSuccessful) { ToVec2X<xiiVec2d, xiiVec2I64, xiiVec2U64, xiiVec2, xiiVec2I32, xiiVec2U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2I64& result, bool& bSuccessful) { ToVec2X<xiiVec2I64, xiiVec2d, xiiVec2U64, xiiVec2, xiiVec2I32, xiiVec2U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2U64& result, bool& bSuccessful) { ToVec2X<xiiVec2U64, xiiVec2I64, xiiVec2d, xiiVec2, xiiVec2I32, xiiVec2U32>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2, typename V3, typename V4, typename V5>
  static void ToVec3X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V3>())
    {
      const V3& v = value.Cast<V3>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V4>())
    {
      const V4& v = value.Cast<V4>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V5>())
    {
      const V5& v = value.Cast<V5>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec3X failed");

      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec3& result, bool& bSuccessful) { ToVec3X<xiiVec3, xiiVec3I32, xiiVec3U32, xiiVec3d, xiiVec3I64, xiiVec3U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3I32& result, bool& bSuccessful) { ToVec3X<xiiVec3I32, xiiVec3, xiiVec3U32, xiiVec3d, xiiVec3I64, xiiVec3U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3U32& result, bool& bSuccessful) { ToVec3X<xiiVec3U32, xiiVec3I32, xiiVec3, xiiVec3d, xiiVec3I64, xiiVec3U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3d& result, bool& bSuccessful) { ToVec3X<xiiVec3d, xiiVec3I64, xiiVec3U64, xiiVec3, xiiVec3I32, xiiVec3U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3I64& result, bool& bSuccessful) { ToVec3X<xiiVec3I64, xiiVec3d, xiiVec3U64, xiiVec3, xiiVec3I32, xiiVec3U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3U64& result, bool& bSuccessful) { ToVec3X<xiiVec3U64, xiiVec3I64, xiiVec3d, xiiVec3, xiiVec3I32, xiiVec3U32>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2, typename V3, typename V4, typename V5>
  static void ToVec4X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V3>())
    {
      const V3& v = value.Cast<V3>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V4>())
    {
      const V4& v = value.Cast<V4>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V5>())
    {
      const V5& v = value.Cast<V5>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec4X failed");

      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec4& result, bool& bSuccessful) { ToVec4X<xiiVec4, xiiVec4I32, xiiVec4U32, xiiVec4d, xiiVec4I64, xiiVec4U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4I32& result, bool& bSuccessful) { ToVec4X<xiiVec4I32, xiiVec4, xiiVec4U32, xiiVec4d, xiiVec4I64, xiiVec4U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4U32& result, bool& bSuccessful) { ToVec4X<xiiVec4U32, xiiVec4I32, xiiVec4, xiiVec4d, xiiVec4I64, xiiVec4U64>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4d& result, bool& bSuccessful) { ToVec4X<xiiVec4d, xiiVec4I64, xiiVec4U64, xiiVec4, xiiVec4I32, xiiVec4U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4I64& result, bool& bSuccessful) { ToVec4X<xiiVec4I64, xiiVec4d, xiiVec4U64, xiiVec4, xiiVec4I32, xiiVec4U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4U64& result, bool& bSuccessful) { ToVec4X<xiiVec4U64, xiiVec4I64, xiiVec4d, xiiVec4, xiiVec4I32, xiiVec4U32>(value, result, bSuccessful); }

  template <typename T, typename V1>
  static void ToQuatX(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiQuatX failed");

      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiQuat& result, bool& bSuccessful) { ToQuatX<xiiQuat, xiiQuatd>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiQuatd& result, bool& bSuccessful) { ToQuatX<xiiQuatd, xiiQuat>(value, result, bSuccessful); }

  template <typename T, typename V1>
  static void ToMat3X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.Element(0, 0)), static_cast<typename T::ComponentType>(v.Element(1, 0)), static_cast<typename T::ComponentType>(v.Element(2, 0)),
                      static_cast<typename T::ComponentType>(v.Element(0, 1)), static_cast<typename T::ComponentType>(v.Element(1, 1)), static_cast<typename T::ComponentType>(v.Element(2, 1)),
                      static_cast<typename T::ComponentType>(v.Element(0, 2)), static_cast<typename T::ComponentType>(v.Element(1, 2)), static_cast<typename T::ComponentType>(v.Element(2, 2)));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiMat3X failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiMat3& result, bool& bSuccessful) { ToMat3X<xiiMat3, xiiMat3d>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiMat3d& result, bool& bSuccessful) { ToMat3X<xiiMat3d, xiiMat3>(value, result, bSuccessful); }

  template <typename T, typename V1>
  static void ToMat4X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.Element(0, 0)), static_cast<typename T::ComponentType>(v.Element(1, 0)), static_cast<typename T::ComponentType>(v.Element(2, 0)), static_cast<typename T::ComponentType>(v.Element(3, 0)),
                      static_cast<typename T::ComponentType>(v.Element(0, 1)), static_cast<typename T::ComponentType>(v.Element(1, 1)), static_cast<typename T::ComponentType>(v.Element(2, 1)), static_cast<typename T::ComponentType>(v.Element(3, 1)),
                      static_cast<typename T::ComponentType>(v.Element(0, 2)), static_cast<typename T::ComponentType>(v.Element(1, 2)), static_cast<typename T::ComponentType>(v.Element(2, 2)), static_cast<typename T::ComponentType>(v.Element(3, 2)),
                      static_cast<typename T::ComponentType>(v.Element(0, 3)), static_cast<typename T::ComponentType>(v.Element(1, 3)), static_cast<typename T::ComponentType>(v.Element(2, 3)), static_cast<typename T::ComponentType>(v.Element(3, 3)));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiMat4X failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiMat4& result, bool& bSuccessful) { ToMat4X<xiiMat4, xiiMat4d>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiMat4d& result, bool& bSuccessful) { ToMat4X<xiiMat4d, xiiMat4>(value, result, bSuccessful); }

  template <typename T, typename V1>
  static void ToTransformX(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1&                                        v         = value.Cast<V1>();
      const xiiVec3Template<typename T::ComponentType> vPosition = xiiVec3Template<typename T::ComponentType>(static_cast<typename T::ComponentType>(v.m_vPosition.x), static_cast<typename T::ComponentType>(v.m_vPosition.y), static_cast<typename T::ComponentType>(v.m_vPosition.z));
      const xiiQuatTemplate<typename T::ComponentType> qRotation = xiiQuatTemplate<typename T::ComponentType>(static_cast<typename T::ComponentType>(v.m_qRotation.x), static_cast<typename T::ComponentType>(v.m_qRotation.y), static_cast<typename T::ComponentType>(v.m_qRotation.z), static_cast<typename T::ComponentType>(v.m_qRotation.w));
      const xiiVec3Template<typename T::ComponentType> vScale    = xiiVec3Template<typename T::ComponentType>(static_cast<typename T::ComponentType>(v.m_vScale.x), static_cast<typename T::ComponentType>(v.m_vScale.y), static_cast<typename T::ComponentType>(v.m_vScale.z));
      result                                                     = T(vPosition, qRotation, vScale);
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiTransformX failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiTransform& result, bool& bSuccessful) { ToTransformX<xiiTransform, xiiTransformd>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiTransformd& result, bool& bSuccessful) { ToTransformX<xiiTransformd, xiiTransform>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariantType::String)
    {
      result.Assign(value.Cast<xiiString>());
    }
    else if (value.GetType() == xiiVariantType::StringView)
    {
      result.Assign(value.Cast<xiiStringView>());
    }
    else
    {
      xiiString s;
      To(value, s, bSuccessful);
      result.Assign(s.GetView());
    }
  }

  static void To(const xiiVariant& value, xiiTempHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariantType::String)
    {
      result = value.Cast<xiiString>();
    }
    else if (value.GetType() == xiiVariantType::StringView)
    {
      result = value.Cast<xiiStringView>();
    }
    else if (value.GetType() == xiiVariant::Type::HashedString)
    {
      result = value.Cast<xiiHashedString>();
    }
    else
    {
      xiiString s;
      To(value, s, bSuccessful);
      result = s.GetView();
    }
  }

  template <typename T>
  static void To(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    XII_IGNORE_UNUSED(value);
    XII_IGNORE_UNUSED(result);

    XII_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", xiiVariant::TypeDeduction<T>::value);

    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE void operator()()
    {
      xiiStringBuilder tmp;
      *m_pResult = xiiConversionUtils::ToString(m_pThis->Cast<T>(), tmp);  // NOLINT (clang-analyzer-core.CallAndMessage)
    }

    const xiiVariant* m_pThis   = nullptr;
    xiiString*        m_pResult = nullptr;
  };
};
