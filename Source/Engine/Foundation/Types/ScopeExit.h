/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// \file

/// Macro to execute a piece of code when the current scope closes.
#define XII_SCOPE_EXIT(code) auto XII_PP_CONCAT(scopeExit_, XII_SOURCE_LINE) = xiiMakeScopeExit([&]() { code; })

/// \internal Helper class to implement XII_SCOPE_EXIT
template <typename T>
struct xiiScopeExit
{
  XII_ALWAYS_INLINE xiiScopeExit(T&& func) :
    m_func(std::forward<T>(func))
  {
  }

  XII_ALWAYS_INLINE ~xiiScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement XII_SCOPE_EXIT
template <typename T>
XII_ALWAYS_INLINE xiiScopeExit<T> xiiMakeScopeExit(T&& func)
{
  return xiiScopeExit<T>(std::forward<T>(func));
}
