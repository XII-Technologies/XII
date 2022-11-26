

// for some reason MSVC does not accept the template keyword here
#if XII_ENABLED(XII_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) return functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) return functor.template operator()<type>(std::forward<Args>(args)...)
#endif

template <typename Functor, class... Args>
auto xiiVariant::DispatchTo(Functor& functor, Type::Enum type, Args&&... args)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(functor, xiiInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(functor, xiiUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(functor, xiiInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(functor, xiiUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(functor, xiiInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(functor, xiiUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(functor, xiiInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(functor, xiiUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(functor, xiiColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(functor, xiiColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(functor, xiiVec2);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(functor, xiiVec3);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(functor, xiiVec4);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(functor, xiiVec2I32);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(functor, xiiVec3I32);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(functor, xiiVec4I32);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(functor, xiiVec2U32);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(functor, xiiVec3U32);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(functor, xiiVec4U32);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(functor, xiiQuat);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(functor, xiiMat3);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(functor, xiiMat4);
      break;

    case Type::Transform:
      CALL_FUNCTOR(functor, xiiTransform);
      break;

    case Type::String:
      CALL_FUNCTOR(functor, xiiString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(functor, xiiStringView);
      break;

    case Type::DataBuffer:
      CALL_FUNCTOR(functor, xiiDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(functor, xiiTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(functor, xiiUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(functor, xiiAngle);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(functor, xiiVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(functor, xiiVariantDictionary);
      break;

    case Type::TypedObject:
      CALL_FUNCTOR(functor, xiiTypedObject);
      break;

    default:
      XII_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      // Intended fall through to disable warning.
    case Type::TypedPointer:
      CALL_FUNCTOR(functor, xiiTypedPointer);
      break;
  }
}

#undef CALL_FUNCTOR

class xiiVariantHelper
{
  friend class xiiVariant;
  friend struct ConvertFunc;

  template <typename T>
  XII_ALWAYS_INLINE static bool CompareFloat(const xiiVariant& v, const T& other, xiiTraitInt<1>)
  {
    return v.ConvertNumber<double>() == static_cast<double>(other);
  }

  template <typename T>
  XII_ALWAYS_INLINE static bool CompareFloat(const xiiVariant& v, const T& other, xiiTraitInt<0>)
  {
    return false;
  }

  template <typename T>
  XII_ALWAYS_INLINE static bool CompareNumber(const xiiVariant& v, const T& other, xiiTraitInt<1>)
  {
    return v.ConvertNumber<xiiInt64>() == static_cast<xiiInt64>(other);
  }

  template <typename T>
  XII_ALWAYS_INLINE static bool CompareNumber(const xiiVariant& v, const T& other, xiiTraitInt<0>)
  {
    return false;
  }

  static void To(const xiiVariant& value, bool& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<xiiInt32>() != 0;
    else if (value.GetType() == xiiVariant::Type::String)
    {
      if (xiiConversionUtils::StringToBool(value.Cast<xiiString>().GetData(), result) == XII_FAILURE)
      {
        result      = false;
        bSuccessful = false;
      }
    }
    else
      XII_REPORT_FAILURE("Conversion to bool failed");
  }

  static void To(const xiiVariant& value, xiiInt8& result, bool& bSuccessful)
  {
    xiiInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (xiiInt8)tempResult;
  }

  static void To(const xiiVariant& value, xiiUInt8& result, bool& bSuccessful)
  {
    xiiUInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (xiiUInt8)tempResult;
  }

  static void To(const xiiVariant& value, xiiInt16& result, bool& bSuccessful)
  {
    xiiInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (xiiInt16)tempResult;
  }

  static void To(const xiiVariant& value, xiiUInt16& result, bool& bSuccessful)
  {
    xiiUInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (xiiUInt16)tempResult;
  }

  static void To(const xiiVariant& value, xiiInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<xiiInt32>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      if (xiiConversionUtils::StringToInt(value.Cast<xiiString>().GetData(), result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else
      XII_REPORT_FAILURE("Conversion to int failed");
  }

  static void To(const xiiVariant& value, xiiUInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<xiiUInt32>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      xiiInt64 tmp = result;
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiString>().GetData(), tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
        result = (xiiUInt32)tmp;
    }
    else
      XII_REPORT_FAILURE("Conversion to uint failed");
  }

  static void To(const xiiVariant& value, xiiInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<xiiInt64>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiString>().GetData(), result) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
    }
    else
      XII_REPORT_FAILURE("Conversion to int64 failed");
  }

  static void To(const xiiVariant& value, xiiUInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<xiiUInt64>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      xiiInt64 tmp = result;
      if (xiiConversionUtils::StringToInt64(value.Cast<xiiString>().GetData(), tmp) == XII_FAILURE)
      {
        result      = 0;
        bSuccessful = false;
      }
      else
        result = (xiiUInt64)tmp;
    }
    else
      XII_REPORT_FAILURE("Conversion to uint64 failed");
  }

  static void To(const xiiVariant& value, float& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<float>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      double tmp = result;
      if (xiiConversionUtils::StringToFloat(value.Cast<xiiString>().GetData(), tmp) == XII_FAILURE)
      {
        result      = 0.0f;
        bSuccessful = false;
      }
      else
        result = (float)tmp;
    }
    else
      XII_REPORT_FAILURE("Conversion to float failed");
  }

  static void To(const xiiVariant& value, double& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= xiiVariant::Type::Double)
      result = value.ConvertNumber<double>();
    else if (value.GetType() == xiiVariant::Type::String)
    {
      if (xiiConversionUtils::StringToFloat(value.Cast<xiiString>().GetData(), result) == XII_FAILURE)
      {
        result      = 0.0;
        bSuccessful = false;
      }
    }
    else
      XII_REPORT_FAILURE("Conversion to double failed");
  }

  static void To(const xiiVariant& value, xiiString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    ToStringFunc toStringFunc;
    toStringFunc.m_pThis   = &value;
    toStringFunc.m_pResult = &result;

    xiiVariant::DispatchTo(toStringFunc, value.GetType());
    bSuccessful = true;
  }

  static void To(const xiiVariant& value, xiiTypedPointer& result, bool& bSuccessful)
  {
    bSuccessful = true;
    XII_ASSERT_DEBUG(value.GetType() == xiiVariant::Type::TypedPointer, "Only ptr can be converted to void*!");
    result = value.Get<xiiTypedPointer>();
  }

  static void To(const xiiVariant& value, xiiColor& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariant::Type::ColorGamma)
      result = value.Get<xiiColorGammaUB>();
    else
      XII_REPORT_FAILURE("Conversion to xiiColor failed");
  }

  static void To(const xiiVariant& value, xiiColorGammaUB& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == xiiVariant::Type::Color)
      result = value.Get<xiiColor>();
    else
      XII_REPORT_FAILURE("Conversion to xiiColorGammaUB failed");
  }

  template <typename T, typename V1, typename V2>
  static void ToVec2X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec2X failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec2& result, bool& bSuccessful) { ToVec2X<xiiVec2, xiiVec2I32, xiiVec2U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2I32& result, bool& bSuccessful) { ToVec2X<xiiVec2I32, xiiVec2, xiiVec2U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec2U32& result, bool& bSuccessful) { ToVec2X<xiiVec2U32, xiiVec2I32, xiiVec2>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec3X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result =
        T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result =
        T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec3X failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec3& result, bool& bSuccessful) { ToVec3X<xiiVec3, xiiVec3I32, xiiVec3U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3I32& result, bool& bSuccessful) { ToVec3X<xiiVec3I32, xiiVec3, xiiVec3U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec3U32& result, bool& bSuccessful) { ToVec3X<xiiVec3U32, xiiVec3I32, xiiVec3>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec4X(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y),
                 static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result      = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y),
                 static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      XII_REPORT_FAILURE("Conversion to xiiVec4X failed");
      bSuccessful = false;
    }
  }

  static void To(const xiiVariant& value, xiiVec4& result, bool& bSuccessful) { ToVec4X<xiiVec4, xiiVec4I32, xiiVec4U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4I32& result, bool& bSuccessful) { ToVec4X<xiiVec4I32, xiiVec4, xiiVec4U32>(value, result, bSuccessful); }

  static void To(const xiiVariant& value, xiiVec4U32& result, bool& bSuccessful) { ToVec4X<xiiVec4U32, xiiVec4I32, xiiVec4>(value, result, bSuccessful); }

  template <typename T>
  static void To(const xiiVariant& value, T& result, bool& bSuccessful)
  {
    XII_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", xiiVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE void operator()()
    {
      xiiStringBuilder tmp;
      *m_pResult = xiiConversionUtils::ToString(m_pThis->Cast<T>(), tmp);
    }

    const xiiVariant* m_pThis;
    xiiString*        m_pResult;
  };
};
