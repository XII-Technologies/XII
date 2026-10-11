/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <SharedPluginAssets/TextureAsset/TextureAssetProfile.h>
#include <Texture/Converter/TextureConverterEnums.h>

XII_CREATE_SIMPLE_TEST(EditorViews, TextureSamplerProperties)
{
  const char*                          oldNames[]        = {"FixedNearest", "FixedBilinear", "FixedTrilinear", "FixedAnisotropic2x", "FixedAnisotropic4x", "FixedAnisotropic8x", "FixedAnisotropic16x",
                                                            "LowestQuality", "LowQuality", "DefaultQuality", "HighQuality", "HighestQuality"};
  const xiiUInt32                      expected[]        = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 4, 6};
  const char*                          addresses[]       = {"Repeat", "Clamp", "ClampBorder", "Mirror"};
  const xiiGALTextureAddressMode::Enum nativeAddresses[] = {xiiGALTextureAddressMode::Wrap, xiiGALTextureAddressMode::Clamp, xiiGALTextureAddressMode::Border, xiiGALTextureAddressMode::Mirror};
  for (bool cube : {false, true})
    for (bool names : {false, true})
      for (xiiUInt32 preset = 0; preset < XII_ARRAY_SIZE(oldNames); ++preset)
      {
        const char*            type       = cube ? "xiiTextureCubeAssetProperties" : "xiiTextureAssetProperties";
        const xiiUInt32        oldVersion = cube ? 3 : 5;
        xiiAbstractObjectGraph legacy;
        auto*                  node = legacy.AddNode(xiiUuid::MakeUuid(), type, oldVersion, "root");
        xiiStringBuilder       name("xiiTextureFilterSetting::", oldNames[preset]);
        node->AddProperty("TextureFilter", names ? xiiVariant(xiiString(name)) : xiiVariant(preset));
        if (!cube)
        {
          node->AddProperty("CVarResScale", 1.25f);
          name.Set(preset % 2 == 0 ? "xiiTexture2DResolution::CVarRtResolution1" : "xiiTexture2DResolution::CVarRtResolution2");
          node->AddProperty("Resolution", names ? xiiVariant(xiiString(name)) : xiiVariant(xiiInt32(6 + preset % 2)));
          for (const char* axis : {"AddressModeU", "AddressModeV", "AddressModeW"})
          {
            name.Set("xiiImageAddressMode::", addresses[preset % 4]);
            node->AddProperty(axis, names ? xiiVariant(xiiString(name)) : xiiVariant(preset % 4));
          }
        }
        xiiAbstractObjectGraph types;
        auto*                  typeNode = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
        typeNode->AddProperty("TypeName", type);
        typeNode->AddProperty("ParentTypeName", "xiiReflectedClass");
        typeNode->AddProperty("TypeVersion", oldVersion);
        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         writer(&storage);
        xiiAbstractGraphBinarySerializer::Write(writer, &legacy, &types);
        xiiMemoryStreamReader  reader(&storage);
        xiiAbstractObjectGraph patched;
        xiiAbstractObjectGraph patchedTypes;
        xiiAbstractGraphBinarySerializer::Read(reader, &patched, &patchedTypes, true);
        const auto* result = patched.GetNodeByName("root");
        if (!XII_TEST_BOOL(result != nullptr))
          continue;
        XII_TEST_INT(result->GetTypeVersion(), cube ? 4 : 7);
        XII_TEST_BOOL(result->FindProperty("TextureFilter") == nullptr);
        if (XII_TEST_BOOL(result->FindProperty("SamplerPreset") != nullptr))
          XII_TEST_INT(result->FindProperty("SamplerPreset")->m_Value.ConvertTo<xiiUInt32>(), expected[preset]);
        if (!cube)
        {
          XII_TEST_BOOL(result->FindProperty("CVarResScale") == nullptr);
          XII_TEST_FLOAT(result->FindProperty("ResolutionScale")->m_Value.ConvertTo<float>(), 1.25f, 0.0f);
          XII_TEST_INT(result->FindProperty("Resolution")->m_Value.ConvertTo<xiiUInt32>(), 6 + preset % 2);
          for (const char* axis : {"AddressModeU", "AddressModeV", "AddressModeW"})
            XII_TEST_INT(result->FindProperty(axis)->m_Value.ConvertTo<xiiUInt32>(), nativeAddresses[preset % 4]);
        }
      }

  xiiGALSamplerCreationDescription sampler;
  sampler.m_AddressU    = xiiGALTextureAddressMode::MirrorOnce;
  sampler.m_fMipLODBias = 0.75f;
  for (xiiUInt32 preset = 0; preset <= xiiTextureConverterSamplerPreset::Aniso16x; ++preset)
  {
    XII_TEST_RESULT(xiiTextureConverterSamplerPreset::ConfigureSampler(static_cast<xiiTextureConverterSamplerPreset::Enum>(preset), sampler));
    XII_TEST_INT(sampler.m_AddressU.GetValue(), xiiGALTextureAddressMode::MirrorOnce);
    XII_TEST_FLOAT(sampler.m_fMipLODBias, 0.75f, 0.0f);
    XII_TEST_INT(sampler.m_uiMaxAnisotropy, preset >= 3 ? 1U << (preset - 2) : 0U);
    XII_TEST_INT(sampler.m_MinFilter.GetValue(), preset == 0 ? xiiGALFilterType::Point : (preset >= 3 ? xiiGALFilterType::Anisotropic : xiiGALFilterType::Linear));
    XII_TEST_INT(sampler.m_MagFilter.GetValue(), sampler.m_MinFilter.GetValue());
    XII_TEST_INT(sampler.m_MipFilter.GetValue(), preset <= 1 ? xiiGALFilterType::Point : sampler.m_MinFilter.GetValue());
  }
  const auto previousHash = sampler.CalculateHash();
  XII_TEST_BOOL(xiiTextureConverterSamplerPreset::ConfigureSampler(static_cast<xiiTextureConverterSamplerPreset::Enum>(255), sampler).Failed());
  XII_TEST_INT(sampler.CalculateHash(), previousHash);
}

XII_CREATE_SIMPLE_TEST(EditorViews, TextureAssetProfile)
{
  xiiTextureAssetProfileConfig profile;
  xiiUInt32                    resolution = 0;
  for (xiiUInt32 mode = 0; mode < 6; ++mode)
  {
    XII_TEST_RESULT(profile.GetRenderTargetResolution(static_cast<xiiTexture2DResolution::Enum>(mode), 5.0f, resolution));
    XII_TEST_INT(resolution, 64U << mode);
  }
  XII_TEST_RESULT(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution1, 1.0f, resolution));
  XII_TEST_INT(resolution, 256);
  XII_TEST_RESULT(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution2, 0.5f, resolution));
  XII_TEST_INT(resolution, 256);
  XII_TEST_RESULT(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution1, 1.1f, resolution));
  XII_TEST_INT(resolution, 288);
  XII_TEST_RESULT(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution1, 0.001f, resolution));
  XII_TEST_INT(resolution, 8);
  XII_TEST_RESULT(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution2, 1000.0f, resolution));
  XII_TEST_INT(resolution, 4096);
  for (float scale : {0.0f, -1.0f, xiiMath::NaN<float>(), xiiMath::Infinity<float>()})
  {
    XII_TEST_BOOL(profile.GetRenderTargetResolution(xiiTexture2DResolution::ProfileResolution1, scale, resolution).Failed());
    XII_TEST_INT(resolution, 0);
  }
  XII_TEST_BOOL(profile.GetRenderTargetResolution(static_cast<xiiTexture2DResolution::Enum>(99), 1.0f, resolution).Failed());
  const auto defaultHash              = profile.GetTransformHash();
  profile.m_uiRenderTargetResolution1 = 128;
  XII_TEST_BOOL(profile.GetTransformHash() != defaultHash);
  const auto firstHash                = profile.GetTransformHash();
  profile.m_uiRenderTargetResolution2 = 1024;
  XII_TEST_BOOL(profile.GetTransformHash() != firstHash);
  const auto secondHash     = profile.GetTransformHash();
  profile.m_uiMaxResolution = 4096;
  XII_TEST_BOOL(profile.GetTransformHash() != secondHash);

  // Missing new profile properties preserve the former resolution defaults in existing project profiles.
  xiiAbstractObjectGraph legacy;
  auto*                  node = legacy.AddNode(xiiUuid::MakeUuid(), "xiiTextureAssetProfileConfig", 1, "root");
  node->AddProperty("MaxResolution", xiiUInt16(2048));
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiAbstractGraphBinarySerializer::Write(writer, &legacy);
  xiiMemoryStreamReader        reader(&storage);
  xiiTextureAssetProfileConfig loaded;
  xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *xiiGetStaticRTTI<xiiTextureAssetProfileConfig>(), &loaded);
  XII_TEST_INT(loaded.m_uiMaxResolution, 2048);
  XII_TEST_INT(loaded.m_uiRenderTargetResolution1, 256);
  XII_TEST_INT(loaded.m_uiRenderTargetResolution2, 512);
}
