#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdNoise.h>

xiiSimdPerlinNoise::xiiSimdPerlinNoise(xiiUInt32 uiSeed)
{
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_Permutations); ++i)
  {
    m_Permutations[i] = static_cast<xiiUInt8>(i);
  }

  xiiRandom rnd;
  rnd.Initialize(uiSeed);

  for (xiiUInt32 i = XII_ARRAY_SIZE(m_Permutations) - 1; i > 0; --i)
  {
    xiiUInt32 uiRandomIndex = rnd.UIntInRange(XII_ARRAY_SIZE(m_Permutations));
    xiiMath::Swap(m_Permutations[i], m_Permutations[uiRandomIndex]);
  }
}

xiiSimdVec4f xiiSimdPerlinNoise::NoiseZeroToOne(const xiiSimdVec4f& inX, const xiiSimdVec4f& inY, const xiiSimdVec4f& inZ, xiiUInt32 uiNumOctaves /*= 1*/)
{
  xiiSimdVec4f result    = xiiSimdVec4f::ZeroVector();
  xiiSimdFloat amplitude = 1.0f;
  xiiUInt32    uiOffset  = 0;

  uiNumOctaves = xiiMath::Max(uiNumOctaves, 1u);
  for (xiiUInt32 i = 0; i < uiNumOctaves; ++i)
  {
    xiiSimdFloat scale  = static_cast<float>(XII_BIT(i));
    xiiSimdVec4f offset = Permute(xiiSimdVec4i(uiOffset) + xiiSimdVec4i(0, 1, 2, 3)).ToFloat();
    xiiSimdVec4f x      = inX * scale + offset.Get<xiiSwizzle::XXXX>();
    xiiSimdVec4f y      = inY * scale + offset.Get<xiiSwizzle::YYYY>();
    xiiSimdVec4f z      = inZ * scale + offset.Get<xiiSwizzle::ZZZZ>();

    result += Noise(x, y, z) * amplitude;

    amplitude *= 0.5f;
    uiOffset += 23;
  }

  return result * 0.5f + xiiSimdVec4f(0.5f);
}

namespace
{
  XII_FORCE_INLINE xiiSimdVec4f Fade(const xiiSimdVec4f& t)
  {
    return t.CompMul(t).CompMul(t).CompMul(t.CompMul(t * 6.0f - xiiSimdVec4f(15.0f)) + xiiSimdVec4f(10.0f));
  }

  XII_FORCE_INLINE xiiSimdVec4f Grad(xiiSimdVec4i hash, const xiiSimdVec4f& x, const xiiSimdVec4f& y, const xiiSimdVec4f& z)
  {
    // convert low 4 bits of hash code into 12 gradient directions.
    const xiiSimdVec4i h = hash & xiiSimdVec4i(15);
    const xiiSimdVec4f u = xiiSimdVec4f::Select(h < xiiSimdVec4i(8), x, y);
    const xiiSimdVec4f v = xiiSimdVec4f::Select(h < xiiSimdVec4i(4), y, xiiSimdVec4f::Select(h == xiiSimdVec4i(12) || h == xiiSimdVec4i(14), x, z));
    return xiiSimdVec4f::Select((h & xiiSimdVec4i(1)) == xiiSimdVec4i::ZeroVector(), u, -u) +
      xiiSimdVec4f::Select((h & xiiSimdVec4i(2)) == xiiSimdVec4i::ZeroVector(), v, -v);
  }

  XII_ALWAYS_INLINE xiiSimdVec4f Lerp(const xiiSimdVec4f& t, const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return xiiSimdVec4f::Lerp(a, b, t); }

} // namespace

// reference: https://mrl.nyu.edu/~perlin/noise/
xiiSimdVec4f xiiSimdPerlinNoise::Noise(const xiiSimdVec4f& inX, const xiiSimdVec4f& inY, const xiiSimdVec4f& inZ)
{
  xiiSimdVec4f x = inX;
  xiiSimdVec4f y = inY;
  xiiSimdVec4f z = inZ;

  // find unit cube that contains point.
  const xiiSimdVec4f xFloored = x.Floor();
  const xiiSimdVec4f yFloored = y.Floor();
  const xiiSimdVec4f zFloored = z.Floor();

  const xiiSimdVec4i maxIndex = xiiSimdVec4i(255);
  const xiiSimdVec4i X        = xiiSimdVec4i::Truncate(xFloored) & maxIndex;
  const xiiSimdVec4i Y        = xiiSimdVec4i::Truncate(yFloored) & maxIndex;
  const xiiSimdVec4i Z        = xiiSimdVec4i::Truncate(zFloored) & maxIndex;

  // find relative x,y,z of point in cube.
  x -= xFloored;
  y -= yFloored;
  z -= zFloored;

  // compute fade curves for each of x,y,z.
  const xiiSimdVec4f u = Fade(x);
  const xiiSimdVec4f v = Fade(y);
  const xiiSimdVec4f w = Fade(z);

  // hash coordinates of the 8 cube corners
  const xiiSimdVec4i i1 = xiiSimdVec4i(1);
  const xiiSimdVec4i A  = Permute(X) + Y;
  const xiiSimdVec4i AA = Permute(A) + Z;
  const xiiSimdVec4i AB = Permute(A + i1) + Z;
  const xiiSimdVec4i B  = Permute(X + i1) + Y;
  const xiiSimdVec4i BA = Permute(B) + Z;
  const xiiSimdVec4i BB = Permute(B + i1) + Z;

  const xiiSimdVec4f f1 = xiiSimdVec4f(1.0f);

  // and add blended results from 8 corners of cube.
  const xiiSimdVec4f c000 = Grad(Permute(AA), x, y, z);
  const xiiSimdVec4f c100 = Grad(Permute(BA), x - f1, y, z);
  const xiiSimdVec4f c010 = Grad(Permute(AB), x, y - f1, z);
  const xiiSimdVec4f c110 = Grad(Permute(BB), x - f1, y - f1, z);
  const xiiSimdVec4f c001 = Grad(Permute(AA + i1), x, y, z - f1);
  const xiiSimdVec4f c101 = Grad(Permute(BA + i1), x - f1, y, z - f1);
  const xiiSimdVec4f c011 = Grad(Permute(AB + i1), x, y - f1, z - f1);
  const xiiSimdVec4f c111 = Grad(Permute(BB + i1), x - f1, y - f1, z - f1);

  const xiiSimdVec4f c000_c100 = Lerp(u, c000, c100);
  const xiiSimdVec4f c010_c110 = Lerp(u, c010, c110);
  const xiiSimdVec4f c001_c101 = Lerp(u, c001, c101);
  const xiiSimdVec4f c011_c111 = Lerp(u, c011, c111);

  return Lerp(w, Lerp(v, c000_c100, c010_c110), Lerp(v, c001_c101, c011_c111));
}


XII_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdNoise);
