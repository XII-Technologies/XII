/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexture2DChannelMappingEnum, 1)
  XII_ENUM_CONSTANTS(xiiTexture2DChannelMappingEnum::R1)
  XII_ENUM_CONSTANTS(xiiTexture2DChannelMappingEnum::RG1, xiiTexture2DChannelMappingEnum::R1_G2)
  XII_ENUM_CONSTANTS(xiiTexture2DChannelMappingEnum::RGB1, xiiTexture2DChannelMappingEnum::RGB1_ABLACK, xiiTexture2DChannelMappingEnum::R1_G2_B3)
  XII_ENUM_CONSTANTS(xiiTexture2DChannelMappingEnum::RGBA1, xiiTexture2DChannelMappingEnum::RGB1_A2, xiiTexture2DChannelMappingEnum::R1_G2_B3_A4)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexture2DResolution, 1)
  XII_ENUM_CONSTANTS(xiiTexture2DResolution::Fixed64x64, xiiTexture2DResolution::Fixed128x128, xiiTexture2DResolution::Fixed256x256, xiiTexture2DResolution::Fixed512x512, xiiTexture2DResolution::Fixed1024x1024, xiiTexture2DResolution::Fixed2048x2048)
  XII_ENUM_CONSTANTS(xiiTexture2DResolution::CVarRtResolution1, xiiTexture2DResolution::CVarRtResolution2)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderTargetFormat, 1)
  XII_ENUM_CONSTANTS(xiiRenderTargetFormat::RGBA8sRgb, xiiRenderTargetFormat::RGBA8, xiiRenderTargetFormat::RGB10, xiiRenderTargetFormat::RGBA16)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetProperties, 5, xiiRTTIDefaultAllocator<xiiTextureAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("IsRenderTarget", m_bIsRenderTarget)->AddAttributes(new xiiHiddenAttribute),
    XII_ENUM_MEMBER_PROPERTY("Usage", xiiTextureConverterUsage, m_TextureUsage),

    XII_ENUM_MEMBER_PROPERTY("Format", xiiRenderTargetFormat, m_RtFormat),
    XII_ENUM_MEMBER_PROPERTY("Resolution", xiiTexture2DResolution, m_Resolution),
    XII_MEMBER_PROPERTY("CVarResScale", m_fCVarResolutionScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, 10.0f)),

    XII_ENUM_MEMBER_PROPERTY("MipmapMode", xiiTexConvMipmapMode, m_MipmapMode),
    XII_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    XII_MEMBER_PROPERTY("AlphaThreshold", m_fAlphaThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ENUM_MEMBER_PROPERTY("CompressionMode", xiiTexConvCompressionMode, m_CompressionMode),
    XII_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),
    XII_MEMBER_PROPERTY("DilateColor", m_bDilateColor)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("FlipHorizontal", m_bFlipHorizontal),
    XII_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new xiiClampValueAttribute(-20.0f, 20.0f)),

    XII_ENUM_MEMBER_PROPERTY("TextureFilter", xiiTextureFilterSetting, m_TextureFilter),
    XII_ENUM_MEMBER_PROPERTY("AddressModeU", xiiImageAddressMode, m_AddressModeU),
    XII_ENUM_MEMBER_PROPERTY("AddressModeV", xiiImageAddressMode, m_AddressModeV),
    XII_ENUM_MEMBER_PROPERTY("AddressModeW", xiiImageAddressMode, m_AddressModeW),

    XII_ENUM_MEMBER_PROPERTY("ChannelMapping", xiiTexture2DChannelMappingEnum, m_ChannelMapping),

    XII_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", xiiFileBrowserAttribute::ImagesLdrAndHdr)),
    XII_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", xiiFileBrowserAttribute::ImagesLdrAndHdr)),
    XII_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", xiiFileBrowserAttribute::ImagesLdrAndHdr)),
    XII_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", xiiFileBrowserAttribute::ImagesLdrAndHdr)),

  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiTextureAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiTextureAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isRenderTarget = e.m_pObject->GetTypeAccessor().GetValue("IsRenderTarget").ConvertTo<bool>();

    props["AddressModeW"].m_Visibility = xiiPropertyUiState::Invisible;

    if (isRenderTarget)
    {
      const xiiInt32 resMode   = e.m_pObject->GetTypeAccessor().GetValue("Resolution").ConvertTo<xiiInt32>();
      const bool     resIsCVar = resMode == xiiTexture2DResolution::CVarRtResolution1 || resMode == xiiTexture2DResolution::CVarRtResolution2;

      props["CVarResScale"].m_Visibility          = resIsCVar ? xiiPropertyUiState::Default : xiiPropertyUiState::Disabled;
      props["Usage"].m_Visibility                 = xiiPropertyUiState::Invisible;
      props["MipmapMode"].m_Visibility            = xiiPropertyUiState::Invisible;
      props["CompressionMode"].m_Visibility       = xiiPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility    = xiiPropertyUiState::Invisible;
      props["FlipHorizontal"].m_Visibility        = xiiPropertyUiState::Invisible;
      props["ChannelMapping"].m_Visibility        = xiiPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = xiiPropertyUiState::Invisible;
      props["AlphaThreshold"].m_Visibility        = xiiPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility    = xiiPropertyUiState::Invisible;
      props["HdrExposureBias"].m_Visibility       = xiiPropertyUiState::Invisible;
      props["DilateColor"].m_Visibility           = xiiPropertyUiState::Invisible;

      props["Input1"].m_Visibility = xiiPropertyUiState::Invisible;
      props["Input2"].m_Visibility = xiiPropertyUiState::Invisible;
      props["Input3"].m_Visibility = xiiPropertyUiState::Invisible;
      props["Input4"].m_Visibility = xiiPropertyUiState::Invisible;

      props["Format"].m_Visibility     = xiiPropertyUiState::Default;
      props["Resolution"].m_Visibility = xiiPropertyUiState::Default;
    }
    else
    {
      const bool hasMips = e.m_pObject->GetTypeAccessor().GetValue("MipmapMode").ConvertTo<xiiInt32>() != xiiTexConvMipmapMode::None;
      const bool isHDR   = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<xiiInt32>() == xiiTextureConverterUsage::Hdr;

      props["CVarResScale"].m_Visibility          = xiiPropertyUiState::Invisible;
      props["Usage"].m_Visibility                 = xiiPropertyUiState::Default;
      props["Mipmaps"].m_Visibility               = xiiPropertyUiState::Default;
      props["Compression"].m_Visibility           = xiiPropertyUiState::Default;
      props["PremultipliedAlpha"].m_Visibility    = xiiPropertyUiState::Disabled;
      props["FlipHorizontal"].m_Visibility        = xiiPropertyUiState::Default;
      props["ChannelMapping"].m_Visibility        = xiiPropertyUiState::Default;
      props["Format"].m_Visibility                = xiiPropertyUiState::Invisible;
      props["Resolution"].m_Visibility            = xiiPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = xiiPropertyUiState::Disabled;
      props["AlphaThreshold"].m_Visibility        = xiiPropertyUiState::Disabled;
      props["HdrExposureBias"].m_Visibility       = xiiPropertyUiState::Disabled;
      props["DilateColor"].m_Visibility           = xiiPropertyUiState::Disabled;

      const xiiInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<xiiInt64>();

      props["Usage"].m_Visibility  = xiiPropertyUiState::Default;
      props["Input1"].m_Visibility = xiiPropertyUiState::Default;
      props["Input2"].m_Visibility = xiiPropertyUiState::Invisible;
      props["Input3"].m_Visibility = xiiPropertyUiState::Invisible;
      props["Input4"].m_Visibility = xiiPropertyUiState::Invisible;

      {
        props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
        props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
        props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
        props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      }

      switch (mapping)
      {
        case xiiTexture2DChannelMappingEnum::R1_G2_B3_A4:
          props["Input4"].m_Visibility = xiiPropertyUiState::Default;
          // fall through

        case xiiTexture2DChannelMappingEnum::R1_G2_B3:
          props["Input3"].m_Visibility = xiiPropertyUiState::Default;
          // fall through

        case xiiTexture2DChannelMappingEnum::RGB1_A2:
        case xiiTexture2DChannelMappingEnum::R1_G2:
          props["Input2"].m_Visibility = xiiPropertyUiState::Default;
          break;
      }

      if (mapping == xiiTexture2DChannelMappingEnum::R1 || mapping == xiiTexture2DChannelMappingEnum::RGBA1 ||
          mapping == xiiTexture2DChannelMappingEnum::R1_G2_B3_A4 || mapping == xiiTexture2DChannelMappingEnum::RGB1_A2 ||
          mapping == xiiTexture2DChannelMappingEnum::R1_G2_B3_A4)
      {
        if (mapping != xiiTexture2DChannelMappingEnum::R1)
        {
          props["PremultipliedAlpha"].m_Visibility = xiiPropertyUiState::Default;
          props["DilateColor"].m_Visibility        = xiiPropertyUiState::Default;
        }

        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = xiiPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility        = xiiPropertyUiState::Default;
        }
      }

      if (isHDR)
      {
        props["HdrExposureBias"].m_Visibility = xiiPropertyUiState::Default;
      }
    }

    // always hide this, feature may be removed at some point
    props["PremultipliedAlpha"].m_Visibility = xiiPropertyUiState::Invisible;
  }
}

xiiString xiiTextureAssetProperties::GetAbsoluteInputFilePath(xiiInt32 iInput) const
{
  xiiStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

xiiInt32 xiiTextureAssetProperties::GetNumInputFiles() const
{
  if (m_bIsRenderTarget)
    return 0;

  switch (m_ChannelMapping)
  {
    case xiiTexture2DChannelMappingEnum::R1:
    case xiiTexture2DChannelMappingEnum::RG1:
    case xiiTexture2DChannelMappingEnum::RGB1:
    case xiiTexture2DChannelMappingEnum::RGB1_ABLACK:
    case xiiTexture2DChannelMappingEnum::RGBA1:
      return 1;

    case xiiTexture2DChannelMappingEnum::R1_G2:
    case xiiTexture2DChannelMappingEnum::RGB1_A2:
      return 2;

    case xiiTexture2DChannelMappingEnum::R1_G2_B3:
      return 3;

    case xiiTexture2DChannelMappingEnum::R1_G2_B3_A4:
      return 4;
  }

  XII_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiTextureAssetPropertiesPatch_2_3 : public xiiGraphPatch
{
public:
  xiiTextureAssetPropertiesPatch_2_3() :
    xiiGraphPatch("xiiTextureAssetProperties", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pMipmaps = pNode->FindProperty("Mipmaps");
    if (pMipmaps && pMipmaps->m_Value.IsA<bool>())
    {
      if (pMipmaps->m_Value.Get<bool>())
        pNode->AddProperty("MipmapMode", (xiiInt32)xiiTexConvMipmapMode::Kaiser);
      else
        pNode->AddProperty("MipmapMode", (xiiInt32)xiiTexConvMipmapMode::None);
    }

    auto* pCompression = pNode->FindProperty("Compression");
    if (pCompression && pCompression->m_Value.IsA<bool>())
    {
      if (pCompression->m_Value.Get<bool>())
        pNode->AddProperty("CompressionMode", (xiiInt32)xiiTexConvCompressionMode::High);
      else
        pNode->AddProperty("CompressionMode", (xiiInt32)xiiTexConvCompressionMode::None);
    }
  }
};

xiiTextureAssetPropertiesPatch_2_3 g_xiiTextureAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class xiiTextureAssetPropertiesPatch_3_4 : public xiiGraphPatch
{
public:
  xiiTextureAssetPropertiesPatch_3_4() :
    xiiGraphPatch("xiiTextureAssetProperties", 4)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    const char* szAddressModes[] = {"AddressModeU", "AddressModeV", "AddressModeW"};

    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto* pAddress = pNode->FindProperty(szAddressModes[i]);
      if (pAddress && pAddress->m_Value.IsA<xiiString>())
      {
        if (pAddress->m_Value.Get<xiiString>() == "xiiTexture2DAddressMode::Wrap")
        {
          pNode->ChangeProperty(szAddressModes[i], (xiiInt32)xiiImageAddressMode::Repeat);
        }
        else if (pAddress->m_Value.Get<xiiString>() == "xiiTexture2DAddressMode::Clamp")
        {
          pNode->ChangeProperty(szAddressModes[i], (xiiInt32)xiiImageAddressMode::Clamp);
        }
        else if (pAddress->m_Value.Get<xiiString>() == "xiiTexture2DAddressMode::Mirror")
        {
          pNode->ChangeProperty(szAddressModes[i], (xiiInt32)xiiImageAddressMode::Mirror);
        }
      }
    }
  }
};

xiiTextureAssetPropertiesPatch_3_4 g_xiiTextureAssetPropertiesPatch_3_4;

//////////////////////////////////////////////////////////////////////////

class xiiTextureAssetPropertiesPatch_4_5 : public xiiGraphPatch
{
public:
  xiiTextureAssetPropertiesPatch_4_5() :
    xiiGraphPatch("xiiTextureAssetProperties", 5)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<xiiString>())
    {
      if (pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTextureConverterUsage::Auto);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Diffuse" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::EmissiveColor")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTextureConverterUsage::Color);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Height" || pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Mask" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::LookupTable" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::EmissiveMask")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTextureConverterUsage::Linear);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::NormalMap")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTextureConverterUsage::NormalMap);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTexture2DUsageEnum::HDR")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTextureConverterUsage::Hdr);
      }
    }
  }
};

xiiTextureAssetPropertiesPatch_4_5 g_xiiTextureAssetPropertiesPatch_4_5;
