/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Platform/SDL/InputDevice_SDL.h>

struct SDL_Window;

using xiiWindowHandle         = SDL_Window*;
using xiiWindowInternalHandle = xiiWindowHandle;

#define XII_INVALID_WINDOW_HANDLE_VALUE nullptr
