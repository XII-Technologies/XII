#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

xiiResult xiiTexConvProcessor::LoadInputImages()
{
  XII_PROFILE_SCOPE("Load Images");

  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    xiiLog::Error("No input images have been specified.");
    return XII_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    xiiLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return XII_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
  {
    // make sure the two arrays have the same size
    m_Descriptor.m_InputFiles.SetCount(m_Descriptor.m_InputImages.GetCount());

    xiiStringBuilder tmp;
    for (xiiUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
    {
      tmp.SetFormat("InputImage{}", xiiArgI(i, 2, true));
      m_Descriptor.m_InputFiles[i] = tmp;
    }
  }
  else
  {
    m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

    for (const auto& file : m_Descriptor.m_InputFiles)
    {
      auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
      if (img.LoadFrom(file).Failed())
      {
        xiiLog::Error("Could not load input file '{0}'.", xiiArgSensitive(file, "File"));
        return XII_FAILURE;
      }
    }
  }

  for (xiiUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == xiiImageFormat::UNKNOWN)
    {
      xiiLog::Error("Unknown image format for '{}'", xiiArgSensitive(m_Descriptor.m_InputFiles[i], "File"));
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::ConvertAndScaleImage(xiiStringView sImageName, xiiImage& inout_Image, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTexConvUsage> usage)
{
  const bool bSingleChannel = xiiImageFormat::GetNumChannels(inout_Image.GetImageFormat()) == 1;

  if (inout_Image.Convert(xiiImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    xiiLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", sImageName);
    return XII_FAILURE;
  }

  // some scale operations fail when they are done in place, so use a scratch image as destination for now
  xiiImage scratch;
  if (xiiImageUtils::Scale(inout_Image, scratch, uiResolutionX, uiResolutionY, nullptr, xiiImageAddressMode::Clamp, xiiImageAddressMode::Clamp).Failed())
  {
    xiiLog::Error("Could not resize '{}' to {}x{}", sImageName, uiResolutionX, uiResolutionY);
    return XII_FAILURE;
  }

  inout_Image.ResetAndMove(std::move(scratch));

  if (usage == xiiTexConvUsage::Color && bSingleChannel)
  {
    // replicate single channel ("red" textures) into the other channels
    XII_SUCCEED_OR_RETURN(xiiImageUtils::CopyChannel(inout_Image, 1, inout_Image, 0));
    XII_SUCCEED_OR_RETURN(xiiImageUtils::CopyChannel(inout_Image, 2, inout_Image, 0));
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::ConvertAndScaleInputImages(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTexConvUsage> usage)
{
  XII_PROFILE_SCOPE("ConvertAndScaleInputImages");

  for (xiiUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto&         img   = m_Descriptor.m_InputImages[idx];
    xiiStringView sName = m_Descriptor.m_InputFiles[idx];

    XII_SUCCEED_OR_RETURN(ConvertAndScaleImage(sName, img, uiResolutionX, uiResolutionY, usage));
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_InputFiles);
