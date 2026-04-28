/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiEnumBase, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBitflagsBase, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectedClass, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// *********************************************
// ***** Standard POD Types for Properties *****

XII_BEGIN_STATIC_REFLECTED_TYPE(bool, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(float, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(double, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt8, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt8, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt16, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt16, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt32, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt32, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiInt64, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUInt64, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiConstCharPtr, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTime, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromNanoseconds, In, "Nanoseconds")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromMicroseconds, In, "Microseconds")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromMilliseconds, In, "Milliseconds")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromSeconds, In, "Seconds")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromMinutes, In, "Minutes")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromHours, In, "Hours")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeZero)->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(AsFloatInSeconds),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColor, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("r", r),
    XII_MEMBER_PROPERTY("g", g),
    XII_MEMBER_PROPERTY("b", b),
    XII_MEMBER_PROPERTY("a", a),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float, float, float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
    XII_CONSTRUCTOR_PROPERTY(xiiColorLinearUB),
    XII_CONSTRUCTOR_PROPERTY(xiiColorGammaUB),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeRGBA, In, "R", In, "G", In, "B", In, "A")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeHSV, In, "Hue", In, "Saturation", In, "Value")->AddFlags(xiiPropertyFlags::PureFunction),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorBaseUB, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("r", r),
    XII_MEMBER_PROPERTY("g", g),
    XII_MEMBER_PROPERTY("b", b),
    XII_MEMBER_PROPERTY("a", a),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorGammaUB, xiiColorBaseUB, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(const xiiColor&),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorLinearUB, xiiColorBaseUB, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt8, xiiUInt8, xiiUInt8, xiiUInt8),
    XII_CONSTRUCTOR_PROPERTY(const xiiColor&),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2d, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(double),
    XII_CONSTRUCTOR_PROPERTY(double, double),
    XII_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    XII_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    XII_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float),
    XII_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y", In, "Z")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    XII_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    XII_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    XII_SCRIPT_FUNCTION_PROPERTY(CrossRH, In, "v"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3d, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(double),
    XII_CONSTRUCTOR_PROPERTY(double, double, double),
    XII_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y", In, "Z")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLength<double>),
    XII_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    XII_SCRIPT_FUNCTION_PROPERTY(GetNormalized<double>),
    XII_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    XII_SCRIPT_FUNCTION_PROPERTY(CrossRH, In, "v"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float),
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4d, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(double),
    XII_CONSTRUCTOR_PROPERTY(double, double, double, double),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2I64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiInt64, xiiInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3I64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiInt64, xiiInt64, xiiInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4I32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiInt32, xiiInt32, xiiInt32, xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4I64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiInt64, xiiInt64, xiiInt64, xiiInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec2U64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64, xiiUInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec3U64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64, xiiUInt64, xiiUInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4U32, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32, xiiUInt32, xiiUInt32, xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVec4U64, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64),
    XII_CONSTRUCTOR_PROPERTY(xiiUInt64, xiiUInt64, xiiUInt64, xiiUInt64),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiQuat, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(float, float, float, float),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromAxisAndAngle, In, "Axis", In, "Angle")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeShortestRotation, In, "DirFrom", In, "DirTo")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeSlerp, In, "From", In, "To", In, "Lerp")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(GetInverse),
    XII_SCRIPT_FUNCTION_PROPERTY(Rotate, In, "v"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiQuatd, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("x", x),
    XII_MEMBER_PROPERTY("y", y),
    XII_MEMBER_PROPERTY("z", z),
    XII_MEMBER_PROPERTY("w", w),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(double, double, double, double),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeFromAxisAndAngle, In, "Axis", In, "Angle")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeShortestRotation, In, "DirFrom", In, "DirTo")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeSlerp, In, "From", In, "To", In, "Lerp")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(GetInverse),
    XII_SCRIPT_FUNCTION_PROPERTY(Rotate, In, "v"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat3, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat3d, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat4, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMat4d, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTransform, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Position", m_vPosition),
    XII_MEMBER_PROPERTY("Rotation", m_qRotation),
    XII_MEMBER_PROPERTY("Scale", m_vScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiVec3, xiiQuat),
    XII_CONSTRUCTOR_PROPERTY(xiiVec3, xiiQuat, xiiVec3),
    XII_SCRIPT_FUNCTION_PROPERTY(Make, In, "Position", In, "Rotation", In, "Scale")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeLocalTransform, In, "Parent", In, "GlobalChild")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeGlobalTransform, In, "Parent", In, "LocalChild")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(TransformPosition, In, "Position"),
    XII_SCRIPT_FUNCTION_PROPERTY(TransformDirection, In, "Direction"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTransformd, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Position", m_vPosition),
    XII_MEMBER_PROPERTY("Rotation", m_qRotation),
    XII_MEMBER_PROPERTY("Scale", m_vScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiVec3d, xiiQuatd),
    XII_CONSTRUCTOR_PROPERTY(xiiVec3d, xiiQuatd, xiiVec3d),
    XII_SCRIPT_FUNCTION_PROPERTY(Make, In, "Position", In, "Rotation", In, "Scale")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeLocalTransform, In, "Parent", In, "GlobalChild")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(MakeGlobalTransform, In, "Parent", In, "LocalChild")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(TransformPosition, In, "Position"),
    XII_SCRIPT_FUNCTION_PROPERTY(TransformDirection, In, "Direction"),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiBasisAxis, 1)
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveX),
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveY),
XII_ENUM_CONSTANT(xiiBasisAxis::PositiveZ),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeX),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeY),
XII_ENUM_CONSTANT(xiiBasisAxis::NegativeZ),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUuid, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVariant, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVariantArray, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVariantDictionary, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiUntrackedString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiStringView, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiStringBuilder, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiHashedString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTempHashedString, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDataBuffer, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAngle, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(MakeFromDegree),
    XII_FUNCTION_PROPERTY(MakeFromRadian),
    XII_FUNCTION_PROPERTY(GetNormalizedRange),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAngled, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(MakeFromDegree),
    XII_FUNCTION_PROPERTY(MakeFromRadian),
    XII_FUNCTION_PROPERTY(GetNormalizedRange),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDoubleInterval, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Start", m_StartValue),
    XII_MEMBER_PROPERTY("End", m_EndValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFloatInterval, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Start", m_StartValue),
    XII_MEMBER_PROPERTY("End", m_EndValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiIntInterval, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Start", m_StartValue),
    XII_MEMBER_PROPERTY("End", m_EndValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiTypeFlags, 1)
XII_BITFLAGS_CONSTANTS(xiiTypeFlags::StandardType, xiiTypeFlags::IsEnum, xiiTypeFlags::Bitflags, xiiTypeFlags::Class, xiiTypeFlags::Abstract, xiiTypeFlags::Phantom, xiiTypeFlags::Minimal)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiPropertyFlags, 1)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::StandardType, xiiPropertyFlags::IsEnum, xiiPropertyFlags::Bitflags, xiiPropertyFlags::Class)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::Const, xiiPropertyFlags::Reference, xiiPropertyFlags::Pointer)
XII_BITFLAGS_CONSTANTS(xiiPropertyFlags::PointerOwner, xiiPropertyFlags::ReadOnly, xiiPropertyFlags::Hidden, xiiPropertyFlags::Phantom)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiFunctionType, 1)
XII_ENUM_CONSTANTS(xiiFunctionType::Member, xiiFunctionType::StaticMember, xiiFunctionType::Constructor)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVariantType, 1)
XII_ENUM_CONSTANTS(xiiVariantType::Invalid, xiiVariantType::Bool, xiiVariantType::Int8, xiiVariantType::UInt8, xiiVariantType::Int16, xiiVariantType::UInt16)
XII_ENUM_CONSTANTS(xiiVariantType::Int32, xiiVariantType::UInt32, xiiVariantType::Int64, xiiVariantType::UInt64, xiiVariantType::Float, xiiVariantType::Double)
XII_ENUM_CONSTANTS(xiiVariantType::Color, xiiVariantType::Vector2I, xiiVariantType::Vector3I, xiiVariantType::Vector4I)
XII_ENUM_CONSTANTS(xiiVariantType::Vector2I64, xiiVariantType::Vector3I64, xiiVariantType::Vector4I64)
XII_ENUM_CONSTANTS(xiiVariantType::Vector2U, xiiVariantType::Vector3U, xiiVariantType::Vector4U)
XII_ENUM_CONSTANTS(xiiVariantType::Vector2U64, xiiVariantType::Vector3U64, xiiVariantType::Vector4U64)
XII_ENUM_CONSTANTS(xiiVariantType::Vector2, xiiVariantType::Vector3, xiiVariantType::Vector4)
XII_ENUM_CONSTANTS(xiiVariantType::Vector2d, xiiVariantType::Vector3d, xiiVariantType::Vector4d)
XII_ENUM_CONSTANTS(xiiVariantType::Quaternion, xiiVariantType::Quaterniond, xiiVariantType::Matrix3, xiiVariantType::Matrix3d)
XII_ENUM_CONSTANTS(xiiVariantType::Matrix4, xiiVariantType::Matrix4d, xiiVariantType::Transform, xiiVariantType::Transformd)
XII_ENUM_CONSTANTS(xiiVariantType::Angle, xiiVariantType::Angled, xiiVariantType::String, xiiVariantType::StringView, xiiVariantType::HashedString)
XII_ENUM_CONSTANTS(xiiVariantType::TempHashedString, xiiVariantType::DataBuffer, xiiVariantType::Time, xiiVariantType::Uuid, xiiVariantType::ColorGamma)
XII_ENUM_CONSTANTS(xiiVariantType::VariantArray, xiiVariantType::VariantDictionary, xiiVariantType::TypedPointer, xiiVariantType::TypedObject)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPropertyCategory, 1)
XII_ENUM_CONSTANTS(xiiPropertyCategory::Constant, xiiPropertyCategory::Member, xiiPropertyCategory::Function, xiiPropertyCategory::Array, xiiPropertyCategory::Set, xiiPropertyCategory::Map)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);
