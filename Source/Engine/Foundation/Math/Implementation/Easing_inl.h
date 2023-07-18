#pragma once

#include <Foundation/Math/Math.h>

namespace xiiMath
{
  XII_ALWAYS_INLINE double EaseConstantZero(double t)
  {
    return 0.0;
  }

  XII_ALWAYS_INLINE double EaseConstantQuarter(double t)
  {
    return 0.25;
  }

  XII_ALWAYS_INLINE double EaseConstantHalf(double t)
  {
    return 0.5;
  }

  XII_ALWAYS_INLINE double EaseConstantThreeFourths(double t)
  {
    return 0.75;
  }

  XII_ALWAYS_INLINE double EaseConstantOne(double t)
  {
    return 1.0;
  }

  XII_ALWAYS_INLINE double EaseInLinear(double t)
  {
    return t;
  }

  XII_ALWAYS_INLINE double EaseOutLinear(double t)
  {
    return 1.0 - t;
  }

  XII_ALWAYS_INLINE double EaseInOutLinear(double t)
  {
    return abs(0.5 - t) * 2.0;
  }

  XII_ALWAYS_INLINE double EaseInSine(double t)
  {
    return 1.0 - cos((t * xiiMath::Pi<double>()) / 2.0);
  }

  XII_ALWAYS_INLINE double EaseOutSine(double t)
  {
    return sin((t * xiiMath::Pi<double>()) / 2.0);
  }

  XII_ALWAYS_INLINE double EaseInOutSine(double t)
  {
    return -(cos(xiiMath::Pi<double>() * t) - 1.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInQuad(double t)
  {
    return t * t;
  }

  XII_ALWAYS_INLINE double EaseOutQuad(double t)
  {
    return 1.0 - (1.0 - t) * (1.0 - t);
  }

  XII_ALWAYS_INLINE double EaseInOutQuad(double t)
  {
    return t < 0.5 ? 2.0 * t * t : 1.0 - pow(-2.0 * t + 2, 2) / 2;
  }

  XII_ALWAYS_INLINE double EaseInCubic(double t)
  {
    return t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutCubic(double t)
  {
    return 1.0 - pow(1 - t, 3.0);
  }

  XII_ALWAYS_INLINE double EaseInOutCubic(double t)
  {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInQuartic(double t)
  {
    return t * t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutQuartic(double t)
  {
    return 1.0 - pow(1.0 - t, 4.0);
  }

  XII_ALWAYS_INLINE double EaseInOutQuartic(double t)
  {
    return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 4.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInQuintic(double t)
  {
    return t * t * t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutQuintic(double t)
  {
    return 1.0 - pow(1.0 - t, 5.0);
  }

  XII_ALWAYS_INLINE double EaseInOutQuintic(double t)
  {
    return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 5.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInExpo(double t)
  {
    return t == 0 ? 0 : pow(2.0, 10.0 * t - 10.0);
  }

  XII_ALWAYS_INLINE double EaseOutExpo(double t)
  {
    return t == 1.0 ? 1.0 : 1.0 - pow(2.0, -10.0 * t);
  }

  XII_ALWAYS_INLINE double EaseInOutExpo(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5 ? pow(2.0, 20.0 * t - 10.0) / 2.0 : (2.0 - pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }
  }

  XII_ALWAYS_INLINE double EaseInCirc(double t)
  {
    return 1.0 - sqrt(1.0 - pow(t, 2));
  }

  XII_ALWAYS_INLINE double EaseOutCirc(double t)
  {
    return sqrt(1.0 - pow(t - 1.0, 2.0));
  }

  XII_ALWAYS_INLINE double EaseInOutCirc(double t)
  {
    return t < 0.5 ? (1.0 - sqrt(1.0 - pow(2.0 * t, 2.0))) / 2.0 : (sqrt(1.0 - pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInBack(double t)
  {
    return 2.70158 * t * t * t - 1.70158 * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutBack(double t)
  {
    return 10 + 2.70158 * pow(t - 1.0, 3.0) + 1.70158 * pow(t - 1.0, 2.0);
  }

  XII_ALWAYS_INLINE double EaseInOutBack(double t)
  {
    return t < 0.5 ? (pow(2.0 * t, 2.0) * (((1.70158 * 1.525) + 1.0) * 2 * t - (1.70158 * 1.525))) / 2.0 : (pow(2.0 * t - 2.0, 2.0) * (((1.70158 * 1.525) + 1.0) * (t * 2.0 - 2.0) + (1.70158 * 1.525)) + 2.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return -pow(2.0, 10.0 * t - 10.0) * sin((t * 10.0 - 10.75) * ((2.0 * xiiMath::Pi<double>()) / 3.0));
    }
  }

  XII_ALWAYS_INLINE double EaseOutElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * ((2.0 * xiiMath::Pi<double>()) / 3.0)) + 1.0;
    }
  }

  XII_ALWAYS_INLINE double EaseInOutElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5 ? -(pow(2.0, 20.0 * t - 10.0) * sin((20.0 * t - 11.125) * ((2 * xiiMath::Pi<double>()) / 4.5))) / 2.0 : (pow(2.0, -20.0 * t + 10.0) * sin((20.0 * t - 11.125) * ((2 * xiiMath::Pi<double>()) / 4.5))) / 2.0 + 1.0;
    }
  }

  XII_ALWAYS_INLINE double EaseInBounce(double t)
  {
    return 1.0 - EaseOutBounce(1.0 - t);
  }

  XII_ALWAYS_INLINE double EaseOutBounce(double t)
  {
    if (t < 1.0 / 2.75)
    {
      return 7.5625 * t * t;
    }
    else if (t < 2.0 / 2.75)
    {
      t -= 1.5 / 2.75;
      return 7.5625 * t * t + 0.75;
    }
    else if (t < 2.5 / 2.75)
    {
      t -= 2.25 / 2.75;
      return 7.5625 * t * t + 0.9375;
    }
    else
    {
      t -= 2.625 / 2.75;
      return 7.5625 * t * t + 0.984375;
    }
  }

  XII_ALWAYS_INLINE double EaseInOutBounce(double t)
  {
    return t < 0.5 ? (1.0 - EaseOutBounce(1.0 - 2.0 * t)) / 2.0 : (1.0 + EaseOutBounce(2.0 * t - 1.0)) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseConical(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - xiiMath::Pow(1.0 - (t * 5.0), 4.0);
    }
    else
    {
      t = (t - 0.2) / 0.8; // Normalize to 0-1 range.

      return 1.0 - xiiMath::Pow(t, 2.0);
    }
  }

  XII_ALWAYS_INLINE double EaseFadeInHoldFadeOut(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - xiiMath::Pow(1.0 - (t * 5.0), 3.0);
    }
    else if (t > 0.8)
    {
      return 1.0 - xiiMath::Pow((t - 0.8) * 5.0, 3.0);
    }
    else
    {
      return 1.0;
    }
  }
  XII_ALWAYS_INLINE double EaseFadeInFadeOut(double t)
  {
    if (t < 0.5)
    {
      return 1.0f - xiiMath::Pow(1.0 - (t * 2.0), 3.0);
    }
    else
    {
      return 1.0 - xiiMath::Pow((t - 0.5) * 2.0, 3.0);
    }
  }

  XII_ALWAYS_INLINE double EaseBell(double t)
  {
    if (t < 0.25)
    {
      return (xiiMath::Pow((t * 4.0), 3.0)) * 0.5;
    }
    else if (t < 0.5)
    {
      return (1.0f - xiiMath::Pow(1.0 - ((t - 0.25) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else if (t < 0.75)
    {
      return (1.0f - xiiMath::Pow(((t - 0.5) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else
    {
      return (xiiMath::Pow(1.0 - ((t - 0.75) * 4.0), 3.0)) * 0.5;
    }
  }
} // namespace xiiMath

  // static
  inline double xiiEasingFunction::GetValue(Enum function, double input)
  {
    switch (function)
    {
      case ConstantZero:
        return xiiMath::EaseConstantZero(input);
      case ConstantQuarter:
        return xiiMath::EaseConstantQuarter(input);
      case ConstantHalf:
        return xiiMath::EaseConstantHalf(input);
      case ConstantThreeFourths:
        return xiiMath::EaseConstantThreeFourths(input);
      case ConstantOne:
        return xiiMath::EaseConstantOne(input);

      case InLinear:
        return xiiMath::EaseInLinear(input);
      case OutLinear:
        return xiiMath::EaseOutLinear(input);
      case InOutLinear:
        return xiiMath::EaseInOutLinear(input);

      case InSine:
        return xiiMath::EaseInSine(input);
      case OutSine:
        return xiiMath::EaseOutSine(input);
      case InOutSine:
        return xiiMath::EaseInOutSine(input);

      case InQuad:
        return xiiMath::EaseInQuad(input);
      case OutQuad:
        return xiiMath::EaseOutQuad(input);
      case InOutQuad:
        return xiiMath::EaseInOutQuad(input);

      case InCubic:
        return xiiMath::EaseInCubic(input);
      case OutCubic:
        return xiiMath::EaseOutCubic(input);
      case InOutCubic:
        return xiiMath::EaseInOutCubic(input);

      case InQuartic:
        return xiiMath::EaseInQuartic(input);
      case OutQuartic:
        return xiiMath::EaseOutQuartic(input);
      case InOutQuartic:
        return xiiMath::EaseInOutQuartic(input);

      case InQuintic:
        return xiiMath::EaseInQuintic(input);
      case OutQuintic:
        return xiiMath::EaseOutQuintic(input);
      case InOutQuintic:
        return xiiMath::EaseInOutQuintic(input);

      case InExpo:
        return xiiMath::EaseInExpo(input);
      case OutExpo:
        return xiiMath::EaseOutExpo(input);
      case InOutExpo:
        return xiiMath::EaseInOutExpo(input);

      case InCirc:
        return xiiMath::EaseInCirc(input);
      case OutCirc:
        return xiiMath::EaseOutCirc(input);
      case InOutCirc:
        return xiiMath::EaseInOutCirc(input);

      case InBack:
        return xiiMath::EaseInBack(input);
      case OutBack:
        return xiiMath::EaseOutBack(input);
      case InOutBack:
        return xiiMath::EaseInOutBack(input);

      case InElastic:
        return xiiMath::EaseInElastic(input);
      case OutElastic:
        return xiiMath::EaseOutElastic(input);
      case InOutElastic:
        return xiiMath::EaseInOutElastic(input);

      case InBounce:
        return xiiMath::EaseInBounce(input);
      case OutBounce:
        return xiiMath::EaseOutBounce(input);
      case InOutBounce:
        return xiiMath::EaseInOutBounce(input);

      case Conical:
        return xiiMath::EaseConical(input);
      case FadeInHoldFadeOut:
        return xiiMath::EaseFadeInHoldFadeOut(input);
      case FadeInFadeOut:
        return xiiMath::EaseFadeInFadeOut(input);
      case Bell:
        return xiiMath::EaseBell(input);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
    return 0.0;
  }

  inline double xiiEasingFunction::GetValue(Enum function, double input, bool bInverse)
  {
    double value = GetValue(function, input);

    return bInverse ? (1.0 - value) : value;
  }
