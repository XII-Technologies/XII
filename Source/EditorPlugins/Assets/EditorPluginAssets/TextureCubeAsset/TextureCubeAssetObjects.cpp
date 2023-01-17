#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTextureCubeChannelMappingEnum, 1)
  XII_ENUM_CONSTANTS(xiiTextureCubeChannelMappingEnum::RGB1, xiiTextureCubeChannelMappingEnum::RGBA1, xiiTextureCubeChannelMappingEnum::RGB1TO6, xiiTextureCubeChannelMappingEnum::RGBA1TO6)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeAssetProperties, 3, xiiRTTIDefaultAllocator<xiiTextureCubeAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Usage", xiiTexConvUsage, m_TextureUsage),

    XII_ENUM_MEMBER_PROPERTY("MipmapMode", xiiTexConvMipmapMode, m_MipmapMode),
    XII_ENUM_MEMBER_PROPERTY("CompressionMode", xiiTexConvCompressionMode, m_CompressionMode),

    XII_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new xiiClampValueAttribute(-20.0f, 20.0f)),

    XII_ENUM_MEMBER_PROPERTY("TextureFilter", xiiTextureFilterSetting, m_TextureFilter),

    XII_ENUM_MEMBER_PROPERTY("ChannelMapping", xiiTextureCubeChannelMappingEnum, m_ChannelMapping),

    XII_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    XII_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    XII_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    XII_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    XII_ACCESSOR_PROPERTY("Input5", GetInputFile4, SetInputFile4)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    XII_ACCESSOR_PROPERTY("Input6", GetInputFile5, SetInputFile5)->AddAttributes(new xiiFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),

  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiTextureCubeAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiTextureCubeAssetProperties>())
  {
    const xiiInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<xiiInt64>();
    const bool     isHDR   = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<xiiInt32>() == xiiTexConvUsage::Hdr;

    auto& props = *e.m_pPropertyStates;

    props["Usage"].m_Visibility           = xiiPropertyUiState::Default;
    props["Input1"].m_Visibility          = xiiPropertyUiState::Default;
    props["Input2"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["Input3"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["Input4"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["Input5"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["Input6"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["HdrExposureBias"].m_Visibility = xiiPropertyUiState::Disabled;

    if (isHDR)
    {
      props["HdrExposureBias"].m_Visibility = xiiPropertyUiState::Default;
    }

    if (mapping == xiiTextureCubeChannelMappingEnum::RGB1TO6 || mapping == xiiTextureCubeChannelMappingEnum::RGBA1TO6)
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::CM_Right";
      props["Input2"].m_sNewLabelText = "TextureAsset::CM_Left";
      props["Input3"].m_sNewLabelText = "TextureAsset::CM_Top";
      props["Input4"].m_sNewLabelText = "TextureAsset::CM_Bottom";
      props["Input5"].m_sNewLabelText = "TextureAsset::CM_Front";
      props["Input6"].m_sNewLabelText = "TextureAsset::CM_Back";
    }
    else
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
      props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
      props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
      props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      props["Input5"].m_sNewLabelText = "TextureAsset::Input5";
      props["Input6"].m_sNewLabelText = "TextureAsset::Input6";
    }

    switch (mapping)
    {
      case xiiTextureCubeChannelMappingEnum::RGB1TO6:
      case xiiTextureCubeChannelMappingEnum::RGBA1TO6:
        props["Input6"].m_Visibility = xiiPropertyUiState::Default;
        props["Input5"].m_Visibility = xiiPropertyUiState::Default;
        props["Input4"].m_Visibility = xiiPropertyUiState::Default;
        props["Input3"].m_Visibility = xiiPropertyUiState::Default;
        props["Input2"].m_Visibility = xiiPropertyUiState::Default;
        break;
    }
  }
}

xiiString xiiTextureCubeAssetProperties::GetAbsoluteInputFilePath(xiiInt32 iInput) const
{
  xiiStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

xiiInt32 xiiTextureCubeAssetProperties::GetNumInputFiles() const
{
  switch (m_ChannelMapping)
  {
    case xiiTextureCubeChannelMappingEnum::RGB1:
    case xiiTextureCubeChannelMappingEnum::RGBA1:
      return 1;

    case xiiTextureCubeChannelMappingEnum::RGB1TO6:
    case xiiTextureCubeChannelMappingEnum::RGBA1TO6:
      return 6;
  }

  XII_REPORT_FAILURE("Invalid Code Path");
  return 1;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiTextureCubeAssetProperties_2_3 : public xiiGraphPatch
{
public:
  xiiTextureCubeAssetProperties_2_3() :
    xiiGraphPatch("xiiTextureCubeAssetProperties", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<xiiString>())
    {
      if (pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::Skybox")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::LookupTable")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<xiiString>() == "xiiTextureCubeUsageEnum::SkyboxHDR")
      {
        pNode->ChangeProperty("Usage", (xiiInt32)xiiTexConvUsage::Hdr);
      }
    }

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
        pNode->AddProperty("CompressionMode", (xiiInt32)xiiTexConvCompressionMode::Medium);
      else
        pNode->AddProperty("CompressionMode", (xiiInt32)xiiTexConvCompressionMode::None);
    }
  }
};

xiiTextureCubeAssetProperties_2_3 g_xiiTextureCubeAssetProperties_2_3;
