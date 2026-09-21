/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

/// Available Procedural Curve Generators.
///
/// Supported easing function types adapted from https://easings.net. To view some of these in action, please
/// visit the above mentioned source link.
///
/// Types:
/// - EaseIn: Indicates a transition from the zero strength to full strength.
/// - EaseOut: Indicates a transition from full strength to zero strength.
/// - EaseInOut: Indicates a transition from zero strength to full strength halfway, then a transition back to zero strength.
/// - EaseOutIn: Indicates a transition from full strength to zero strength halfway, then a transition back to full strength.
struct XII_FOUNDATION_DLL xiiEasingFunction
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    ConstantZero,
    ConstantQuarter,
    ConstantHalf,
    ConstantThreeFourths,
    ConstantOne,

    InLinear,
    OutLinear,
    InOutLinear,

    InSine,
    OutSine,
    InOutSine,

    InQuad,
    OutQuad,
    InOutQuad,

    InCubic,
    OutCubic,
    InOutCubic,

    InQuartic,
    OutQuartic,
    InOutQuartic,

    InQuintic,
    OutQuintic,
    InOutQuintic,

    InExpo,
    OutExpo,
    InOutExpo,

    InCirc,
    OutCirc,
    InOutCirc,

    InBack,    ///< Values exceed the 0-1 range briefly.
    OutBack,   ///< Values exceed the 0-1 range briefly.
    InOutBack, ///< Values exceed the 0-1 range briefly.

    InElastic,    ///< Values exceed the 0-1 range briefly.
    OutElastic,   ///< Values exceed the 0-1 range briefly.
    InOutElastic, ///< Values exceed the 0-1 range briefly.

    InBounce,
    OutBounce,
    InOutBounce,

    Conical,
    FadeInHoldFadeOut,
    FadeInFadeOut,
    Bell,

    ENUM_COUNT, // All easing function types must be stated before this.

    Default = InLinear
  };

  /// Helper function that returns the function value at the given input.
  static double GetValue(xiiEasingFunction::Enum function, double fInput);

  /// Helper function that returns the function value at the given input.
  ///
  /// if \a inverse is true, the value (1 - result) is returned.
  static double GetValue(xiiEasingFunction::Enum function, double fInput, bool bInverse);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiEasingFunction);

namespace xiiMath
{
  double EaseConstantZero(double t);
  double EaseConstantQuarter(double t);
  double EaseConstantHalf(double t);
  double EaseConstantThreeFourths(double t);
  double EaseConstantOne(double t);
  double EaseInLinear(double t);
  double EaseOutLinear(double t);
  double EaseInOutLinear(double t);
  double EaseInSine(double t);
  double EaseOutSine(double t);
  double EaseInOutSine(double t);
  double EaseInQuad(double t);
  double EaseOutQuad(double t);
  double EaseInOutQuad(double t);
  double EaseInCubic(double t);
  double EaseOutCubic(double t);
  double EaseInOutCubic(double t);
  double EaseInQuartic(double t);
  double EaseOutQuartic(double t);
  double EaseInOutQuartic(double t);
  double EaseInQuintic(double t);
  double EaseOutQuintic(double t);
  double EaseInOutQuintic(double t);
  double EaseInExpo(double t);
  double EaseOutExpo(double t);
  double EaseInOutExpo(double t);
  double EaseInCirc(double t);
  double EaseOutCirc(double t);
  double EaseInOutCirc(double t);
  double EaseInBack(double t);
  double EaseOutBack(double t);
  double EaseInOutBack(double t);
  double EaseInElastic(double t);
  double EaseOutElastic(double t);
  double EaseInOutElastic(double t);
  double EaseInBounce(double t);
  double EaseOutBounce(double t);
  double EaseInOutBounce(double t);
  double EaseConical(double t);
  double EaseFadeInHoldFadeOut(double t);
  double EaseFadeInFadeOut(double t);
  double EaseBell(double t);
} // namespace xiiMath

#include <Foundation/Math/Implementation/Easing_inl.h>
