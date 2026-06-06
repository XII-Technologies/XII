/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Application/Application.h>
#include <Texture/Converter/TextureComparer.h>

class xiiStreamWriter;

struct xiiTexConvMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Convert,
    Compare,

    Default = Convert
  };
};

class xiiTexConv : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(xiiStringView sKey, xiiInt32 iVal) :
      m_sKey(sKey), m_iEnumValue(iVal)
    {
    }

    xiiStringView m_sKey;
    xiiInt32      m_iEnumValue = -1;
  };

  xiiTexConv();

public:
  virtual xiiApplication::Execution Run() override;
  virtual xiiResult                 BeforeCoreSystemsStartup() override;
  virtual void                      AfterCoreSystemsStartup() override;
  virtual void                      BeforeCoreSystemsShutdown() override;

  xiiResult ParseCommandLine();
  xiiResult ParseMode();
  xiiResult ParseCompareMode();
  xiiResult ParseOutputType();
  xiiResult DetectOutputFormat();
  xiiResult ParseInputFiles();
  xiiResult ParseOutputFiles();
  xiiResult ParseChannelMappings();
  xiiResult ParseChannelSliceMapping(xiiInt32 iSlice);
  xiiResult ParseChannelMappingConfig(xiiTexConvChannelMapping& out_mapping, xiiStringView sCfg, xiiInt32 iChannelIndex, bool bSingleChannel);
  xiiResult ParseUsage();
  xiiResult ParseMipmapMode();
  xiiResult ParseTargetPlatform();
  xiiResult ParseCompressionMode();
  xiiResult ParseWrapModes();
  xiiResult ParseFilterModes();
  xiiResult ParseResolutionModifiers();
  xiiResult ParseMiscOptions();
  xiiResult ParseAssetHeader();
  xiiResult ParseBumpMapFilter();

  xiiResult ParseUIntOption(xiiStringView sOption, xiiInt32 iMinValue, xiiInt32 iMaxValue, xiiUInt32& ref_uiResult) const;
  xiiResult ParseStringOption(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed, xiiInt32& ref_iResult) const;
  void      PrintOptionValues(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const;
  void      PrintOptionValuesHelp(xiiStringView sOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const;
  bool      ParseFile(xiiStringView sOption, xiiString& ref_sResult) const;

  bool      IsTexFormat() const;
  xiiResult WriteTexFile(xiiStreamWriter& inout_stream, const xiiImage& image);
  xiiResult WriteOutputFile(xiiStringView sFile, const xiiImage& image);

private:
  xiiString m_sOutputFile;
  xiiString m_sOutputThumbnailFile;
  xiiString m_sOutputLowResFile;

  bool m_bOutputSupports2D          = false;
  bool m_bOutputSupports3D          = false;
  bool m_bOutputSupportsCube        = false;
  bool m_bOutputSupportsAtlas       = false;
  bool m_bOutputSupportsMipmaps     = false;
  bool m_bOutputSupportsFiltering   = false;
  bool m_bOutputSupportsCompression = false;

  xiiEnum<xiiTexConvMode> m_Mode;
  xiiTexConvProcessor     m_Processor;

  // Comparer specific

  xiiTextureComparer m_Comparer;
  xiiString      m_sHtmlTitle;
};
