/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <CoreFoundation/CoreFoundation.h>

/// Helper class to release references of core foundation objects correctly.
template <typename T>
class xiiScopedCFRef
{
public:
  xiiScopedCFRef(T Ref) :
    m_Ref(Ref)
  {
  }

  ~xiiScopedCFRef()
  {
    CFRelease(m_Ref);
  }

  operator T() const
  {
    return m_Ref;
  }

private:
  T m_Ref;
};
