#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

struct xiiSimdMath
{
  static xiiSimdVec4f Sin(const xiiSimdVec4f& f);
  static xiiSimdVec4f Cos(const xiiSimdVec4f& f);
  static xiiSimdVec4f Tan(const xiiSimdVec4f& f);

  static xiiSimdVec4f ASin(const xiiSimdVec4f& f);
  static xiiSimdVec4f ACos(const xiiSimdVec4f& f);
  static xiiSimdVec4f ATan(const xiiSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
