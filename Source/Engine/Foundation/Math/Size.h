/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// A simple size class templated on the type for width and height.
///
template <typename Type>
class xiiSizeTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type width;
  Type height;

  // *** Constructors ***
public:
  /// Default constructor does not initialize the data.
  xiiSizeTemplate();

  /// Constructor to set all values.
  xiiSizeTemplate(Type width, Type height);

  /// Static function that returns a zero-size.
  [[nodiscard]] static constexpr xiiSizeTemplate<Type> MakeZero() { return xiiSizeTemplate<Type>(0, 0); }

  /// Returns a size initialized to x,y.
  [[nodiscard]] static constexpr xiiSizeTemplate<Type> Make(Type x, Type y) { return xiiSizeTemplate<Type>(x, y); }

  // *** Common Functions ***
public:
  /// Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;
};

template <typename Type>
constexpr bool operator==(const xiiSizeTemplate<Type>& v1, const xiiSizeTemplate<Type>& v2);

#include <Foundation/Math/Implementation/Size_inl.h>

using xiiSizeU8     = xiiSizeTemplate<xiiUInt8>;
using xiiSizeU16    = xiiSizeTemplate<xiiUInt16>;
using xiiSizeU32    = xiiSizeTemplate<xiiUInt32>;
using xiiSizeU64    = xiiSizeTemplate<xiiUInt64>;
using xiiSizeFloat  = xiiSizeTemplate<float>;
using xiiSizeDouble = xiiSizeTemplate<double>;
using xiiSizeReal   = xiiSizeTemplate<xiiReal>;

XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiSizeU32& arg);
