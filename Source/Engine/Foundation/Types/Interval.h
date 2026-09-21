/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Math.h>

/// Represents an interval with a start and an end value.
template <class Type>
class xiiInterval
{
public:
  /// The default constructor initializes the two values to zero.
  constexpr xiiInterval() = default;

  /// Initializes both start and end to the same value.
  constexpr xiiInterval(Type startAndEndValue);

  /// Initializes start and end to the given values.
  ///
  /// Clamps the end value to not be lower than the start value.
  constexpr xiiInterval(Type start, Type end);

  /// Sets the start value. If necessary, the end value will adjusted to not be lower than the start value.
  void SetStartAdjustEnd(Type value);

  /// Sets the end value. If necessary, the start value will adjusted to not be higher than the end value.
  void SetEndAdjustStart(Type value);

  /// Adjusts the start and end value to be within the given range. Prefers to adjust the end value, over the start value.
  ///
  /// Enforces that the start and end value are not closer than the given minimum separation.
  void ClampToIntervalAdjustEnd(Type minValue, Type maxValue, Type minimumSeparation = Type());

  /// Adjusts the start and end value to be within the given range. Prefers to adjust the start value, over the end value.
  ///
  /// Enforces that the start and end value are not closer than the given minimum separation.
  void ClampToIntervalAdjustStart(Type minValue, Type maxValue, Type minimumSeparation = Type());

  /// Returns how much the start and and value are separated from each other.
  Type GetSeparation() const;

  bool operator==(const xiiInterval<Type>& rhs) const;

  Type m_StartValue = Type();
  Type m_EndValue   = Type();
};

using xiiDoubleInterval = xiiInterval<double>;
using xiiFloatInterval  = xiiInterval<float>;
using xiiIntInterval    = xiiInterval<xiiInt32>;

#include <Foundation/Types/Implementation/Interval_inl.h>
