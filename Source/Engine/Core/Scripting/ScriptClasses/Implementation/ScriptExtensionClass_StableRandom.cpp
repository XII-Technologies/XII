#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_StableRandom.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_StableRandom, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(IntMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    XII_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOne, Inout, "Position", In, "Seed"),
    XII_SCRIPT_FUNCTION_PROPERTY(FloatMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    XII_SCRIPT_FUNCTION_PROPERTY(Vec3MinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("StableRandom"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
xiiInt32 xiiScriptExtensionClass_StableRandom::IntMinMax(xiiInt32& inout_iPosition, xiiInt32 iMinValue, xiiInt32 iMaxValue, xiiUInt32 uiSeed)
{
  const xiiSimdVec4i result = xiiSimdVec4i::Truncate(xiiSimdRandom::FloatMinMax(xiiSimdVec4i(inout_iPosition), xiiSimdVec4f((float)iMinValue), xiiSimdVec4f((float)iMaxValue), xiiSimdVec4u(uiSeed)));
  ++inout_iPosition;
  return result.x();
}

// static
float xiiScriptExtensionClass_StableRandom::FloatZeroToOne(xiiInt32& inout_iPosition, xiiUInt32 uiSeed)
{
  const xiiSimdVec4f result = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(inout_iPosition), xiiSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
float xiiScriptExtensionClass_StableRandom::FloatMinMax(xiiInt32& inout_iPosition, float fMinValue, float fMaxValue, xiiUInt32 uiSeed)
{
  const xiiSimdVec4f result = xiiSimdRandom::FloatMinMax(xiiSimdVec4i(inout_iPosition), xiiSimdVec4f(fMinValue), xiiSimdVec4f(fMaxValue), xiiSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
xiiVec3 xiiScriptExtensionClass_StableRandom::Vec3MinMax(xiiInt32& inout_iPosition, const xiiVec3& vMinValue, const xiiVec3& vMaxValue, xiiUInt32 uiSeed)
{
  const xiiSimdVec4i offset(0, 1, 2, 3);
  const xiiSimdVec4f result = xiiSimdRandom::FloatMinMax(xiiSimdVec4i(inout_iPosition) + offset, xiiSimdConversion::ToVec3(vMinValue), xiiSimdConversion::ToVec3(vMaxValue), xiiSimdVec4u(uiSeed));
  inout_iPosition += 4;
  return xiiSimdConversion::ToVec3(result);
}


XII_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_StableRandom);
