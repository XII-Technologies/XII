/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

class XII_FOUNDATION_DLL xiiSimdPerlinNoise
{
public:
  xiiSimdPerlinNoise(xiiUInt32 uiSeed);

  xiiSimdVec4f NoiseZeroToOne(const xiiSimdVec4f& x, const xiiSimdVec4f& y, const xiiSimdVec4f& z, xiiUInt32 uiNumOctaves = 1);

private:
  xiiSimdVec4f Noise(const xiiSimdVec4f& x, const xiiSimdVec4f& y, const xiiSimdVec4f& z);

  XII_FORCE_INLINE xiiSimdVec4i Permute(const xiiSimdVec4i& v)
  {
#if 0
    xiiArrayPtr<xiiUInt8> p = xiiMakeArrayPtr(m_Permutations);
#else
    xiiUInt8* p = m_Permutations;
#endif

    xiiSimdVec4i i = v & xiiSimdVec4i(XII_ARRAY_SIZE(m_Permutations) - 1);
    return xiiSimdVec4i(p[i.x()], p[i.y()], p[i.z()], p[i.w()]);
  }

  xiiUInt8 m_Permutations[256];
};
