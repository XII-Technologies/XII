#pragma once

namespace xiiMinX11
{
  using XID      = unsigned long;
  using Mask     = unsigned long;
  using Atom     = unsigned long;
  using VisualID = unsigned long;
  using Time     = unsigned long;

  struct xii_xcb_connection_t;
  using xcb_connection_t = xii_xcb_connection_t*;

  using Window   = XID;
  using Drawable = XID;
  using Font     = XID;
  using Pixmap   = XID;
  using Cursor   = XID;
  using Colormap = XID;
  using GContext = XID;
  using KeySym   = XID;

  using KeyCode = xiiUInt8;

  template <typename T>
  struct ToNativeImpl
  {
  };

  template <typename T>
  struct FromNativeImpl
  {
  };

  // Helper function to convert xiiMinX11 types into native X11 types.
  /// Include IncludeX11.h before using it.
  template <typename T>
  XII_ALWAYS_INLINE typename ToNativeImpl<T>::type ToNative(T t)
  {
    return ToNativeImpl<T>::ToNative(t);
  }

  /// Helper function to native X11 types to xiiMinX11 types.
  /// Include IncludeX11.h before using it.
  template <typename T>
  XII_ALWAYS_INLINE typename FromNativeImpl<T>::type FromNative(T t)
  {
    return FromNativeImpl<T>::FromNative(t);
  }
} // namespace xiiMinX11
