#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>

namespace xiiMath
{
  /// \brief Available Procedural Curve Generators.
  ///
  /// Supported easing function types adapted from https://easings.net. To view some of these in action, please
  /// visit the above mentioned source link.
  ///
  /// Types:
  /// - EaseIn: Indicates a transition from the zero strength to full strength.
  /// - EaseOut: Indicates a transition from full strength to zero strength.
  /// - EaseInOut: Indicates a transition from zero strength to full strength halfway, then a transition back to zero strength.
  /// - EaseOutIn: Indicates a transition from full strength to zero strength halfway, then a transition back to full strength.
  enum xiiEasingFunctions
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

    InBack,
    OutBack,
    InOutBack,

    InElastic,
    OutElastic,
    InOutElastic,

    InBounce,
    OutBounce,
    InOutBounce,

    Conical,
    FadeInHoldFadeOut,
    FadeInFadeOut,
    Bell,

    ENUM_COUNT, // All easing function types must be stated before this.
  };

  /// \brief Helper function that returns the easing value from an easing function.
  template <typename Type>
  Type GetEasingValue(xiiEasingFunctions easingFunction, Type input);

  /// \brief Helper function that returns the function value at the given x coordinate.
  ///
  /// If \a inverse is true, the value (1-Y) is returned.
  template <typename Type>
  Type GetEasingValue(xiiEasingFunctions easingFunction, Type input, bool inverse);

  template <typename Type>
  Type EaseConstantZero(Type t);

  template <typename Type>
  Type EaseConstantQuarter(Type t);

  template <typename Type>
  Type EaseConstantHalf(Type t);

  template <typename Type>
  Type EaseConstantThreeFourths(Type t);

  template <typename Type>
  Type EaseConstantOne(Type t);

  template <typename Type>
  Type EaseInLinear(Type t);

  template <typename Type>
  Type EaseOutLinear(Type t);

  template <typename Type>
  Type EaseInOutLinear(Type t);

  template <typename Type>
  Type EaseInSine(Type t);

  template <typename Type>
  Type EaseOutSine(Type t);

  template <typename Type>
  Type EaseInOutSine(Type t);

  template <typename Type>
  Type EaseInQuad(Type t);

  template <typename Type>
  Type EaseOutQuad(Type t);

  template <typename Type>
  Type EaseInOutQuad(Type t);

  template <typename Type>
  Type EaseInCubic(Type t);

  template <typename Type>
  Type EaseOutCubic(Type t);

  template <typename Type>
  Type EaseInOutCubic(Type t);

  template <typename Type>
  Type EaseInQuartic(Type t);

  template <typename Type>
  Type EaseOutQuartic(Type t);

  template <typename Type>
  Type EaseInOutQuartic(Type t);

  template <typename Type>
  Type EaseInQuintic(Type t);

  template <typename Type>
  Type EaseOutQuintic(Type t);

  template <typename Type>
  Type EaseInOutQuintic(Type t);

  template <typename Type>
  Type EaseInExpo(Type t);

  template <typename Type>
  Type EaseOutExpo(Type t);

  template <typename Type>
  Type EaseInOutExpo(Type t);

  template <typename Type>
  Type EaseInCirc(Type t);

  template <typename Type>
  Type EaseOutCirc(Type t);

  template <typename Type>
  Type EaseInOutCirc(Type t);

  template <typename Type>
  Type EaseInBack(Type t);

  template <typename Type>
  Type EaseOutBack(Type t);

  template <typename Type>
  Type EaseInOutBack(Type t);

  template <typename Type>
  Type EaseInElastic(Type t);

  template <typename Type>
  Type EaseOutElastic(Type t);

  template <typename Type>
  Type EaseInOutElastic(Type t);

  template <typename Type>
  Type EaseInBounce(Type t);

  template <typename Type>
  Type EaseOutBounce(Type t);

  template <typename Type>
  Type EaseInOutBounce(Type t);

  template <typename Type>
  Type EaseConical(Type t);

  template <typename Type>
  Type EaseFadeInHoldFadeOut(Type t);

  template <typename Type>
  Type EaseFadeInFadeOut(Type t);

  template <typename Type>
  Type EaseBell(Type t);

} // namespace xiiMath

#include <Foundation/Math/Implementation/Easing_inl.h>
