#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

struct xiiSimdMath
{
  static xiiSimdVec4f Exp(const xiiSimdVec4f& f);
  static xiiSimdVec4f Ln(const xiiSimdVec4f& f);
  static xiiSimdVec4f Log2(const xiiSimdVec4f& f);
  static xiiSimdVec4i Log2i(const xiiSimdVec4i& i);
  static xiiSimdVec4f Log10(const xiiSimdVec4f& f);
  static xiiSimdVec4f Pow2(const xiiSimdVec4f& f);

  static xiiSimdVec4f Sin(const xiiSimdVec4f& f);
  static xiiSimdVec4f Cos(const xiiSimdVec4f& f);
  static xiiSimdVec4f Tan(const xiiSimdVec4f& f);

  static xiiSimdVec4f ASin(const xiiSimdVec4f& f);
  static xiiSimdVec4f ACos(const xiiSimdVec4f& f);
  static xiiSimdVec4f ATan(const xiiSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
