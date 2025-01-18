#pragma once

#include <Core/Platform/Android/InputDevice_android.h>

struct ANativeWindow;

using xiiWindowHandle         = ANativeWindow*;
using xiiWindowInternalHandle = xiiWindowHandle;

#define INVALID_WINDOW_HANDLE_VALUE nullptr
