/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// A class which can be used to represent rational numbers by stating their numerator and denominator.
///
/// xiiRational uses the following rules
///   0/0 is legal and will be interpreted as 0/1
///   If you are representing a whole number, the denominator should be 1
///
class xiiRational
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor, initializes to 0/1.
  xiiRational();

  /// Constructor to initialize a rational
  xiiRational(xiiUInt32 uiNumerator, xiiUInt32 uiDenominator);

  /// returns true if the division of the numerator by the denominator would result in a full integer
  bool IsIntegral() const;

  /// Equality operator
  bool operator==(const xiiRational& other) const;

  /// Returns the numerator of the rational number
  xiiUInt32 GetNumerator() const;

  /// Returns the denominator
  xiiUInt32 GetDenominator() const;

  /// Returns the result of the division as an integer.
  xiiUInt32 GetIntegralResult() const;

  /// Returns the result of the division as a floating point number (double).
  double GetFloatingPointResult() const;

  /// Returns true if the rational is valid (follows the rules stated in the class description)
  bool IsValid() const;

  /// This helper returns a reduced fraction in case of an integral input.
  ///
  /// Note that this will assert in DEV builds if this class is not integral.
  xiiRational ReduceIntegralFraction() const;

protected:
  xiiUInt32 m_uiNumerator   = 0;
  xiiUInt32 m_uiDenominator = 1;
};

#include <Foundation/Math/Implementation/Rational_inl.h>
