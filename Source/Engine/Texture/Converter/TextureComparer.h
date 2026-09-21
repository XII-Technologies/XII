/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Image/Image.h>

/// Input options for xiiTextureComparer
class XII_TEXTURE_DLL xiiTextureComparerDescription
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTextureComparerDescription);

public:
  xiiTextureComparerDescription() = default;

  /// Path to a file to load as a reference image. Optional, if m_ExpectedImage is already filled out.
  xiiString m_sExpectedFile;

  /// Path to a file to load as the input image. Optional, if m_ActualImage is already filled out.
  xiiString m_sActualFile;

  /// The reference image to compare. Ignored if m_sExpectedFile is filled out.
  xiiImage m_ExpectedImage;

  /// The image to compare. Ignored if m_sActualFile is filled out.
  xiiImage m_ActualImage;

  /// If enabled, the image comparison allows for more wiggle room.
  /// For images containing single-pixel rasterized lines.
  bool m_bRelaxedComparison = false;

  /// If the comparison yields a larger MSE than this, the images are considered to be too different.
  xiiUInt32 m_MeanSquareErrorThreshold = 100;
};

/// Compares two images and generates various outputs.
class XII_TEXTURE_DLL xiiTextureComparer
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTextureComparer);

public:
  xiiTextureComparer();

  /// The input data to compare.
  xiiTextureComparerDescription m_Descriptor;

  /// Executes the comparison and fill out the public variables to describe the result.
  xiiResult Compare();

  /// If true, the mean-square error of the difference was larger than the threshold.
  bool m_bExceededMSE = false;
  /// The MSE of the difference image.
  xiiUInt32 m_OutputMSE = 0;

  /// The (normalized) difference image.
  xiiImage m_OutputImageDiff;
  /// Only the RGB part of the (normalized) difference image.
  xiiImage m_OutputImageDiffRgb;
  /// Only the Alpha part of the (normalized) difference image.
  xiiImage m_OutputImageDiffAlpha;

  /// Only the RGB part of the actual input image.
  xiiImage m_ExtractedActualRgb;
  /// Only the RGB part of the reference input image.
  xiiImage m_ExtractedExpectedRgb;
  /// Only the Alpha part of the actual input image.
  xiiImage m_ExtractedActualAlpha;
  /// Only the Alpha part of the reference input image.
  xiiImage m_ExtractedExpectedAlpha;

  /// Min/Max difference of the RGB and Alpha images.
  xiiUInt8 m_uiOutputMinDiffRgb   = 0;
  xiiUInt8 m_uiOutputMaxDiffRgb   = 0;
  xiiUInt8 m_uiOutputMinDiffAlpha = 0;
  xiiUInt8 m_uiOutputMaxDiffAlpha = 0;

private:
  xiiResult LoadInputImages();
  xiiResult ComputeMSE();
  xiiResult ExtractImages();
};
