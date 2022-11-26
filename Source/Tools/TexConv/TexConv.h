#pragma once

#include <Foundation/Application/Application.h>

class xiiStreamWriter;

class xiiTexConv : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* key, xiiInt32 val) :
      m_szKey(key), m_iEnumValue(val)
    {
    }

    const char* m_szKey;
    xiiInt32    m_iEnumValue = -1;
  };

  xiiTexConv();

public:
  virtual Execution Run() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;
  virtual void      BeforeCoreSystemsShutdown() override;

  xiiResult ParseCommandLine();
  xiiResult ParseOutputType();
  xiiResult DetectOutputFormat();
  xiiResult ParseInputFiles();
  xiiResult ParseOutputFiles();
  xiiResult ParseChannelMappings();
  xiiResult ParseChannelSliceMapping(xiiInt32 iSlice);
  xiiResult ParseChannelMappingConfig(xiiTexConvChannelMapping& out_Mapping, const char* cfg, xiiInt32 iChannelIndex, bool bSingleChannel);
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

  xiiResult ParseUIntOption(const char* szOption, xiiInt32 iMinValue, xiiInt32 iMaxValue, xiiUInt32& uiResult) const;
  xiiResult ParseStringOption(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed, xiiInt32& iResult) const;
  void      PrintOptionValues(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const;
  void      PrintOptionValuesHelp(const char* szOption, const xiiDynamicArray<KeyEnumValuePair>& allowed) const;
  bool      ParseFile(const char* szOption, xiiString& result) const;

  bool      IsTexFormat() const;
  xiiResult WriteTexFile(xiiStreamWriter& stream, const xiiImage& image);
  xiiResult WriteOutputFile(const char* szFile, const xiiImage& image);

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

  xiiTexConvProcessor m_Processor;
};
