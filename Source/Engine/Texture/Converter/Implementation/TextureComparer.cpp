/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexComparer.h>

xiiTextureComparer::xiiTextureComparer() = default;

xiiResult xiiTextureComparer::Compare()
{
  XII_PROFILE_SCOPE("Compare");

  XII_SUCCEED_OR_RETURN(LoadInputImages());

  if ((m_Descriptor.m_ActualImage.GetWidth() != m_Descriptor.m_ExpectedImage.GetWidth()) || (m_Descriptor.m_ActualImage.GetHeight() != m_Descriptor.m_ExpectedImage.GetHeight()))
  {
    xiiLog::Error("Image sizes are not identical: {}x{} != {}x{}", m_Descriptor.m_ActualImage.GetWidth(), m_Descriptor.m_ActualImage.GetHeight(), m_Descriptor.m_ExpectedImage.GetWidth(), m_Descriptor.m_ExpectedImage.GetHeight());
    return XII_FAILURE;
  }

  XII_SUCCEED_OR_RETURN(ComputeMSE());

  if (m_OutputMSE > m_Descriptor.m_MeanSquareErrorThreshold)
  {
    m_bExceededMSE = true;

    XII_SUCCEED_OR_RETURN(ExtractImages());
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureComparer::LoadInputImages()
{
  XII_PROFILE_SCOPE("Load Images");

  if (!m_Descriptor.m_sActualFile.IsEmpty())
  {
    if (m_Descriptor.m_ActualImage.LoadFrom(m_Descriptor.m_sActualFile).Failed())
    {
      xiiLog::Error("Could not load image file '{0}'.", xiiArgSensitive(m_Descriptor.m_sActualFile, "File"));
      return XII_FAILURE;
    }
  }

  if (!m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    if (m_Descriptor.m_ExpectedImage.LoadFrom(m_Descriptor.m_sExpectedFile).Failed())
    {
      xiiLog::Error("Could not load reference file '{0}'.", xiiArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
      return XII_FAILURE;
    }
  }

  if (!m_Descriptor.m_ActualImage.IsValid())
  {
    xiiLog::Error("No image available.");
    return XII_FAILURE;
  }

  if (!m_Descriptor.m_ExpectedImage.IsValid())
  {
    xiiLog::Error("No reference image available.");
    return XII_FAILURE;
  }

  if (m_Descriptor.m_ActualImage.GetImageFormat() == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Unknown image format for '{}'", xiiArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return XII_FAILURE;
  }

  if (m_Descriptor.m_ExpectedImage.GetImageFormat() == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Unknown image format for '{}'", xiiArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return XII_FAILURE;
  }

  if (xiiImageConversion::Convert(m_Descriptor.m_ActualImage, m_Descriptor.m_ActualImage, xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    xiiLog::Error("Could not convert to RGBA8: '{}'", xiiArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return XII_FAILURE;
  }

  if (xiiImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_Descriptor.m_ExpectedImage, xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    xiiLog::Error("Could not convert to RGBA8: '{}'", xiiArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureComparer::ComputeMSE()
{
  XII_PROFILE_SCOPE("ComputeMSE");

  if (m_Descriptor.m_bRelaxedComparison)
    xiiImageUtils::ComputeImageDifferenceABSRelaxed(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);
  else
    xiiImageUtils::ComputeImageDifferenceABS(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);

  m_OutputMSE = xiiImageUtils::ComputeMeanSquareError(m_OutputImageDiff, 32);

  return XII_SUCCESS;
}

xiiResult xiiTextureComparer::ExtractImages()
{
  XII_PROFILE_SCOPE("ExtractImages");

  xiiImageUtils::Normalize(m_OutputImageDiff, m_uiOutputMinDiffRgb, m_uiOutputMaxDiffRgb, m_uiOutputMinDiffAlpha, m_uiOutputMaxDiffAlpha);

  XII_SUCCEED_OR_RETURN(xiiImageConversion::Convert(m_OutputImageDiff, m_OutputImageDiffRgb, xiiImageFormat::R8G8B8_UNORM));

  xiiImageUtils::ExtractAlphaChannel(m_OutputImageDiff, m_OutputImageDiffAlpha);

  XII_SUCCEED_OR_RETURN(xiiImageConversion::Convert(m_Descriptor.m_ActualImage, m_ExtractedActualRgb, xiiImageFormat::R8G8B8_UNORM));
  xiiImageUtils::ExtractAlphaChannel(m_Descriptor.m_ActualImage, m_ExtractedActualAlpha);

  XII_SUCCEED_OR_RETURN(xiiImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedRgb, xiiImageFormat::R8G8B8_UNORM));
  xiiImageUtils::ExtractAlphaChannel(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedAlpha);

  return XII_SUCCESS;
}
