/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// Hash helper to be used as a template argument to xiiHashTable / xiiHashSet for case insensitive string keys.
struct XII_FOUNDATION_DLL xiiHashHelperString_NoCase
{
  static xiiUInt32 Hash(xiiStringView sValue); // [tested]

  static bool Equal(xiiStringView lhs, xiiStringView rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>
