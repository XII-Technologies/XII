/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Math.h>

namespace xiiMath
{
  XII_ALWAYS_INLINE double EaseConstantZero(double t)
  {
    XII_IGNORE_UNUSED(t);

    return 0.0;
  }

  XII_ALWAYS_INLINE double EaseConstantQuarter(double t)
  {
    XII_IGNORE_UNUSED(t);

    return 0.25;
  }

  XII_ALWAYS_INLINE double EaseConstantHalf(double t)
  {
    XII_IGNORE_UNUSED(t);

    return 0.5;
  }

  XII_ALWAYS_INLINE double EaseConstantThreeFourths(double t)
  {
    XII_IGNORE_UNUSED(t);

    return 0.75;
  }

  XII_ALWAYS_INLINE double EaseConstantOne(double t)
  {
    XII_IGNORE_UNUSED(t);

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
    return xiiMath::Abs(0.5 - t) * 2.0;
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
    return t < 0.5 ? 2.0 * t * t : 1.0 - xiiMath::Pow(-2.0 * t + 2.0, 2.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInCubic(double t)
  {
    return t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutCubic(double t)
  {
    return 1.0 - xiiMath::Pow(1 - t, 3.0);
  }

  XII_ALWAYS_INLINE double EaseInOutCubic(double t)
  {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - xiiMath::Pow(-2.0 * t + 2.0, 3.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInQuartic(double t)
  {
    return t * t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutQuartic(double t)
  {
    return 1.0 - xiiMath::Pow(1.0 - t, 4.0);
  }

  XII_ALWAYS_INLINE double EaseInOutQuartic(double t)
  {
    return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - xiiMath::Pow(-2.0 * t + 2.0, 4.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInQuintic(double t)
  {
    return t * t * t * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutQuintic(double t)
  {
    return 1.0 - xiiMath::Pow(1.0 - t, 5.0);
  }

  XII_ALWAYS_INLINE double EaseInOutQuintic(double t)
  {
    return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - xiiMath::Pow(-2.0 * t + 2.0, 5.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInExpo(double t)
  {
    return t == 0 ? 0 : xiiMath::Pow(2.0, 10.0 * t - 10.0);
  }

  XII_ALWAYS_INLINE double EaseOutExpo(double t)
  {
    return t == 1.0 ? 1.0 : 1.0 - xiiMath::Pow(2.0, -10.0 * t);
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
      return t < 0.5 ? xiiMath::Pow(2.0, 20.0 * t - 10.0) / 2.0 : (2.0 - xiiMath::Pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }
  }

  XII_ALWAYS_INLINE double EaseInCirc(double t)
  {
    return 1.0 - sqrt(1.0 - xiiMath::Pow(t, 2.0));
  }

  XII_ALWAYS_INLINE double EaseOutCirc(double t)
  {
    return sqrt(1.0 - xiiMath::Pow(t - 1.0, 2.0));
  }

  XII_ALWAYS_INLINE double EaseInOutCirc(double t)
  {
    return t < 0.5 ? (1.0 - sqrt(1.0 - xiiMath::Pow(2.0 * t, 2.0))) / 2.0 : (sqrt(1.0 - xiiMath::Pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
  }

  XII_ALWAYS_INLINE double EaseInBack(double t)
  {
    return 2.70158 * t * t * t - 1.70158 * t * t;
  }

  XII_ALWAYS_INLINE double EaseOutBack(double t)
  {
    return 10 + 2.70158 * xiiMath::Pow(t - 1.0, 3.0) + 1.70158 * xiiMath::Pow(t - 1.0, 2.0);
  }

  XII_ALWAYS_INLINE double EaseInOutBack(double t)
  {
    return t < 0.5 ? (xiiMath::Pow(2.0 * t, 2.0) * (((1.70158 * 1.525) + 1.0) * 2 * t - (1.70158 * 1.525))) / 2.0 : (xiiMath::Pow(2.0 * t - 2.0, 2.0) * (((1.70158 * 1.525) + 1.0) * (t * 2.0 - 2.0) + (1.70158 * 1.525)) + 2.0) / 2.0;
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
      return -xiiMath::Pow(2.0, 10.0 * t - 10.0) * sin((t * 10.0 - 10.75) * ((2.0 * xiiMath::Pi<double>()) / 3.0));
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
      return xiiMath::Pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * ((2.0 * xiiMath::Pi<double>()) / 3.0)) + 1.0;
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
      return t < 0.5 ? -(xiiMath::Pow(2.0, 20.0 * t - 10.0) * sin((20.0 * t - 11.125) * ((2 * xiiMath::Pi<double>()) / 4.5))) / 2.0 : (xiiMath::Pow(2.0, -20.0 * t + 10.0) * sin((20.0 * t - 11.125) * ((2 * xiiMath::Pi<double>()) / 4.5))) / 2.0 + 1.0;
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
inline double xiiEasingFunction::GetValue(xiiEasingFunction::Enum function, double fInput)
{
  switch (function)
  {
    case ConstantZero:
      return xiiMath::EaseConstantZero(fInput);
    case ConstantQuarter:
      return xiiMath::EaseConstantQuarter(fInput);
    case ConstantHalf:
      return xiiMath::EaseConstantHalf(fInput);
    case ConstantThreeFourths:
      return xiiMath::EaseConstantThreeFourths(fInput);
    case ConstantOne:
      return xiiMath::EaseConstantOne(fInput);

    case InLinear:
      return xiiMath::EaseInLinear(fInput);
    case OutLinear:
      return xiiMath::EaseOutLinear(fInput);
    case InOutLinear:
      return xiiMath::EaseInOutLinear(fInput);

    case InSine:
      return xiiMath::EaseInSine(fInput);
    case OutSine:
      return xiiMath::EaseOutSine(fInput);
    case InOutSine:
      return xiiMath::EaseInOutSine(fInput);

    case InQuad:
      return xiiMath::EaseInQuad(fInput);
    case OutQuad:
      return xiiMath::EaseOutQuad(fInput);
    case InOutQuad:
      return xiiMath::EaseInOutQuad(fInput);

    case InCubic:
      return xiiMath::EaseInCubic(fInput);
    case OutCubic:
      return xiiMath::EaseOutCubic(fInput);
    case InOutCubic:
      return xiiMath::EaseInOutCubic(fInput);

    case InQuartic:
      return xiiMath::EaseInQuartic(fInput);
    case OutQuartic:
      return xiiMath::EaseOutQuartic(fInput);
    case InOutQuartic:
      return xiiMath::EaseInOutQuartic(fInput);

    case InQuintic:
      return xiiMath::EaseInQuintic(fInput);
    case OutQuintic:
      return xiiMath::EaseOutQuintic(fInput);
    case InOutQuintic:
      return xiiMath::EaseInOutQuintic(fInput);

    case InExpo:
      return xiiMath::EaseInExpo(fInput);
    case OutExpo:
      return xiiMath::EaseOutExpo(fInput);
    case InOutExpo:
      return xiiMath::EaseInOutExpo(fInput);

    case InCirc:
      return xiiMath::EaseInCirc(fInput);
    case OutCirc:
      return xiiMath::EaseOutCirc(fInput);
    case InOutCirc:
      return xiiMath::EaseInOutCirc(fInput);

    case InBack:
      return xiiMath::EaseInBack(fInput);
    case OutBack:
      return xiiMath::EaseOutBack(fInput);
    case InOutBack:
      return xiiMath::EaseInOutBack(fInput);

    case InElastic:
      return xiiMath::EaseInElastic(fInput);
    case OutElastic:
      return xiiMath::EaseOutElastic(fInput);
    case InOutElastic:
      return xiiMath::EaseInOutElastic(fInput);

    case InBounce:
      return xiiMath::EaseInBounce(fInput);
    case OutBounce:
      return xiiMath::EaseOutBounce(fInput);
    case InOutBounce:
      return xiiMath::EaseInOutBounce(fInput);

    case Conical:
      return xiiMath::EaseConical(fInput);
    case FadeInHoldFadeOut:
      return xiiMath::EaseFadeInHoldFadeOut(fInput);
    case FadeInFadeOut:
      return xiiMath::EaseFadeInFadeOut(fInput);
    case Bell:
      return xiiMath::EaseBell(fInput);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return 0.0;
}

inline double xiiEasingFunction::GetValue(xiiEasingFunction::Enum function, double fInput, bool bInverse)
{
  double value = GetValue(function, fInput);

  return bInverse ? (1.0 - value) : value;
}
