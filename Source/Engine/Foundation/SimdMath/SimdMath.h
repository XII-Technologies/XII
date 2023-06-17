#pragma once

#include <Foundation/SimdMath/SimdVec4d.h>
#include <Foundation/SimdMath/SimdVec4i.h>

struct xiiSimdMath
{
  static xiiSimdVec4f Exp(const xiiSimdVec4f& f);
  static xiiSimdVec4d Exp(const xiiSimdVec4d& f);
  static xiiSimdVec4f Ln(const xiiSimdVec4f& f);
  static xiiSimdVec4d Ln(const xiiSimdVec4d& f);
  static xiiSimdVec4f Log2(const xiiSimdVec4f& f);
  static xiiSimdVec4d Log2(const xiiSimdVec4d& f);
  static xiiSimdVec4i Log2i(const xiiSimdVec4i& i);
  static xiiSimdVec4f Log10(const xiiSimdVec4f& f);
  static xiiSimdVec4d Log10(const xiiSimdVec4d& f);
  static xiiSimdVec4f Pow2(const xiiSimdVec4f& f);
  static xiiSimdVec4d Pow2(const xiiSimdVec4d& f);

  static xiiSimdVec4f Sin(const xiiSimdVec4f& f);
  static xiiSimdVec4d Sin(const xiiSimdVec4d& f);
  static xiiSimdVec4f Cos(const xiiSimdVec4f& f);
  static xiiSimdVec4d Cos(const xiiSimdVec4d& f);
  static xiiSimdVec4f Tan(const xiiSimdVec4f& f);
  static xiiSimdVec4d Tan(const xiiSimdVec4d& f);

  static xiiSimdVec4f ASin(const xiiSimdVec4f& f);
  static xiiSimdVec4d ASin(const xiiSimdVec4d& f);
  static xiiSimdVec4f ACos(const xiiSimdVec4f& f);
  static xiiSimdVec4d ACos(const xiiSimdVec4d& f);
  static xiiSimdVec4f ATan(const xiiSimdVec4f& f);
  static xiiSimdVec4d ATan(const xiiSimdVec4d& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
