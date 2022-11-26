
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Hash helper to be used as a template argument to xiiHashTable / xiiHashSet for case insensitive string keys.
struct XII_FOUNDATION_DLL xiiHashHelperString_NoCase
{
  inline static xiiUInt32 Hash(xiiStringView szValue); // [tested]

  XII_ALWAYS_INLINE static bool Equal(xiiStringView lhs, xiiStringView rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>
