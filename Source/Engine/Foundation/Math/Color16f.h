/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Float16.h>

/// A 16bit per channel float color storage format.
///
/// For any calculations or conversions use xiiColor.
/// \see xiiColor
class XII_FOUNDATION_DLL xiiColorLinear16f
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  // *** Data ***
public:
  xiiFloat16 r;
  xiiFloat16 g;
  xiiFloat16 b;
  xiiFloat16 a;

  // *** Constructors ***
public:
  /// default-constructed color is uninitialized (for speed)
  xiiColorLinear16f(); // [tested]

  /// Initializes the color with r, g, b, a
  xiiColorLinear16f(xiiFloat16 r, xiiFloat16 g, xiiFloat16 b, xiiFloat16 a); // [tested]

  /// Initializes the color with xiiColor
  xiiColorLinear16f(const xiiColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:
  /// Conversion to xiiColor.
  xiiColor ToLinearFloat() const; // [tested]

  /// Conversion to const xiiFloat16*.
  const xiiFloat16* GetData() const { return &r; }

  /// Conversion to xiiFloat16* - use with care!
  xiiFloat16* GetData() { return &r; }
};

#include <Foundation/Math/Implementation/Color16f_inl.h>
