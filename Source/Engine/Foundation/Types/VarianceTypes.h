#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/TypeTraits.h>

#define XII_DECLARE_VARIANCE_HASH_HELPER(TYPE)                        \
  template <>                                                         \
  struct xiiHashHelper<TYPE>                                          \
  {                                                                   \
    XII_ALWAYS_INLINE static xiiUInt32 Hash(const TYPE& value)        \
    {                                                                 \
      return xiiHashingUtils::xxHash32(&value, sizeof(TYPE));         \
    }                                                                 \
    XII_ALWAYS_INLINE static bool Equal(const TYPE& a, const TYPE& b) \
    {                                                                 \
      return a == b;                                                  \
    }                                                                 \
  };

struct XII_FOUNDATION_DLL xiiVarianceTypeBase
{
  XII_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiVarianceTypeBase);

struct XII_FOUNDATION_DLL xiiVarianceTypeFloat : public xiiVarianceTypeBase
{
  XII_DECLARE_POD_TYPE();
  bool operator==(const xiiVarianceTypeFloat& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const xiiVarianceTypeFloat& rhs) const
  {
    return !(*this == rhs);
  }
  float m_Value = 0;
};

XII_DECLARE_VARIANCE_HASH_HELPER(xiiVarianceTypeFloat);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiVarianceTypeFloat);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeFloat);

struct XII_FOUNDATION_DLL xiiVarianceTypeTime : public xiiVarianceTypeBase
{
  XII_DECLARE_POD_TYPE();
  bool operator==(const xiiVarianceTypeTime& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const xiiVarianceTypeTime& rhs) const
  {
    return !(*this == rhs);
  }
  xiiTime m_Value;
};

XII_DECLARE_VARIANCE_HASH_HELPER(xiiVarianceTypeTime);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiVarianceTypeTime);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeTime);

struct XII_FOUNDATION_DLL xiiVarianceTypeAngle : public xiiVarianceTypeBase
{
  XII_DECLARE_POD_TYPE();
  bool operator==(const xiiVarianceTypeAngle& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const xiiVarianceTypeAngle& rhs) const
  {
    return !(*this == rhs);
  }
  xiiAngle m_Value;
};

XII_DECLARE_VARIANCE_HASH_HELPER(xiiVarianceTypeAngle);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiVarianceTypeAngle);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeAngle);
