/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/SimdMath/SimdFloat.h>

/// Represents a function used for filtering an image.
class XII_TEXTURE_DLL xiiImageFilter
{
public:
  /// Samples the filter function at a single point. Note that the distribution isn't necessarily normalized.
  virtual xiiSimdFloat SamplePoint(const xiiSimdFloat& x) const = 0;

  /// Returns the width of the filter; outside of the interval [-width, width], the filter function is always zero.
  xiiSimdFloat GetWidth() const;

protected:
  xiiImageFilter(float fWidth);

private:
  xiiSimdFloat m_fWidth;
};

/// Box filter
class XII_TEXTURE_DLL xiiImageFilterBox : public xiiImageFilter
{
public:
  xiiImageFilterBox(float fWidth = 0.5f);

  virtual xiiSimdFloat SamplePoint(const xiiSimdFloat& x) const override;
};

/// Triangle filter
class XII_TEXTURE_DLL xiiImageFilterTriangle : public xiiImageFilter
{
public:
  xiiImageFilterTriangle(float fWidth = 1.0f);

  virtual xiiSimdFloat SamplePoint(const xiiSimdFloat& x) const override;
};

/// Kaiser-windowed sinc filter
class XII_TEXTURE_DLL xiiImageFilterSincWithKaiserWindow : public xiiImageFilter
{
public:
  /// Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
  /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other
  /// filtering tools.
  xiiImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual xiiSimdFloat SamplePoint(const xiiSimdFloat& x) const override;

private:
  xiiSimdFloat m_fBeta;
  xiiSimdFloat m_fInvBesselBeta;
};

/// Pre-computes the required filter weights for rescaling a sequence of image samples.
class XII_TEXTURE_DLL xiiImageFilterWeights
{
public:
  /// Pre-compute the weights for the given filter for scaling between the given number of samples.
  xiiImageFilterWeights(const xiiImageFilter& filter, xiiUInt32 uiSrcSamples, xiiUInt32 uiDstSamples);

  /// Returns the number of weights.
  xiiUInt32 GetNumWeights() const;

  /// Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  xiiSimdFloat GetWeight(xiiUInt32 uiDstSampleIndex, xiiUInt32 uiWeightIndex) const;

  /// Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  xiiInt32 GetFirstSourceSampleIndex(xiiUInt32 uiDstSampleIndex) const;

  xiiArrayPtr<const float> ViewWeights() const;

private:
  xiiHybridArray<float, 16> m_Weights;
  xiiSimdFloat              m_fWidthInSourceSpace;
  xiiSimdFloat              m_fSourceToDestScale;
  xiiSimdFloat              m_fDestToSourceScale;
  xiiUInt32                 m_uiNumWeights;
  xiiUInt32                 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
