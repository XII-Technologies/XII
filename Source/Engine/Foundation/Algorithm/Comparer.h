/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// A comparer object is used in sorting algorithms to compare to objects of the same type.
template <typename T>
struct xiiCompareHelper
{
  /// Returns true if a is less than b.
  XII_ALWAYS_INLINE bool Less(const T& a, const T& b) const
  {
    return a < b;
  }

  /// Returns true if a is less than b.
  template <typename U>
  XII_ALWAYS_INLINE bool Less(const T& a, const U& b) const
  {
    return a < b;
  }

  /// Returns true if a is less than b.
  template <typename U>
  XII_ALWAYS_INLINE bool Less(const U& a, const T& b) const
  {
    return a < b;
  }

  /// Returns true if a is equal to b.
  XII_ALWAYS_INLINE bool Equal(const T& a, const T& b) const
  {
    return a == b;
  }

  /// Returns true if a is equal to b.
  template <typename U>
  XII_ALWAYS_INLINE bool Equal(const T& a, const U& b) const
  {
    return a == b;
  }

  /// Returns true if a is equal to b.
  template <typename U>
  XII_ALWAYS_INLINE bool Equal(const U& a, const T& b) const
  {
    return a == b;
  }
};

// See <Foundation/Strings/String.h> for xiiString specialization and case insensitive version.
