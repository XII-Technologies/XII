#pragma once

#include <Foundation/Basics.h>

#define XII_INCLUDED_X11_H 1

#if XII_ENABLED(XII_PLATFORM_LINUX)

// Include X11 required headers
#  include <stdio.h>

// clang-format off
#  include <xcb/xcb.h>
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xcms.h>
#  include <X11/Xatom.h>
#  include <X11/XKBlib.h>
#  include <X11/Xresource.h>
#  include <X11/extensions/Xinerama.h>
#  include <X11/Xcursor/Xcursor.h>

#  include <X11/Xlib-xcb.h>
// clang-format on

// Unset X11 macros
#  undef None
#  undef Always
#  undef Bool
#  undef Status
#  undef True
#  undef False
#  undef Success
#  undef Above
#  undef Below
#  undef Always

#  include <Foundation/Basics/Platform/Linux/MinX11.h>

namespace xiiMinX11
{
  template <>
  struct ToNativeImpl<XID>
  {
    typedef ::XID                  type;
    static XII_ALWAYS_INLINE ::XID ToNative(XID xID) { return reinterpret_cast<::XID>(xID); }
  };

  template <>
  struct ToNativeImpl<xcb_connection_t>
  {
    using type = ::xcb_connection_t;
    static XII_ALWAYS_INLINE ::xcb_connection_t* ToNative(xcb_connection_t xcbConnection) { return reinterpret_cast<::xcb_connection_t*>(xcbConnection); }
  };

  template <>
  struct FromNativeImpl<::xcb_connection_t*>
  {
    using type = xcb_connection_t;
    static XII_ALWAYS_INLINE xcb_connection_t FromNative(::xcb_connection_t* xcbConnection) { return reinterpret_cast<xcb_connection_t>(xcbConnection); }
  };

  template <>
  struct FromNativeImpl<::XID>
  {
    using type = XID;
    static XII_ALWAYS_INLINE XID FromNative(::XID xID) { return reinterpret_cast<XID>(xID); }
  };
} // namespace xiiMinX11

#endif
