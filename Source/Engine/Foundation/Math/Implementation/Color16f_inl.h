
inline xiiColorLinear16f::xiiColorLinear16f() {}

inline xiiColorLinear16f::xiiColorLinear16f(xiiFloat16 r, xiiFloat16 g, xiiFloat16 b, xiiFloat16 a) :
  r(r), g(g), b(b), a(a)
{
}

inline xiiColorLinear16f::xiiColorLinear16f(const xiiColor& color) :
  r(color.r), g(color.g), b(color.b), a(color.a)
{
}

inline xiiColor xiiColorLinear16f::ToLinearFloat() const
{
  return xiiColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
}
