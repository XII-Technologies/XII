#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/IncludeX11.h>
#  include <Foundation/Basics/Platform/Linux/MinX11.h>
#  include <type_traits>

template <typename xiiType, typename X11Type, bool mustBeConvertible>
void xiiVerifyX11Type()
{
  static_assert(sizeof(xiiType) == sizeof(X11Type), "XII <=> X11.h size mismatch");
  static_assert(alignof(xiiType) == alignof(X11Type), "XII <=> X11.h alignment mismatch");
  static_assert(std::is_pointer<xiiType>::value == std::is_pointer<X11Type>::value, "XII <=> X11.h pointer type mismatch");
  static_assert(!mustBeConvertible || xiiConversionTest<xiiType, X11Type>::exists == 1, "XII <=> X11.h conversion failure");
  static_assert(!mustBeConvertible || xiiConversionTest<X11Type, xiiType>::exists == 1, "X11.h <=> XII conversion failure");
};

// Will never be called and thus removed by the linker
void xiiCheckX11TypeSizes()
{
  xiiVerifyX11Type<xiiMinX11::XID, ::Window, true>();
  xiiVerifyX11Type<xiiMinX11::Mask, ::Mask, true>();
  xiiVerifyX11Type<xiiMinX11::Atom, ::Atom, true>();
  xiiVerifyX11Type<xiiMinX11::VisualID, ::VisualID, true>();
  xiiVerifyX11Type<xiiMinX11::Time, ::Time, true>();

  xiiVerifyX11Type<xiiMinX11::Window, ::Window, true>();
  xiiVerifyX11Type<xiiMinX11::Drawable, ::Drawable, true>();
  xiiVerifyX11Type<xiiMinX11::Font, ::Font, true>();
  xiiVerifyX11Type<xiiMinX11::Pixmap, ::Pixmap, true>();
  xiiVerifyX11Type<xiiMinX11::Cursor, ::Cursor, true>();
  xiiVerifyX11Type<xiiMinX11::Colormap, ::Colormap, true>();
  xiiVerifyX11Type<xiiMinX11::GContext, ::GContext, true>();
  xiiVerifyX11Type<xiiMinX11::KeySym, ::KeySym, true>();

  xiiVerifyX11Type<xiiMinX11::KeyCode, ::KeyCode, true>();

  xiiVerifyX11Type<xiiMinX11::xcb_connection_t, ::xcb_connection_t*, false>();
}

#endif
