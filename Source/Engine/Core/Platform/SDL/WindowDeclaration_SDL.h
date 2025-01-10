#pragma once

#include <Core/Platform/SDL/InputDevice_SDL.h>

struct SDL_Window;

using xiiWindowHandle         = SDL_Window*;
using xiiWindowInternalHandle = SDL_Window*;

#define INVALID_WINDOW_HANDLE_VALUE (SDL_Window*)(nullptr)
