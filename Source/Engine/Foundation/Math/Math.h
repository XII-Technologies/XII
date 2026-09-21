/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Constants.h>
#include <Foundation/Math/Declarations.h>

/// This namespace provides common math-functionality as functions.
///
/// It is a namespace, instead of a static class, because that allows it to be extended
/// at other locations, which is especially useful when adding custom types.
namespace xiiMath
{
  /// Returns whether the given value is NaN under this type.
  template <typename Type>
  constexpr static bool IsNaN(Type value)
  {
    XII_IGNORE_UNUSED(value);
    return false;
  }

  /// Returns whether the given value represents a finite value (i.e. not +/- Infinity and not NaN)
  template <typename Type>
  constexpr static bool IsFinite(Type value)
  {
    XII_IGNORE_UNUSED(value);
    return true;
  }

  /// ***** Trigonometric Functions *****

  /// Takes an angle, returns its sine.
  template <typename Type>
  [[nodiscard]] Type Sin(xiiAngleTemplate<Type> a); // [tested]

  /// Takes an angle, returns its cosine.
  template <typename Type>
  [[nodiscard]] Type Cos(xiiAngleTemplate<Type> a); // [tested]

  /// Takes an angle, returns its tangent.
  template <typename Type>
  [[nodiscard]] Type Tan(xiiAngleTemplate<Type> a); // [tested]

  /// Returns the arcus sinus of f.
  template <typename Type>
  [[nodiscard]] xiiAngleTemplate<Type> ASin(Type f); // [tested]

  /// Returns the arcus cosinus of f.
  template <typename Type>
  [[nodiscard]] xiiAngleTemplate<Type> ACos(Type f); // [tested]

  /// Returns the arcus tangent of f.
  template <typename Type>
  [[nodiscard]] xiiAngleTemplate<Type> ATan(Type f); // [tested]

  /// Returns the atan2 of x and y.
  template <typename Type>
  [[nodiscard]] xiiAngleTemplate<Type> ATan2(Type y, Type x); // [tested]

  /// Returns e^f
  [[nodiscard]] float Exp(float f); // [tested]

  /// Returns e^f
  [[nodiscard]] double Exp(double f); // [tested]

  /// Returns the logarithmus naturalis of f
  [[nodiscard]] float Ln(float f); // [tested]

  /// Returns the logarithmus naturalis of f
  [[nodiscard]] double Ln(double f); // [tested]

  /// Returns log (f), to the base 2
  [[nodiscard]] float Log2(float f); // [tested]

  /// Returns log (f), to the base 2
  [[nodiscard]] double Log2(double f); // [tested]

  /// Returns the integral logarithm to the base 2, that comes closest to the given integer.
  [[nodiscard]] xiiUInt32 Log2i(xiiUInt32 uiVal); // [tested]

  /// Returns log (f), to the base 10
  [[nodiscard]] float Log10(float f); // [tested]

  /// Returns log (f), to the base 10
  [[nodiscard]] double Log10(double f); // [tested]

  /// Returns log (f), to the base fBase
  [[nodiscard]] float Log(float fBase, float f); // [tested]

  /// Returns log (f), to the base fBase
  [[nodiscard]] double Log(double fBase, double f); // [tested]

  /// Returns 2^f
  [[nodiscard]] float Pow2(float f); // [tested]

  /// Returns 2^f
  [[nodiscard]] double Pow2(double f); // [tested]

  /// Returns base^exp
  [[nodiscard]] float Pow(float fBase, float fExp); // [tested]

  /// Returns base^exp
  [[nodiscard]] double Pow(double fBase, double fExp); // [tested]

  /// Returns 2^f
  [[nodiscard]] constexpr xiiInt32 Pow2(xiiInt32 i); // [tested]

  /// Returns base^exp
  [[nodiscard]] xiiInt32 Pow(xiiInt32 iBase, xiiInt32 iExp); // [tested]

  /// Returns f * f
  template <typename T>
  [[nodiscard]] constexpr T Square(T f); // [tested]

  /// Returns the square root of f
  [[nodiscard]] float Sqrt(float f); // [tested]

  /// Returns the square root of f
  [[nodiscard]] double Sqrt(double f); // [tested]

  /// Returns the n-th root of f.
  [[nodiscard]] float Root(float f, float fNthRoot); // [tested]

  /// Returns the sign of f (i.e: -1, 1 or 0)
  template <typename T>
  [[nodiscard]] constexpr T Sign(T f); // [tested]

  /// Returns the absolute value of f
  template <typename T>
  [[nodiscard]] constexpr T Abs(T f); // [tested]

  /// Returns the smaller value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Min(T f1, T f2); // [tested]

  /// Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// Returns the greater value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Max(T f1, T f2); // [tested]

  /// Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Clamp(T value, T minValue, T maxValue); // [tested]

  /// Wraps uiValue around the maximum value, so that it stays within the range [0; uiExcludedMaxValue-1].
  ///
  /// Ie a value of uiExcludedMaxValue would be wrapped to 0, and (uiExcludedMaxValue+1) to 1, etc.
  /// A value of 0 for uiExcludedMaxValue is invalid and results in a division by zero error.
  [[nodiscard]] constexpr xiiUInt32 WrapUInt(xiiUInt32 uiValue, xiiUInt32 uiExcludedMaxValue); // [tested]

  /// Wraps iValue around the maximum value, so that it stays within the range [0; uiExcludedMaxValue-1].
  ///
  /// Ie a value of uiExcludedMaxValue would be wrapped to 0, and (uiExcludedMaxValue+1) to 1, etc.
  /// Negative values are wrapped back around to a large value, ie -1 would be wrapped to (uiExcludedMaxValue-1).
  /// A value of 0 for uiExcludedMaxValue is invalid and results in a division by zero error.
  [[nodiscard]] constexpr xiiInt32 WrapInt(xiiInt32 iValue, xiiUInt32 uiExcludedMaxValue); // [tested]

  /// Wraps iValue around the minimum and maximum value, so that it stays within the range [iMinValue; iExcludedMaxValue-1].
  ///
  /// Ie a value of iExcludedMaxValue would be wrapped to iMinValue, and (iExcludedMaxValue+1) to (iMinValue+1), etc.
  /// Values below iMinValue are wrapped back around to a large value, ie (iMinValue-1) would be wrapped to (iExcludedMaxValue-1).
  ///
  /// Both iMinValue and iExcludedMaxValue can be negative, but iMinValue has to be strictly smaller than iExcludedMaxValue.
  [[nodiscard]] constexpr xiiInt32 WrapInt(xiiInt32 iValue, xiiInt32 iMinValue, xiiInt32 iExcludedMaxValue); // [tested]

  /// Wraps a float value around to stay within the [0; 1] range.
  ///
  /// Wrapping happens in both positive and negative direction. Ie -0.1f will be wrapped to 0.9f and 1.1f will be wrapped to 0.1f.
  /// Note that here the value 1.0f is included in the range. Only values larger than 1.0f get wrapped back to zero.
  /// Therefore it is different to what 'Fraction' would return.
  [[nodiscard]] float WrapFloat01(float fValue); // [tested]

  /// Wraps a float value around to stay within the [min; max] range.
  ///
  /// Both fMinValue and fMaxValue are inclusive.
  /// Both values may be negative, but fMinValue has to be strictly smaller than fMaxValue.
  [[nodiscard]] float WrapFloat(float fValue, float fMinValue, float fMaxValue); // [tested]

  /// Clamps "value" to the range [0; 1]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Saturate(T value); // [tested]

  /// Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  [[nodiscard]] float Floor(float f); // [tested]

  /// Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  ///
  /// This function is identical to 'Floor()' except that it already returns the result cast to an int.
  [[nodiscard]] xiiInt32 FloorToInt(float f); // [tested]

  /// Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  [[nodiscard]] double Floor(double f); // [tested]

  /// Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  [[nodiscard]] float Ceil(float f); // [tested]

  /// Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  ///
  /// This function is identical to 'Ceil()' except that it already returns the result cast to an int.
  [[nodiscard]] xiiInt32 CeilToInt(float f); // [tested]

  /// Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  [[nodiscard]] double Ceil(double f); // [tested]

  /// Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] float RoundDown(float f, float fMultiple); // [tested]

  /// Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] double RoundDown(double f, double fMultiple); // [tested]

  /// Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] float RoundUp(float f, float fMultiple); // [tested]

  /// Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] double RoundUp(double f, double fMultiple); // [tested]

  /// Returns the integer-part of f (removes the fraction).
  template <typename Type>
  [[nodiscard]] Type Trunc(Type f); // [tested]

  /// Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr xiiInt32 FloatToInt(float value);

  // There is a compiler bug in VS 2019 targeting 32-bit that causes an internal compiler error when casting double to long long.
  // FloatToInt(double) is not available on these version of the MSVC compiler.
#if XII_DISABLED(XII_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
  /// Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr xiiInt64 FloatToInt(double value);
#endif

  /// Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] float Round(float f); // [tested]

  /// Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  ///
  /// This function is identical to 'Round()' except that it already returns the result cast to an int.
  [[nodiscard]] xiiInt32 RoundToInt(float f); // [tested]

  /// Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] double Round(double f); // [tested]

  /// Rounds f to the closest value of multiple.
  [[nodiscard]] float RoundToMultiple(float f, float fMultiple);

  /// Rounds f to the closest value of multiple.
  [[nodiscard]] double RoundToMultiple(double f, double fMultiple);

  /// Returns the fraction-part of f.
  template <typename Type>
  [[nodiscard]] Type Fraction(Type f); // [tested]

  /// Returns "value mod div" for floats. This also works with negative numbers, both for value and for div.
  [[nodiscard]] float Mod(float value, float fDiv); // [tested]

  /// Returns "value mod div" for doubles. This also works with negative numbers, both for value and for div.
  [[nodiscard]] double Mod(double f, double fDiv); // [tested]

  /// Returns 1 / f
  template <typename Type>
  [[nodiscard]] constexpr Type Invert(Type f); // [tested]

  /// Returns a multiple of the given multiple that is larger than or equal to value.
  [[nodiscard]] constexpr xiiInt32 RoundUp(xiiInt32 value, xiiUInt16 uiMultiple); // [tested]

  /// Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr xiiInt32 RoundDown(xiiInt32 value, xiiUInt16 uiMultiple); // [tested]

  /// Returns a multiple of the given multiple that is greater than or equal to value.
  [[nodiscard]] constexpr xiiUInt32 RoundUp(xiiUInt32 value, xiiUInt16 uiMultiple); // [tested]

  /// Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr xiiUInt32 RoundDown(xiiUInt32 value, xiiUInt16 uiMultiple); // [tested]

  /// Returns true, if i is an odd number
  [[nodiscard]] constexpr bool IsOdd(xiiInt32 i); // [tested]

  /// Returns true, if i is an even number
  [[nodiscard]] constexpr bool IsEven(xiiInt32 i); // [tested]

  /// Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] xiiUInt32 FirstBitLow(xiiUInt32 uiBitmask); // [tested]

  /// Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] xiiUInt32 FirstBitLow(xiiUInt64 uiBitmask); // [tested]

  /// Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] xiiUInt32 FirstBitHigh(xiiUInt32 uiBitmask); // [tested]

  /// Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] xiiUInt32 FirstBitHigh(xiiUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the end (least significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 0
  /// 0b0110 -> 1
  /// 0b0100 -> 2
  /// Returns 32 when the input is 0
  [[nodiscard]] xiiUInt32 CountTrailingZeros(xiiUInt32 uiBitmask); // [tested]

  /// 64 bit overload for CountTrailingZeros()
  [[nodiscard]] xiiUInt32 CountTrailingZeros(xiiUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the start (most significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 29
  /// 0b0011 -> 30
  /// 0b0001 -> 31
  /// 0b0000 -> 32
  /// Returns 32 when the input is 0
  [[nodiscard]] xiiUInt32 CountLeadingZeros(xiiUInt32 uiBitmask); // [tested]

  /// Returns the number of bits set
  [[nodiscard]] xiiUInt32 CountBits(xiiUInt32 value);

  /// Returns the number of bits set
  [[nodiscard]] xiiUInt32 CountBits(xiiUInt64 value);

  /// Creates a bitmask in which the low N bits are set. For example for N=5, this would be '0000 ... 0001 1111'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] constexpr Type Bitmask_LowN(xiiUInt32 uiNumBitsToSet);

  /// Creates a bitmask in which the high N bits are set. For example for N=5, this would be '1111 1000 ... 0000'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] constexpr Type Bitmask_HighN(xiiUInt32 uiNumBitsToSet);

  /// Swaps the values in the two variables f1 and f2
  template <typename T>
  void Swap(T& ref_f1, T& ref_f2); // [tested]

  /// Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, float fFactor); // [tested]

  /// Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, double fFactor); // [tested]

  /// Returns the interpolation factor such that Lerp(fMin, fMax, factor) == fValue.
  template <typename T>
  [[nodiscard]] constexpr float Unlerp(T fMin, T fMax, T fValue); // [tested]

  /// Returns the interpolation factor such that Lerp(fMin, fMax, factor) == fValue.
  template <typename T>
  [[nodiscard]] constexpr double UnlerpDouble(T fMin, T fMax, T fValue); // [tested]

  /// Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  [[nodiscard]] constexpr T Step(T value, T edge); // [tested]

  /// Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between.
  template <typename Type>
  [[nodiscard]] Type SmoothStep(Type value, Type edge1, Type edge2); // [tested]

  /// Returns 0, if value is <= edge1, 1 if value >= edge2 and the second order hermite interpolation in between.
  template <typename Type>
  [[nodiscard]] Type SmootherStep(Type value, Type edge1, Type edge2); // [tested]

  /// Returns true, if there exists some x with base^x == value.
  [[nodiscard]] XII_FOUNDATION_DLL bool IsPowerOf(xiiInt32 value, xiiInt32 iBase); // [tested]

  /// Returns true, if there exists some x with 2^x == value.
  [[nodiscard]] constexpr bool IsPowerOf2(xiiInt32 value); // [tested]

  /// Returns true, if there exists some x with 2^x == value.
  [[nodiscard]] constexpr bool IsPowerOf2(xiiUInt32 value); // [tested]

  /// Returns true, if there exists some x with 2^x == value.
  [[nodiscard]] constexpr bool IsPowerOf2(xiiInt64 value); // [tested]

  /// Returns true, if there exists some x with 2^x == value.
  [[nodiscard]] constexpr bool IsPowerOf2(xiiUInt64 value); // [tested]

  /// Returns the next power-of-two that is <= value.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt32 PowerOfTwo_Floor(xiiUInt32 value); // [tested]

  /// Returns the next power-of-two that is <= value.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt64 PowerOfTwo_Floor(xiiUInt64 value); // [tested]

  /// Returns the next power-of-two that is >= value.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt32 PowerOfTwo_Ceil(xiiUInt32 value); // [tested]

  /// Returns the next power-of-two that is >= value.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt64 PowerOfTwo_Ceil(xiiUInt64 value); // [tested]

  /// Returns the greatest common divisor.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt32 GreatestCommonDivisor(xiiUInt32 a, xiiUInt32 b); // [tested]

  /// Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  template <typename Type>
  [[nodiscard]] constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon);

  /// Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  [[nodiscard]] constexpr bool IsInRange(T value, T minVal, T maxVal); // [tested]

  /// Checks whether the given number is close to zero.
  template <typename Type>
  [[nodiscard]] bool IsZero(Type f, Type fEpsilon); // [tested]

  /// Converts a color value from float [0;1] range to unsigned int with the given number of bits, with proper rounding.
  template <xiiUInt32 BitCount>
  [[nodiscard]] xiiUInt32 ColorFloatToUnsignedInt(float value);

  /// Converts a color value from float [0;1] range to unsigned byte [0;255] range, with proper rounding.
  [[nodiscard]] xiiUInt8 ColorFloatToByte(float value); // [tested]

  /// Converts a color value from float [0;1] range to unsigned short [0;65535] range, with proper rounding.
  [[nodiscard]] xiiUInt16 ColorFloatToShort(float value); // [tested]

  /// Converts a color value from float [-1;1] range to signed byte [-127;127] range, with proper rounding.
  [[nodiscard]] xiiInt8 ColorFloatToSignedByte(float value); // [tested]

  /// Converts a color value from float [-1;1] range to signed short [-32767;32767] range, with proper rounding.
  [[nodiscard]] xiiInt16 ColorFloatToSignedShort(float value); // [tested]

  /// Converts a color value from unsigned byte [0;255] range to float [0;1] range, with proper rounding.
  [[nodiscard]] constexpr float ColorByteToFloat(xiiUInt8 value); // [tested]

  /// Converts a color value from unsigned short [0;65535] range to float [0;1] range, with proper rounding.
  [[nodiscard]] constexpr float ColorShortToFloat(xiiUInt16 value); // [tested]

  /// Converts a color value from signed byte [-128;127] range to float [-1;1] range, with proper rounding.
  [[nodiscard]] constexpr float ColorSignedByteToFloat(xiiInt8 value); // [tested]

  /// Converts a color value from signed short [-32768;32767] range to float [0;1] range, with proper rounding.
  [[nodiscard]] constexpr float ColorSignedShortToFloat(xiiInt16 value); // [tested]

  /// Evaluates the cubic spline defined by four control points at time \a t and returns the interpolated result.
  /// Can be used with T as float, vec2, vec3 or vec4
  template <typename T, typename T2>
  [[nodiscard]] T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint);

  /// out_Result = \a a * \a b. If an overflow happens, XII_FAILURE is returned.
  XII_FOUNDATION_DLL xiiResult TryMultiply32(xiiUInt32& out_uiResult, xiiUInt32 a, xiiUInt32 b, xiiUInt32 c = 1, xiiUInt32 d = 1); // [tested]

  /// returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt32 SafeMultiply32(xiiUInt32 a, xiiUInt32 b, xiiUInt32 c = 1, xiiUInt32 d = 1);

  /// out_Result = \a a * \a b. If an overflow happens, XII_FAILURE is returned.
  XII_FOUNDATION_DLL xiiResult TryMultiply64(xiiUInt64& out_uiResult, xiiUInt64 a, xiiUInt64 b, xiiUInt64 c = 1, xiiUInt64 d = 1); // [tested]

  /// returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] XII_FOUNDATION_DLL xiiUInt64 SafeMultiply64(xiiUInt64 a, xiiUInt64 b, xiiUInt64 c = 1, xiiUInt64 d = 1);

  /// Checks whether the given 64bit value actually fits into size_t, If it doesn't XII_FAILURE is returned.
  xiiResult TryConvertToSizeT(size_t& out_uiResult, xiiUInt64 uiValue); // [tested]

  /// Checks whether the given 64bit value actually fits into size_t, If it doesn't the program is terminated.
  [[nodiscard]] XII_FOUNDATION_DLL size_t SafeConvertToSizeT(xiiUInt64 uiValue);

  /// If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] XII_FOUNDATION_DLL float ReplaceNaN(float fValue, float fFallback); // [tested]

  /// If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] XII_FOUNDATION_DLL double ReplaceNaN(double fValue, double fFallback); // [tested]

  /// Returns the hypotenuse of a given x and y term.
  [[nodiscard]] float Hypot(float x, float y);

  /// Returns the hypotenuse of a given x and y term.
  [[nodiscard]] double Hypot(double x, double y);

  /// Calculates a value between 0 and 1, given the precondition that the value is between the min and the max. 0 means value = min and 1 means value = max.
  [[nodiscard]] float NormalizeToRange(float value, float fMin, float fMax);

  /// Calculates a value between 0 and 1, given the precondition that the value is between the min and the max. 0 means value = min and 1 means value = max.
  [[nodiscard]] double NormalizeToRange(double value, double fMin, double fMax);

} // namespace xiiMath

#include <Foundation/Math/Implementation/MathDouble_inl.h>
#include <Foundation/Math/Implementation/MathFixedPoint_inl.h>
#include <Foundation/Math/Implementation/MathFloat_inl.h>
#include <Foundation/Math/Implementation/MathInt32_inl.h>
#include <Foundation/Math/Implementation/Math_inl.h>
