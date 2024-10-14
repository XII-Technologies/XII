#pragma once

namespace xiiMinWindows
{
  using BOOL  = xiiInt32;
  using DWORD = unsigned long;
  using UINT  = xiiUInt32;
  using LPSTR = char*;

  struct xiiHINSTANCE;
  using HINSTANCE = xiiHINSTANCE*;
  using HMODULE   = HINSTANCE;

  struct xiiHWND;
  using HWND = xiiHWND*;

  using HRESULT = signed long;
  using HANDLE  = void*;

#if XII_ENABLED(XII_PLATFORM_64BIT)
  using WPARAM = xiiUInt64;
  using LPARAM = xiiUInt64;
#else
  using WPARAM = xiiUInt32;
  using LPARAM = xiiUInt32;
#endif

  template <typename T>
  struct ToNativeImpl
  {
  };

  template <typename T>
  struct FromNativeImpl
  {
  };

  /// Helper function to convert xiiMinWindows types into native windows.h types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  XII_ALWAYS_INLINE typename ToNativeImpl<T>::type ToNative(T t)
  {
    return ToNativeImpl<T>::ToNative(t);
  }

  /// Helper function to native windows.h types to xiiMinWindows types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  XII_ALWAYS_INLINE typename FromNativeImpl<T>::type FromNative(T t)
  {
    return FromNativeImpl<T>::FromNative(t);
  }
} // namespace xiiMinWindows

#define XII_WINDOWS_CALLBACK             __stdcall
#define XII_WINDOWS_WINAPI               __stdcall
#define XII_WINDOWS_INVALID_HANDLE_VALUE ((void*)(signed long long)-1)
