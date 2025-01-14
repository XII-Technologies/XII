#pragma once

#include <Core/Platform/SDL/InputDevice_SDL.h>

struct SDL_Window;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/MinWindows.h>

using xiiWindowHandle         = xiiMinWindows::HWND;
using xiiWindowInternalHandle = SDL_Window*;
#else
#  error "Platform window handle not implemented."
#endif
#define INVALID_WINDOW_HANDLE_VALUE (SDL_Window*)(nullptr)
