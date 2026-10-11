/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <SharedPluginAssets/TextureAsset/TextureAssetProfile.h>
#include <Texture/Converter/TextureConverterEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct xiiPropertyMetaStateEvent;

struct xiiTexture2DChannelMappingEnum
{
  using StorageType = xiiInt8;

  enum Enum
  {
    R1,

    RG1,
    R1_G2,

    RGB1,
    R1_G2_B3,

    RGBA1,
    RGB1_A2,
    RGB1_ABLACK,
    R1_G2_B3_A4,

    Default = RGB1,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTexture2DChannelMappingEnum);

struct xiiRenderTargetFormat
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    RGBA8sRgb,
    RGBA8,
    RGB10,
    RGBA16,

    Default = RGBA8sRgb
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiRenderTargetFormat);

class xiiTextureAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetProperties, xiiReflectedClass);

public:
  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  const char* GetInputFile(xiiInt32 iInput) const { return m_Input[iInput]; }

  void        SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void        SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void        SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void        SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }

  xiiString GetAbsoluteInputFilePath(xiiInt32 iInput) const;

  xiiTexture2DChannelMappingEnum::Enum GetChannelMapping() const { return m_ChannelMapping; }

  xiiInt32 GetNumInputFiles() const;

  /// Builds the persistent native sampler. Failure leaves the destination unchanged.
  xiiResult GetSamplerDescription(xiiGALSamplerCreationDescription& out_description) const;

  bool  m_bIsRenderTarget        = false;
  bool  m_bPremultipliedAlpha    = false;
  bool  m_bFlipHorizontal        = false;
  bool  m_bDilateColor           = false;
  bool  m_bPreserveAlphaCoverage = false;
  float m_fResolutionScale       = 1.0f;
  float m_fHdrExposureBias       = 0;
  float m_fAlphaThreshold        = 0.5f;

  xiiEnum<xiiTextureConverterSamplerPreset> m_SamplerPreset;
  bool                                      m_bUseCustomFiltering = false;
  xiiEnum<xiiGALFilterType>                 m_MinFilter           = xiiGALFilterType::Linear;
  xiiEnum<xiiGALFilterType>                 m_MagFilter           = xiiGALFilterType::Linear;
  xiiEnum<xiiGALFilterType>                 m_MipFilter           = xiiGALFilterType::Linear;
  xiiUInt32                                 m_uiMaxAnisotropy     = 0;
  float                                     m_fMipLODBias         = 0;
  float                                     m_fMinLOD             = 0;
  float                                     m_fMaxLOD             = xiiMath::MaxValue<float>();
  xiiEnum<xiiGALTextureAddressMode>         m_AddressModeU        = xiiGALTextureAddressMode::Wrap;
  xiiEnum<xiiGALTextureAddressMode>         m_AddressModeV        = xiiGALTextureAddressMode::Wrap;
  xiiEnum<xiiGALTextureAddressMode>         m_AddressModeW        = xiiGALTextureAddressMode::Wrap;
  xiiEnum<xiiTexture2DResolution>           m_Resolution;
  xiiEnum<xiiTextureConverterUsage>         m_TextureUsage;
  xiiEnum<xiiRenderTargetFormat>            m_RtFormat;

  xiiEnum<xiiTextureConverterCompressionMode> m_CompressionMode;
  xiiEnum<xiiTextureConverterMipmapMode>      m_MipmapMode;

private:
  xiiEnum<xiiTexture2DChannelMappingEnum> m_ChannelMapping;
  xiiString                               m_Input[4];
};
