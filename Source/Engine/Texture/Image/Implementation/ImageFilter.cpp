#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

xiiSimdFloat xiiImageFilter::GetWidth() const
{
  return m_fWidth;
}

xiiImageFilter::xiiImageFilter(float width) :
  m_fWidth(width)
{
}

xiiImageFilterBox::xiiImageFilterBox(float width) :
  xiiImageFilter(width)
{
}

xiiSimdFloat xiiImageFilterBox::SamplePoint(const xiiSimdFloat& x) const
{
  xiiSimdFloat absX = x.Abs();

  if (absX <= GetWidth())
  {
    return 1.0f;
  }
  else
  {
    return 0.0f;
  }
}

xiiImageFilterTriangle::xiiImageFilterTriangle(float width) :
  xiiImageFilter(width)
{
}

xiiSimdFloat xiiImageFilterTriangle::SamplePoint(const xiiSimdFloat& x) const
{
  xiiSimdFloat absX = x.Abs();

  xiiSimdFloat width = GetWidth();

  if (absX <= width)
  {
    return width - absX;
  }
  else
  {
    return 0.0f;
  }
}

static xiiSimdFloat sinc(const xiiSimdFloat& x)
{
  xiiSimdFloat absX = x.Abs();

  // Use Taylor expansion for small values to avoid division
  if (absX < 0.0001f)
  {
    // sin(x) / x = (x - x^3/6 + x^5/120 - ...) / x = 1 - x^2/6 + x^4/120 - ...
    return xiiSimdFloat(1.0f) - x * x * xiiSimdFloat(1.0f / 6.0f);
  }
  else
  {
    return xiiMath::Sin(xiiAngle::Radian(x)) / x;
  }
}

static xiiSimdFloat modifiedBessel0(const xiiSimdFloat& x)
{
  // Implementation as I0(x) = sum((1/4 * x * x) ^ k / (k!)^2, k, 0, inf), see
  // http://mathworld.wolfram.com/ModifiedBesselFunctionoftheFirstKind.html

  xiiSimdFloat sum = 1.0f;

  xiiSimdFloat xSquared = x * x * xiiSimdFloat(0.25f);

  xiiSimdFloat currentTerm = xSquared;

  for (xiiUInt32 i = 2; currentTerm > 0.001f; ++i)
  {
    sum += currentTerm;
    currentTerm *= xSquared / xiiSimdFloat(i * i);
  }

  return sum;
}

xiiImageFilterSincWithKaiserWindow::xiiImageFilterSincWithKaiserWindow(float width, float beta) :
  xiiImageFilter(width), m_fBeta(beta), m_fInvBesselBeta(1.0f / modifiedBessel0(m_fBeta))
{
}

xiiSimdFloat xiiImageFilterSincWithKaiserWindow::SamplePoint(const xiiSimdFloat& x) const
{
  xiiSimdFloat scaledX = x / GetWidth();

  xiiSimdFloat xSq = 1.0f - scaledX * scaledX;

  if (xSq <= 0.0f)
  {
    return 0.0f;
  }
  else
  {
    return sinc(x * xiiSimdFloat(xiiMath::Pi<float>())) * modifiedBessel0(m_fBeta * xSq.GetSqrt()) * m_fInvBesselBeta;
  }
}

xiiImageFilterWeights::xiiImageFilterWeights(const xiiImageFilter& filter, xiiUInt32 srcSamples, xiiUInt32 dstSamples)
{
  // Filter weights repeat after the common phase
  xiiUInt32 commonPhase = xiiMath::GreatestCommonDivisor(srcSamples, dstSamples);

  srcSamples /= commonPhase;
  dstSamples /= commonPhase;

  m_uiDstSamplesReduced = dstSamples;

  m_fSourceToDestScale = float(dstSamples) / float(srcSamples);
  m_fDestToSourceScale = float(srcSamples) / float(dstSamples);

  xiiSimdFloat filterScale, invFilterScale;

  if (dstSamples > srcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale    = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale    = m_fDestToSourceScale;
    invFilterScale = m_fSourceToDestScale;
  }

  m_fWidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = xiiUInt32(xiiMath::Ceil(m_fWidthInSourceSpace * xiiSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(dstSamples * m_uiNumWeights);

  for (xiiUInt32 dstSample = 0; dstSample < dstSamples; ++dstSample)
  {
    xiiSimdFloat dstSampleInSourceSpace = (xiiSimdFloat(dstSample) + xiiSimdFloat(0.5f)) * m_fDestToSourceScale;

    xiiInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    xiiSimdFloat totalWeight = 0.0f;

    for (xiiUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      xiiSimdFloat sourceSample = xiiSimdFloat(firstSourceIdx + xiiInt32(weightIdx)) + xiiSimdFloat(0.5f);

      xiiSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_Weights[dstSample * m_uiNumWeights + weightIdx] = weight;
    }

    // Normalize weights
    xiiSimdFloat invWeight = 1.0f / totalWeight;

    for (xiiUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      m_Weights[dstSample * m_uiNumWeights + weightIdx] *= invWeight;
    }
  }
}

xiiUInt32 xiiImageFilterWeights::GetNumWeights() const
{
  return m_uiNumWeights;
}

xiiSimdFloat xiiImageFilterWeights::GetWeight(xiiUInt32 dstSampleIndex, xiiUInt32 weightIndex) const
{
  XII_ASSERT_DEBUG(weightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", weightIndex, m_uiNumWeights);

  return xiiSimdFloat(m_Weights[(dstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + weightIndex]);
}



XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFilter);
