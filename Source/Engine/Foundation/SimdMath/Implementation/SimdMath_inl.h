/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

///\todo optimize these methods if needed

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Exp(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_exp_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Exp(f.x()), xiiMath::Exp(f.y()), xiiMath::Exp(f.z()), xiiMath::Exp(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Exp(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_exp_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Exp(f.x()), xiiMath::Exp(f.y()), xiiMath::Exp(f.z()), xiiMath::Exp(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Ln(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_log_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Ln(f.x()), xiiMath::Ln(f.y()), xiiMath::Ln(f.z()), xiiMath::Ln(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Ln(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_log_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Ln(f.x()), xiiMath::Ln(f.y()), xiiMath::Ln(f.z()), xiiMath::Ln(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Log2(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_log2_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Log2(f.x()), xiiMath::Log2(f.y()), xiiMath::Log2(f.z()), xiiMath::Log2(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Log2(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_log2_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Log2(f.x()), xiiMath::Log2(f.y()), xiiMath::Log2(f.z()), xiiMath::Log2(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4i xiiSimdMath::Log2i(const xiiSimdVec4i& i)
{
  return xiiSimdVec4i(xiiMath::Log2i(i.x()), xiiMath::Log2i(i.y()), xiiMath::Log2i(i.z()), xiiMath::Log2i(i.w()));
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Log10(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_log10_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Log10(f.x()), xiiMath::Log10(f.y()), xiiMath::Log10(f.z()), xiiMath::Log10(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Log10(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_log10_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Log10(f.x()), xiiMath::Log10(f.y()), xiiMath::Log10(f.z()), xiiMath::Log10(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Pow2(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_exp2_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Pow2(f.x()), xiiMath::Pow2(f.y()), xiiMath::Pow2(f.z()), xiiMath::Pow2(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Pow2(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_exp2_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Pow2(f.x()), xiiMath::Pow2(f.y()), xiiMath::Pow2(f.z()), xiiMath::Pow2(f.w()));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Sin(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_sin_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Sin(xiiAngle::MakeFromRadian(f.x())), xiiMath::Sin(xiiAngle::MakeFromRadian(f.y())), xiiMath::Sin(xiiAngle::MakeFromRadian(f.z())), xiiMath::Sin(xiiAngle::MakeFromRadian(f.w())));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Sin(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_sin_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Sin(xiiAngled::MakeFromRadian(f.x())), xiiMath::Sin(xiiAngled::MakeFromRadian(f.y())), xiiMath::Sin(xiiAngled::MakeFromRadian(f.z())), xiiMath::Sin(xiiAngled::MakeFromRadian(f.w())));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Cos(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_cos_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Cos(xiiAngle::MakeFromRadian(f.x())), xiiMath::Cos(xiiAngle::MakeFromRadian(f.y())), xiiMath::Cos(xiiAngle::MakeFromRadian(f.z())), xiiMath::Cos(xiiAngle::MakeFromRadian(f.w())));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Cos(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_cos_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Cos(xiiAngled::MakeFromRadian(f.x())), xiiMath::Cos(xiiAngled::MakeFromRadian(f.y())), xiiMath::Cos(xiiAngled::MakeFromRadian(f.z())), xiiMath::Cos(xiiAngled::MakeFromRadian(f.w())));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::Tan(const xiiSimdVec4f& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm_tan_ps(f.m_v);
#else
  return xiiSimdVec4f(xiiMath::Tan(xiiAngle::MakeFromRadian(f.x())), xiiMath::Tan(xiiAngle::MakeFromRadian(f.y())), xiiMath::Tan(xiiAngle::MakeFromRadian(f.z())), xiiMath::Tan(xiiAngle::MakeFromRadian(f.w())));
#endif
}

// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::Tan(const xiiSimdVec4d& f)
{
#if XII_ENABLED(XII_COMPILER_MSVC) && (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
  return _mm256_tan_pd(f.m_v);
#else
  return xiiSimdVec4d(xiiMath::Tan(xiiAngled::MakeFromRadian(f.x())), xiiMath::Tan(xiiAngled::MakeFromRadian(f.y())), xiiMath::Tan(xiiAngled::MakeFromRadian(f.z())), xiiMath::Tan(xiiAngled::MakeFromRadian(f.w())));
#endif
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdMath::ASin(const xiiSimdVec4f& f)
{
  return xiiSimdVec4f(xiiMath::Pi<float>() * 0.5f) - ACos(f);
}

// static
XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdMath::ASin(const xiiSimdVec4d& f)
{
  return xiiSimdVec4d(xiiMath::Pi<double>() * 0.5) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::ACos(const xiiSimdVec4f& f)
{
  xiiSimdVec4f x1 = f.Abs();
  xiiSimdVec4f x2 = x1.CompMul(x1);
  xiiSimdVec4f x3 = x2.CompMul(x1);

  xiiSimdVec4f s = x1 * -0.2121144f + xiiSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((xiiSimdVec4f(1.0f) - x1).GetSqrt());

  return xiiSimdVec4f::Select(f >= xiiSimdVec4f::MakeZero(), s, xiiSimdVec4f(xiiMath::Pi<float>()) - s);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::ACos(const xiiSimdVec4d& f)
{
  xiiSimdVec4d x1 = f.Abs();
  xiiSimdVec4d x2 = x1.CompMul(x1);
  xiiSimdVec4d x3 = x2.CompMul(x1);

  xiiSimdVec4d s = x1 * -0.2121144 + xiiSimdVec4d(1.5707288);
  s += x2 * 0.0742610;
  s += x3 * -0.0187293;
  s = s.CompMul((xiiSimdVec4d(1.0) - x1).GetSqrt());

  return xiiSimdVec4d::Select(f >= xiiSimdVec4d::MakeZero(), s, xiiSimdVec4d(xiiMath::Pi<double>()) - s);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
XII_FORCE_INLINE xiiSimdVec4f xiiSimdMath::ATan(const xiiSimdVec4f& f)
{
  xiiSimdVec4f x    = f.Abs();
  xiiSimdVec4f t0   = xiiSimdVec4f::Select(x < xiiSimdVec4f(1.0f), x, x.GetReciprocal());
  xiiSimdVec4f t1   = t0.CompMul(t0);
  xiiSimdVec4f poly = xiiSimdVec4f(0.0872929f);
  poly              = xiiSimdVec4f(-0.301895f) + poly.CompMul(t1);
  poly              = xiiSimdVec4f(1.0f) + poly.CompMul(t1);
  poly              = poly.CompMul(t0);
  t0                = xiiSimdVec4f::Select(x < xiiSimdVec4f(1.0f), poly, xiiSimdVec4f(xiiMath::Pi<float>() * 0.5f) - poly);

  return xiiSimdVec4f::Select(f < xiiSimdVec4f::MakeZero(), -t0, t0);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
XII_FORCE_INLINE xiiSimdVec4d xiiSimdMath::ATan(const xiiSimdVec4d& f)
{
  xiiSimdVec4d x    = f.Abs();
  xiiSimdVec4d t0   = xiiSimdVec4d::Select(x < xiiSimdVec4d(1.0f), x, x.GetReciprocal());
  xiiSimdVec4d t1   = t0.CompMul(t0);
  xiiSimdVec4d poly = xiiSimdVec4d(0.0872929);
  poly              = xiiSimdVec4d(-0.301895) + poly.CompMul(t1);
  poly              = xiiSimdVec4d(1.0) + poly.CompMul(t1);
  poly              = poly.CompMul(t0);
  t0                = xiiSimdVec4d::Select(x < xiiSimdVec4d(1.0), poly, xiiSimdVec4d(xiiMath::Pi<float>() * 0.5) - poly);

  return xiiSimdVec4d::Select(f < xiiSimdVec4d::MakeZero(), -t0, t0);
}
