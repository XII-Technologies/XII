#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingUtils.h>

xiiVec3 xiiBakingUtils::FibonacciSphere(xiiUInt32 uiSampleIndex, xiiUInt32 uiNumSamples)
{
  float offset    = 2.0f / uiNumSamples;
  float increment = xiiMath::Pi<float>() * (3.0f - xiiMath::Sqrt(5.0f));

  float y = ((uiSampleIndex * offset) - 1) + (offset / 2);
  float r = xiiMath::Sqrt(1 - y * y);

  xiiAngle phi = xiiAngle::Radian(((uiSampleIndex + 1) % uiNumSamples) * increment);

  float x = xiiMath::Cos(phi) * r;
  float z = xiiMath::Sin(phi) * r;

  return xiiVec3(x, y, z);
}

static xiiUInt32 s_BitsPerDir[xiiAmbientCubeBasis::NumDirs] = {5, 5, 5, 5, 6, 6};

xiiCompressedSkyVisibility xiiBakingUtils::CompressSkyVisibility(const xiiAmbientCube<float>& skyVisibility)
{
  xiiCompressedSkyVisibility result   = 0;
  xiiUInt32                  uiOffset = 0;
  for (xiiUInt32 i = 0; i < xiiAmbientCubeBasis::NumDirs; ++i)
  {
    float     maxValue      = static_cast<float>((1u << s_BitsPerDir[i]) - 1u);
    xiiUInt32 compressedDir = static_cast<xiiUInt8>(xiiMath::Saturate(skyVisibility.m_Values[i]) * maxValue + 0.5f);
    result |= (compressedDir << uiOffset);
    uiOffset += s_BitsPerDir[i];
  }

  return result;
}

void xiiBakingUtils::DecompressSkyVisibility(xiiCompressedSkyVisibility compressedSkyVisibility, xiiAmbientCube<float>& out_skyVisibility)
{
  xiiUInt32 uiOffset = 0;
  for (xiiUInt32 i = 0; i < xiiAmbientCubeBasis::NumDirs; ++i)
  {
    xiiUInt32 maxValue            = (1u << s_BitsPerDir[i]) - 1u;
    out_skyVisibility.m_Values[i] = static_cast<float>((compressedSkyVisibility >> uiOffset) & maxValue) * (1.0f / maxValue);
    uiOffset += s_BitsPerDir[i];
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingUtils);
