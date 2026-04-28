/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Platform/SDL/InputDevice_SDL.h>

struct SDL_Window;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/MinWindows.h>

using xiiWindowHandle         = xiiMinWindows::HWND;
using xiiWindowInternalHandle = SDL_Window*;

#  define INVALID_WINDOW_HANDLE_VALUE    (xiiWindowHandle)(0)
#  define INVALID_INTERNAL_WINDOW_HANDLE nullptr

#elif XII_ENABLED(XII_PLATFORM_LINUX)
struct xiiWindowHandle
{
  enum class Type
  {
    Invalid = 0,
    SDL, ///< Used by the Runtime.
  };

  Type m_Type;

  union
  {
    SDL_Window* m_pSDLWindow;
  };

  bool operator==(xiiWindowHandle& rhs)
  {
    if (m_Type != rhs.m_Type)
      return false;

    if (m_Type == Type::SDL)
    {
      return m_pSDLWindow == rhs.m_pSDLWindow;
    }
    else
    {
      return false;
    }
  }

  operator SDL_Window*() const
  {
    return m_pSDLWindow;
  }
};

using xiiWindowInternalHandle = xiiWindowHandle;

#  define INVALID_WINDOW_HANDLE_VALUE \
    xiiWindowHandle {}
#  define INVALID_INTERNAL_WINDOW_HANDLE INVALID_WINDOW_HANDLE_VALUE

#else
#  error "Platform window handle not implemented."
#endif
