/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TextureConverter/TextureConverterPCH.h>

#include <TextureConverter/TextureConverter.h>

#include <Foundation/Utilities/CommandLineOptions.h>

xiiCommandLineOptionEnum opt_Mode("_TextureConverter", "-mode", "Mode determines which arguments need to be set.\n\
  In compare mode the mean-square error (MSE) is returned. 0 if it is below the threshold.\
",
                                  "Convert | Compare", 0);

xiiCommandLineOptionPath opt_Out("_TextureConverter", "-out",
                                 "Absolute path to main output file.\n\
   ext = tga, dds, xiiBinTexture2D, xiiBinTexture3D, xiiBinTextureCube or xiiBinTextureAtlas.",
                                 "");


xiiCommandLineOptionDoc opt_In("_TextureConverter", "-inX", "\"File\"",
                               "Specifies input image X.\n\
   X = 0 .. 63, e.g. -in0, -in1, etc.\n\
   If X is not given, X equals 0.",
                               "");

xiiCommandLineOptionDoc opt_Channels("_TextureConverter", "-r;-rg;-rgb;-rgba", "inX.rgba",
                                     "\
  Specifies how many output channels are used (1 - 4) and from which input image to take the data.\n\
  Examples:\n\
  -rgba in0 -> Output has 4 channels, all taken from input image 0.\n\
  -rgb in0 -> Output has 3 channels, all taken from input image 0.\n\
  -rgb in0 -a in1.r -> Output has 4 channels, RGB taken from input image 0 (RGB) Alpha taken from input 1 (Red).\n\
  -rgb in0.bgr -> Output has 3 channels, taken from image 0 and swapped blue and red.\n\
  -r in0.r -g in1.r -b in2.r -a in3.r -> Output has 4 channels, each one taken from another input image (Red).\n\
  -rgb0 in0 -rgb1 in1 -rgb2 in2 -rgb3 in3 -rgb4 in4 -rgb5 in5 -> Output has 3 channels and six faces (-type Cubemap), built from 6 images.\n\
",
                                     "");

xiiCommandLineOptionBool opt_MipsPreserveCoverage("_TextureConverter", "-mipsPreserveCoverage", "Whether to preserve alpha-coverage in mipmaps for alpha-tested geometry.", false);

xiiCommandLineOptionBool opt_FlipHorz("_TextureConverter", "-flip_horz", "Whether to flip the output horizontally.", false);

xiiCommandLineOptionBool opt_Dilate("_TextureConverter", "-dilate", "Dilate/smear color from opaque areas into transparent areas.", false);

xiiCommandLineOptionInt opt_DilateStrength("_TextureConverter", "-dilateStrength", "How many pixels to smear the image, if -dilate is enabled.", 8, 1, 255);

xiiCommandLineOptionBool opt_Premulalpha("_TextureConverter", "-premulalpha", "Whether to multiply the alpha channel into the RGB channels.", false);

xiiCommandLineOptionInt opt_ThumbnailRes("_TextureConverter", "-thumbnailRes", "Thumbnail resolution. Should be a power-of-two.", 0, 32, 1024);

xiiCommandLineOptionPath opt_ThumbnailOut("_TextureConverter", "-thumbnailOut",
                                          "\
  Path to 2D thumbnail file.\n\
  ext = tga, jpg, png\n\
",
                                          "");

xiiCommandLineOptionPath opt_LowOut("_TextureConverter", "-lowOut",
                                    "\
  Path to low-resolution output file.\n\
  ext = Same as main output\n\
",
                                    "");

xiiCommandLineOptionInt opt_LowMips("_TextureConverter", "-lowMips", "Number of mipmaps to use from main result as low-res data.", 0, 0, 8);

xiiCommandLineOptionInt opt_MinRes("_TextureConverter", "-minRes", "The minimum resolution allowed for the output.", 16, 4, 8 * 1024);

xiiCommandLineOptionInt opt_MaxRes("_TextureConverter", "-maxRes", "The maximum resolution allowed for the output.", 1024 * 8, 4, 16 * 1024);

xiiCommandLineOptionInt opt_Downscale("_TextureConverter", "-downscale", "How often to half the input texture resolution.", 0, 0, 10);

xiiCommandLineOptionFloat opt_MipsAlphaThreshold("_TextureConverter", "-mipsAlphaThreshold", "Alpha threshold used by renderer for alpha-testing, when alpha-coverage should be preserved.", 0.5f, 0.01f, 0.99f);

xiiCommandLineOptionFloat opt_HdrExposure("_TextureConverter", "-hdrExposure", "For scaling HDR image brightness up or down.", 0.0f, -20.0f, +20.0f);

xiiCommandLineOptionFloat opt_Clamp("_TextureConverter", "-clamp", "Input values will be clamped to [-value ; +value].", 64000.0f, -64000.0f, 64000.0f);

xiiCommandLineOptionInt opt_AssetVersion("_TextureConverter", "-assetVersion", "Asset version number to embed in XII specific output formats", 0, 1, 0xFFFF);

xiiCommandLineOptionString opt_AssetHashLow("_TextureConverter", "-assetHashLow", "Low part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using XII specific output formats.\n\
Example: -assetHashLow 0xABCDABCD",
                                            "");

xiiCommandLineOptionString opt_AssetHashHigh("_TextureConverter", "-assetHashHigh", "High part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using XII specific output formats.\n\
Example: -assetHashHigh 0xABCDABCD",
                                             "");

xiiCommandLineOptionEnum opt_Type("_TextureConverter", "-type", "The type of output to generate.", "2D = 1 | Volume = 2 | Cubemap = 3 | Atlas = 4", 1);

xiiCommandLineOptionEnum opt_Compression("_TextureConverter", "-compression", "Compression strength for output format.", "Medium = 1 | High = 2 | None = 0", 1);

xiiCommandLineOptionEnum opt_Usage("_TextureConverter", "-usage", "What type of data the image contains. Affects which final output format is used and how mipmaps are generated.", "Auto = 0 | Color = 1 | Linear = 2 | HDR = 3 | NormalMap = 4 | NormalMap_Inverted = 5 | BumpMap = 6", 0);

xiiCommandLineOptionEnum opt_Mipmaps("_TextureConverter", "-mipmaps", "Whether to generate mipmaps and with which algorithm.", "None = 0 |Linear = 1 | Kaiser = 2", 1);

xiiCommandLineOptionEnum opt_AddressU("_TextureConverter", "-addressU", "Which texture address mode to use along U. Only supported by XII specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
xiiCommandLineOptionEnum opt_AddressV("_TextureConverter", "-addressV", "Which texture address mode to use along V. Only supported by XII specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
xiiCommandLineOptionEnum opt_AddressW("_TextureConverter", "-addressW", "Which texture address mode to use along W. Only supported by XII specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);

xiiCommandLineOptionEnum opt_Filter("_TextureConverter", "-filter", "Which texture filter mode to use at runtime. Only supported by XII specific output formats.", "Default = 9 | Lowest = 7 | Low = 8 | High = 10 | Highest = 11 | Nearest = 0 | Linear = 1 | Trilinear = 2 | Aniso2x = 3 | Aniso4x = 4 | Aniso8x = 5 | Aniso16x = 6", 9);

xiiCommandLineOptionEnum opt_BumpMapFilter("_TextureConverter", "-bumpMapFilter", "Filter used to approximate the x/y bump map gradients.", "Finite = 0 | Sobel = 1 | Scharr = 2", 0);

xiiCommandLineOptionEnum opt_Platform("_TextureConverter", "-platform", "What platform to generate the textures for.", "PC", 0);

xiiCommandLineOptionString opt_CompareHtmlTitle("_TextureConverter", "-cmpHtml", "Title for the compare result HTML. If empty no HTML file is written.", "");
xiiCommandLineOptionPath   opt_CompareActual("_TextureConverter", "-cmpImg", "Path to an image to compare with another.", "");
xiiCommandLineOptionPath   opt_CompareExpected("_TextureConverter", "-cmpRef", "Path to a reference image to compare against.", "");
xiiCommandLineOptionInt    opt_CompareThreshold("_TextureConverter", "-cmpMSE", "The error threshold for the comparison to be considered as failed.\n\
  No output files are written, if the image difference is below this value.",
                                                100, 0);
xiiCommandLineOptionBool   opt_CompareRelaxed("_TextureConverter", "-cmpRelaxed", "Use a more lenient comparison method.\nUseful for images with single-pixel wide rasterized lines.", false);


xiiResult xiiTextureConverter::ParseCommandLine()
{
  if (xiiCommandLineOption::LogAvailableOptions(xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_TextureConverter"))
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(ParseMode());

  if (m_Mode == xiiTextureConverterMode::Compare)
  {
    XII_SUCCEED_OR_RETURN(ParseCompareMode());
  }
  else
  {
    XII_SUCCEED_OR_RETURN(ParseOutputFiles());
    XII_SUCCEED_OR_RETURN(DetectOutputFormat());

    XII_SUCCEED_OR_RETURN(ParseOutputType());
    XII_SUCCEED_OR_RETURN(ParseAssetHeader());
    XII_SUCCEED_OR_RETURN(ParseTargetPlatform());
    XII_SUCCEED_OR_RETURN(ParseCompressionMode());
    XII_SUCCEED_OR_RETURN(ParseUsage());
    XII_SUCCEED_OR_RETURN(ParseMipmapMode());
    XII_SUCCEED_OR_RETURN(ParseWrapModes());
    XII_SUCCEED_OR_RETURN(ParseFilterModes());
    XII_SUCCEED_OR_RETURN(ParseResolutionModifiers());
    XII_SUCCEED_OR_RETURN(ParseMiscOptions());
    XII_SUCCEED_OR_RETURN(ParseInputFiles());
    XII_SUCCEED_OR_RETURN(ParseChannelMappings());
    XII_SUCCEED_OR_RETURN(ParseBumpMapFilter());
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseMode()
{
  switch (opt_Mode.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime))
  {
    case 0:
      m_Mode = xiiTextureConverterMode::Convert;
      return XII_SUCCESS;

    case 1:
      m_Mode = xiiTextureConverterMode::Compare;
      return XII_SUCCESS;
  }

  xiiLog::Error("Invalid mode selected.");
  return XII_FAILURE;
}

xiiResult xiiTextureConverter::ParseCompareMode()
{
  m_sOutputFile = opt_Out.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  if (m_sOutputFile.IsEmpty())
  {
    xiiLog::Warning("Output path is not specified. Use option '-out \"path\"' to set the prefix path for the output files.");
  }

  m_sHtmlTitle = opt_CompareHtmlTitle.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

  xiiStringBuilder tmp, res;
  const auto       pCmd = xiiCommandLineUtils::GetGlobalInstance();

  m_Comparer.m_Descriptor.m_sActualFile              = opt_CompareActual.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_sExpectedFile            = opt_CompareExpected.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold = opt_CompareThreshold.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_bRelaxedComparison       = opt_CompareRelaxed.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

  if (m_Comparer.m_Descriptor.m_sActualFile.IsEmpty())
  {
    xiiLog::Error("Image to compare is not specified.");
    return XII_FAILURE;
  }

  if (m_Comparer.m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    xiiLog::Error("Reference image to compare against is not specified.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseOutputType()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = xiiTextureConverterOutputType::None;
    return XII_SUCCESS;
  }

  xiiInt32 value = opt_Type.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_OutputType = static_cast<xiiTextureConverterOutputType::Enum>(value);

  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Texture2D)
  {
    if (!m_bOutputSupports2D)
    {
      xiiLog::Error("2D textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Cubemap)
  {
    if (!m_bOutputSupportsCube)
    {
      xiiLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Atlas)
  {
    if (!m_bOutputSupportsAtlas)
    {
      xiiLog::Error("Atlas textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }

    if (!ParseFile("-atlasDesc", m_Processor.m_Descriptor.m_sTextureAtlasDescFile))
      return XII_FAILURE;
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Volume)
  {
    if (!m_bOutputSupports3D)
    {
      xiiLog::Error("Volume textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseInputFiles()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Atlas)
    return XII_SUCCESS;

  xiiStringBuilder tmp, res;
  const auto       pCmd = xiiCommandLineUtils::GetGlobalInstance();

  auto& files = m_Processor.m_Descriptor.m_InputFiles;

  for (xiiUInt32 i = 0; i < 64; ++i)
  {
    tmp.SetFormat("-in{0}", i);

    res = pCmd->GetAbsolutePathOption(tmp);

    // stop once an option was not found
    if (res.IsEmpty())
      break;

    files.EnsureCount(i + 1);
    files[i] = res;
  }

  // if no numbered inputs were given, try '-in', ignore it otherwise
  if (files.IsEmpty())
  {
    // short version for -in1
    res = pCmd->GetAbsolutePathOption("-in");

    if (!res.IsEmpty())
    {
      files.PushBack(res);
    }
  }

  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Cubemap)
  {
    // 0 = +X = Right
    // 1 = -X = Left
    // 2 = +Y = Top
    // 3 = -Y = Bottom
    // 4 = +Z = Front
    // 5 = -Z = Back

    if (files.IsEmpty() && (pCmd->GetOptionIndex("-right") != -1 || pCmd->GetOptionIndex("-px") != -1))
    {
      files.SetCount(6);

      files[0] = pCmd->GetAbsolutePathOption("-right", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-left", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-top", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-bottom", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-front", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-back", 0, files[5]);

      files[0] = pCmd->GetAbsolutePathOption("-px", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-nx", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-py", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-ny", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-pz", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-nz", 0, files[5]);
    }
  }

  for (xiiUInt32 i = 0; i < files.GetCount(); ++i)
  {
    if (files[i].IsEmpty())
    {
      xiiLog::Error("Input file {} is not specified", i);
      return XII_FAILURE;
    }

    xiiLog::Info("Input file {}: '{}'", i, files[i]);
  }

  if (m_Processor.m_Descriptor.m_InputFiles.IsEmpty())
  {
    xiiLog::Error("No input files were specified. Use \'-in \"path/to/file\"' to specify an input file. Use '-in0', '-in1' etc. to specify multiple input files.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseOutputFiles()
{
  m_sOutputFile          = opt_Out.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_sOutputThumbnailFile = opt_ThumbnailOut.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiThumbnailOutputResolution = opt_ThumbnailRes.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  m_sOutputLowResFile = opt_LowOut.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  if (!m_sOutputLowResFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiLowResMipmaps = opt_LowMips.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseUsage()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Atlas)
    return XII_SUCCESS;

  const xiiInt32 iValue            = opt_Usage.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_Usage = static_cast<xiiTextureConverterUsage::Enum>(iValue);
  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseMipmapMode()
{
  if (!m_bOutputSupportsMipmaps)
  {
    xiiLog::Info("Selected output format does not support -mipmap options.");

    m_Processor.m_Descriptor.m_MipmapMode = xiiTextureConverterMipmapMode::None;
    return XII_SUCCESS;
  }

  const xiiInt32 iValue = opt_Mipmaps.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_MipmapMode              = static_cast<xiiTextureConverterMipmapMode::Enum>(iValue);
  m_Processor.m_Descriptor.m_bPreserveMipmapCoverage = opt_MipsPreserveCoverage.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  if (m_Processor.m_Descriptor.m_bPreserveMipmapCoverage)
  {
    m_Processor.m_Descriptor.m_fMipmapAlphaThreshold = opt_MipsAlphaThreshold.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseTargetPlatform()
{
  xiiInt32 iValue                           = opt_Platform.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
  m_Processor.m_Descriptor.m_TargetPlatform = static_cast<xiiTextureConverterTargetPlatform::Enum>(iValue);
  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseCompressionMode()
{
  if (!m_bOutputSupportsCompression)
  {
    xiiLog::Info("Selected output format does not support -compression options.");

    m_Processor.m_Descriptor.m_CompressionMode = xiiTextureConverterCompressionMode::None;
    return XII_SUCCESS;
  }

  const xiiInt32 iValue                      = opt_Compression.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_CompressionMode = static_cast<xiiTextureConverterCompressionMode::Enum>(iValue);
  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseWrapModes()
{
  // cubemaps do not require any wrap mode settings
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Cubemap || m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Atlas || m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::None)
    return XII_SUCCESS;

  {
    xiiInt32 iValue                         = opt_AddressU.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeU = static_cast<xiiImageAddressMode::Enum>(iValue);
  }
  {
    xiiInt32 iValue                         = opt_AddressV.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeV = static_cast<xiiImageAddressMode::Enum>(iValue);
  }

  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Volume)
  {
    xiiInt32 iValue                         = opt_AddressW.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Processor.m_Descriptor.m_AddressModeW = static_cast<xiiImageAddressMode::Enum>(iValue);
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
  {
    xiiLog::Info("Selected output format does not support -filter options.");
    return XII_SUCCESS;
  }

  xiiInt32 iValue                       = opt_Filter.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_FilterMode = static_cast<xiiTextureFilterSetting::Enum>(iValue);
  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::None)
    return XII_SUCCESS;

  m_Processor.m_Descriptor.m_uiMinResolution  = opt_MinRes.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiMaxResolution  = opt_MaxRes.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiDownscaleSteps = opt_Downscale.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Texture2D || m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::None)
  {
    m_Processor.m_Descriptor.m_bFlipHorizontal = opt_FlipHorz.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    m_Processor.m_Descriptor.m_bPremultiplyAlpha = opt_Premulalpha.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    if (opt_Dilate.GetOptionValue(xiiCommandLineOption::LogMode::Always))
    {
      m_Processor.m_Descriptor.m_uiDilateColor = static_cast<xiiUInt8>(opt_DilateStrength.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    }
  }

  if (m_Processor.m_Descriptor.m_Usage == xiiTextureConverterUsage::Hdr)
  {
    m_Processor.m_Descriptor.m_fHdrExposureBias = opt_HdrExposure.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  m_Processor.m_Descriptor.m_fMaxValue = opt_Clamp.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseAssetHeader()
{
  const xiiStringView ext = xiiPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("xii"))
    return XII_SUCCESS;

  m_Processor.m_Descriptor.m_uiAssetVersion = (xiiUInt16)opt_AssetVersion.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  xiiUInt32 uiHashLow  = 0;
  xiiUInt32 uiHashHigh = 0;
  if (xiiConversionUtils::ConvertHexStringToUInt32(opt_AssetHashLow.GetOptionValue(xiiCommandLineOption::LogMode::Always), uiHashLow).Failed() || xiiConversionUtils::ConvertHexStringToUInt32(opt_AssetHashHigh.GetOptionValue(xiiCommandLineOption::LogMode::Always), uiHashHigh).Failed())
  {
    xiiLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return XII_FAILURE;
  }

  m_Processor.m_Descriptor.m_uiAssetHash = (static_cast<xiiUInt64>(uiHashHigh) << 32) | static_cast<xiiUInt64>(uiHashLow);

  if (m_Processor.m_Descriptor.m_uiAssetHash == 0)
  {
    xiiLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseBumpMapFilter()
{
  const xiiInt32 iValue = opt_BumpMapFilter.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_BumpMapFilter = static_cast<xiiTextureConverterBumpMapFilter::Enum>(iValue);
  return XII_SUCCESS;
}
