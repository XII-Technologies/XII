#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

#include <Foundation/Utilities/CommandLineOptions.h>

xiiCommandLineOptionPath opt_Out("_TexConv", "-out", "Absolute path to main output file.\n\
   ext = tga, dds, xiiTexture2D, xiiTexture3D, xiiTextureCube or xiiTextureAtlas.",
                                 "");


xiiCommandLineOptionDoc opt_In("_TexConv", "-inX", "\"File\"", "Specifies input image X.\n\
   X = 0 .. 63, e.g. -in0, -in1, etc.\n\
   If X is not given, X equals 0.",
                               "");

xiiCommandLineOptionDoc opt_Channels("_TexConv", "-r;-rg;-rgb;-rgba", "inX.rgba", "\
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

xiiCommandLineOptionBool opt_MipsPreserveCoverage("_TexConv", "-mipsPreserveCoverage", "Whether to preserve alpha-coverage in mipmaps for alpha-tested geometry.", false);

xiiCommandLineOptionBool opt_FlipHorz("_TexConv", "-flip_horz", "Whether to flip the output horizontally.", false);

xiiCommandLineOptionBool opt_Dilate("_TexConv", "-dilate", "Dilate/smear color from opaque areas into transparent areas.", false);

xiiCommandLineOptionInt opt_DilateStrength("_TexConv", "-dilateStrength", "How many pixels to smear the image, if -dilate is enabled.", 8, 1, 255);

xiiCommandLineOptionBool opt_Premulalpha("_TexConv", "-premulalpha", "Whether to multiply the alpha channel into the RGB channels.", false);

xiiCommandLineOptionInt opt_ThumbnailRes("_TexConv", "-thumbnailRes", "Thumbnail resolution. Should be a power-of-two.", 0, 32, 1024);

xiiCommandLineOptionPath opt_ThumbnailOut("_TexConv", "-thumbnailOut", "\
  Path to 2D thumbnail file.\n\
  ext = tga, jpg, png\n\
",
                                          "");

xiiCommandLineOptionPath opt_LowOut("_TexConv", "-lowOut", "\
  Path to low-resolution output file.\n\
  ext = Same as main output\n\
",
                                    "");

xiiCommandLineOptionInt opt_LowMips("_TexConv", "-lowMips", "Number of mipmaps to use from main result as low-res data.", 0, 0, 8);

xiiCommandLineOptionInt opt_MinRes("_TexConv", "-minRes", "The minimum resolution allowed for the output.", 16, 4, 8 * 1024);

xiiCommandLineOptionInt opt_MaxRes("_TexConv", "-maxRes", "The maximum resolution allowed for the output.", 1024 * 8, 4, 16 * 1024);

xiiCommandLineOptionInt opt_Downscale("_TexConv", "-downscale", "How often to half the input texture resolution.", 0, 0, 10);

xiiCommandLineOptionFloat opt_MipsAlphaThreshold("_TexConv", "-mipsAlphaThreshold", "Alpha threshold used by renderer for alpha-testing, when alpha-coverage should be preserved.", 0.5f, 0.01f, 0.99f);

xiiCommandLineOptionFloat opt_HdrExposure("_TexConv", "-hdrExposure", "For scaling HDR image brightness up or down.", 0.0f, -20.0f, +20.0f);

xiiCommandLineOptionFloat opt_Clamp("_TexConv", "-clamp", "Input values will be clamped to [-value ; +value].", 64000.0f, -64000.0f, 64000.0f);

xiiCommandLineOptionInt opt_AssetVersion("_TexConv", "-assetVersion", "Asset version number to embed in xii specific output formats", 0, 1, 0xFFFF);

xiiCommandLineOptionString opt_AssetHashLow("_TexConv", "-assetHashLow", "Low part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using xii specific output formats.\n\
Example: -assetHashLow 0xABCDABCD",
                                            "");

xiiCommandLineOptionString opt_AssetHashHigh("_TexConv", "-assetHashHigh", "High part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using xii specific output formats.\n\
Example: -assetHashHigh 0xABCDABCD",
                                             "");

xiiCommandLineOptionEnum opt_Type("_TexConv", "-type", "The type of output to generate.", "2D = 1 | Volume = 2 | Cubemap = 3 | Atlas = 4", 1);

xiiCommandLineOptionEnum opt_Compression("_TexConv", "-compression", "Compression strength for output format.", "Medium = 1 | High = 2 | None = 0", 1);

xiiCommandLineOptionEnum opt_Usage("_TexConv", "-usage", "What type of data the image contains. Affects which final output format is used and how mipmaps are generated.", "Auto = 0 | Color = 1 | Linear = 2 | HDR = 3 | NormalMap = 4 | NormalMap_Inverted = 5 | BumpMap = 6", 0);

xiiCommandLineOptionEnum opt_Mipmaps("_TexConv", "-mipmaps", "Whether to generate mipmaps and with which algorithm.", "None = 0 |Linear = 1 | Kaiser = 2", 1);

xiiCommandLineOptionEnum opt_AddressU("_TexConv", "-addressU", "Which texture address mode to use along U. Only supported by xii-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
xiiCommandLineOptionEnum opt_AddressV("_TexConv", "-addressV", "Which texture address mode to use along V. Only supported by xii-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
xiiCommandLineOptionEnum opt_AddressW("_TexConv", "-addressW", "Which texture address mode to use along W. Only supported by xii-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);

xiiCommandLineOptionEnum opt_Filter("_TexConv", "-filter", "Which texture filter mode to use at runtime. Only supported by xii-specific output formats.", "Default = 9 | Lowest = 7 | Low = 8 | High = 10 | Highest = 11 | Nearest = 0 | Linear = 1 | Trilinear = 2 | Aniso2x = 3 | Aniso4x = 4 | Aniso8x = 5 | Aniso16x = 6", 9);

xiiCommandLineOptionEnum opt_BumpMapFilter("_TexConv", "-bumpMapFilter", "Filter used to approximate the x/y bump map gradients.", "Finite = 0 | Sobel = 1 | Scharr = 2", 0);

xiiCommandLineOptionEnum opt_Platform("_TexConv", "-platform", "What platform to generate the textures for.", "PC | Android", 0);

xiiResult xiiTexConv::ParseCommandLine()
{
  if (xiiCommandLineOption::LogAvailableOptions(xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_TexConv"))
    return XII_FAILURE;

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

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseOutputType()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = xiiTexConvOutputType::None;
    return XII_SUCCESS;
  }

  xiiInt32 value = opt_Type.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_OutputType = static_cast<xiiTexConvOutputType::Enum>(value);

  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Texture2D)
  {
    if (!m_bOutputSupports2D)
    {
      xiiLog::Error("2D textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Cubemap)
  {
    if (!m_bOutputSupportsCube)
    {
      xiiLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas)
  {
    if (!m_bOutputSupportsAtlas)
    {
      xiiLog::Error("Atlas textures are not supported by the chosen output file format.");
      return XII_FAILURE;
    }

    if (!ParseFile("-atlasDesc", m_Processor.m_Descriptor.m_sTextureAtlasDescFile))
      return XII_FAILURE;
  }
  else if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Volume)
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

xiiResult xiiTexConv::ParseInputFiles()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas)
    return XII_SUCCESS;

  xiiStringBuilder tmp, res;
  const auto       pCmd = xiiCommandLineUtils::GetGlobalInstance();

  auto& files = m_Processor.m_Descriptor.m_InputFiles;

  for (xiiUInt32 i = 0; i < 64; ++i)
  {
    tmp.Format("-in{0}", i);

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

  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Cubemap)
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
    xiiLog::Error("No input files were specified. Use \'-in \"path/to/file\"' to specify an input file. Use '-in0', '-in1' etc. to specify "
                  "multiple input files.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseOutputFiles()
{
  const auto pCmd = xiiCommandLineUtils::GetGlobalInstance();

  m_sOutputFile = opt_Out.GetOptionValue(xiiCommandLineOption::LogMode::Always);

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

xiiResult xiiTexConv::ParseUsage()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas)
    return XII_SUCCESS;

  const xiiInt32 value = opt_Usage.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_Usage = static_cast<xiiTexConvUsage::Enum>(value);
  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseMipmapMode()
{
  if (!m_bOutputSupportsMipmaps)
  {
    xiiLog::Info("Selected output format does not support -mipmap options.");

    m_Processor.m_Descriptor.m_MipmapMode = xiiTexConvMipmapMode::None;
    return XII_SUCCESS;
  }

  const xiiInt32 value = opt_Mipmaps.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_MipmapMode = static_cast<xiiTexConvMipmapMode::Enum>(value);

  m_Processor.m_Descriptor.m_bPreserveMipmapCoverage = opt_MipsPreserveCoverage.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  if (m_Processor.m_Descriptor.m_bPreserveMipmapCoverage)
  {
    m_Processor.m_Descriptor.m_fMipmapAlphaThreshold = opt_MipsAlphaThreshold.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseTargetPlatform()
{
  xiiInt32 value = opt_Platform.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);

  m_Processor.m_Descriptor.m_TargetPlatform = static_cast<xiiTexConvTargetPlatform::Enum>(value);
  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseCompressionMode()
{
  if (!m_bOutputSupportsCompression)
  {
    xiiLog::Info("Selected output format does not support -compression options.");

    m_Processor.m_Descriptor.m_CompressionMode = xiiTexConvCompressionMode::None;
    return XII_SUCCESS;
  }

  const xiiInt32 value = opt_Compression.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_CompressionMode = static_cast<xiiTexConvCompressionMode::Enum>(value);
  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseWrapModes()
{
  // cubemaps do not require any wrap mode settings
  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Cubemap || m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas || m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::None)
    return XII_SUCCESS;

  {
    xiiInt32 value                          = opt_AddressU.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeU = static_cast<xiiImageAddressMode::Enum>(value);
  }
  {
    xiiInt32 value                          = opt_AddressV.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeV = static_cast<xiiImageAddressMode::Enum>(value);
  }

  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Volume)
  {
    xiiInt32 value                          = opt_AddressW.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Processor.m_Descriptor.m_AddressModeW = static_cast<xiiImageAddressMode::Enum>(value);
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
  {
    xiiLog::Info("Selected output format does not support -filter options.");
    return XII_SUCCESS;
  }

  xiiInt32 value = opt_Filter.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_FilterMode = static_cast<xiiTextureFilterSetting::Enum>(value);
  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::None)
    return XII_SUCCESS;

  m_Processor.m_Descriptor.m_uiMinResolution  = opt_MinRes.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiMaxResolution  = opt_MaxRes.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiDownscaleSteps = opt_Downscale.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Texture2D || m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::None)
  {
    m_Processor.m_Descriptor.m_bFlipHorizontal = opt_FlipHorz.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    m_Processor.m_Descriptor.m_bPremultiplyAlpha = opt_Premulalpha.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    if (opt_Dilate.GetOptionValue(xiiCommandLineOption::LogMode::Always))
    {
      m_Processor.m_Descriptor.m_uiDilateColor = static_cast<xiiUInt8>(opt_DilateStrength.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    }
  }

  if (m_Processor.m_Descriptor.m_Usage == xiiTexConvUsage::Hdr)
  {
    m_Processor.m_Descriptor.m_fHdrExposureBias = opt_HdrExposure.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  }

  m_Processor.m_Descriptor.m_fMaxValue = opt_Clamp.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseAssetHeader()
{
  const xiiStringView ext = xiiPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("xii"))
    return XII_SUCCESS;

  const auto pCmd = xiiCommandLineUtils::GetGlobalInstance();

  xiiUInt32 tmp = m_Processor.m_Descriptor.m_uiAssetVersion;

  m_Processor.m_Descriptor.m_uiAssetVersion = (xiiUInt16)opt_AssetVersion.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  const xiiUInt64 uiHashLow  = xiiConversionUtils::ConvertHexStringToUInt32(opt_AssetHashLow.GetOptionValue(xiiCommandLineOption::LogMode::Always));
  const xiiUInt64 uiHashHigh = xiiConversionUtils::ConvertHexStringToUInt32(opt_AssetHashHigh.GetOptionValue(xiiCommandLineOption::LogMode::Always));

  m_Processor.m_Descriptor.m_uiAssetHash = (uiHashHigh << 32) | uiHashLow;

  if (m_Processor.m_Descriptor.m_uiAssetHash == 0)
  {
    xiiLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConv::ParseBumpMapFilter()
{
  const xiiInt32 value = opt_BumpMapFilter.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_BumpMapFilter = static_cast<xiiTexConvBumpMapFilter::Enum>(value);
  return XII_SUCCESS;
}
