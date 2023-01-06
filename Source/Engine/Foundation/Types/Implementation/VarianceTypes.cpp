#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVarianceTypeBase, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Variance", m_fVariance)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVarianceTypeFloat, xiiVarianceTypeBase, 1, xiiRTTIDefaultAllocator<xiiVarianceTypeFloat>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_Value)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVarianceTypeDouble, xiiVarianceTypeBase, 1, xiiRTTIDefaultAllocator<xiiVarianceTypeDouble>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_Value)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVarianceTypeTime, xiiVarianceTypeBase, 1, xiiRTTIDefaultAllocator<xiiVarianceTypeTime>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_Value)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVarianceTypeAngle, xiiVarianceTypeBase, 1, xiiRTTIDefaultAllocator<xiiVarianceTypeAngle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_Value)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeFloat);
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeDouble);
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeTime);
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiVarianceTypeAngle);

XII_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VarianceTypes);
