/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <Texture/Converter/TextureConverterEnums.h>

namespace
{
  class TextureSamplerPatch : public xiiGraphPatch
  {
  public:
    TextureSamplerPatch(const char* szType, xiiUInt32 uiVersion, bool bAddressModes) :
      xiiGraphPatch(szType, uiVersion), m_bAddressModes(bAddressModes) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* pNode) const override
    {
      // These are persisted values of the removed authoring enum, not runtime sampler values.
      const char*                                  names[]   = {"FixedNearest", "FixedBilinear", "FixedTrilinear", "FixedAnisotropic2x", "FixedAnisotropic4x", "FixedAnisotropic8x", "FixedAnisotropic16x",
                                                                "LowestQuality", "LowQuality", "DefaultQuality", "HighQuality", "HighestQuality"};
      const xiiTextureConverterSamplerPreset::Enum presets[] = {
        xiiTextureConverterSamplerPreset::Nearest, xiiTextureConverterSamplerPreset::Linear, xiiTextureConverterSamplerPreset::Trilinear,
        xiiTextureConverterSamplerPreset::Aniso2x, xiiTextureConverterSamplerPreset::Aniso4x, xiiTextureConverterSamplerPreset::Aniso8x, xiiTextureConverterSamplerPreset::Aniso16x,
        xiiTextureConverterSamplerPreset::Nearest, xiiTextureConverterSamplerPreset::Linear, xiiTextureConverterSamplerPreset::Trilinear,
        xiiTextureConverterSamplerPreset::Aniso4x, xiiTextureConverterSamplerPreset::Aniso16x};
      if (auto* property = pNode->FindProperty("TextureFilter"))
      {
        xiiInt64 index = -1;
        if (property->m_Value.IsA<xiiString>())
        {
          xiiStringView value = property->m_Value.Get<xiiString>();
          value.TrimWordStart("xiiTextureFilterSetting::");
          for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(names); ++i)
            if (value == names[i])
              index = i;
        }
        else if (property->m_Value.CanConvertTo<xiiInt64>())
          index = property->m_Value.ConvertTo<xiiInt64>();
        if (index >= 0 && index < XII_ARRAY_SIZE(presets))
          property->m_Value = static_cast<xiiInt32>(presets[index]);
        pNode->RenameProperty("TextureFilter", "SamplerPreset");
      }
      if (!m_bAddressModes)
        return;

      const char*                          addresses[]   = {"Repeat", "Clamp", "ClampBorder", "Mirror"};
      const xiiGALTextureAddressMode::Enum nativeModes[] = {xiiGALTextureAddressMode::Wrap, xiiGALTextureAddressMode::Clamp, xiiGALTextureAddressMode::Border, xiiGALTextureAddressMode::Mirror};
      for (const char* name : {"AddressModeU", "AddressModeV", "AddressModeW"})
      {
        auto* property = pNode->FindProperty(name);
        if (property == nullptr)
          continue;
        xiiInt64 index = -1;
        if (property->m_Value.IsA<xiiString>())
        {
          xiiStringView value = property->m_Value.Get<xiiString>();
          value.TrimWordStart("xiiImageAddressMode::");
          for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(addresses); ++i)
            if (value == addresses[i])
              index = i;
        }
        else if (property->m_Value.CanConvertTo<xiiInt64>())
          index = property->m_Value.ConvertTo<xiiInt64>();
        if (index >= 0 && index < XII_ARRAY_SIZE(nativeModes))
          property->m_Value = static_cast<xiiInt32>(nativeModes[index]);
      }
    }

  private:
    bool m_bAddressModes;
  };

  TextureSamplerPatch s_TexturePatch("xiiTextureAssetProperties", 6, true);
  TextureSamplerPatch s_CubePatch("xiiTextureCubeAssetProperties", 4, false);
} // namespace
