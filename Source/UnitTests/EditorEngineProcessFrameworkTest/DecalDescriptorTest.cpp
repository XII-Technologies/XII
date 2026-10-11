/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

namespace
{
  class FailingDecalWriter : public xiiStreamWriter
  {
    xiiResult WriteBytes(const void*, xiiUInt64) override { return XII_FAILURE; }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, DecalDescriptor)
{
  xiiDecalResourceDescriptor descriptor;
  descriptor.m_sDecalId.Assign("{ 01234567-89AB-CDEF-0123-456789ABCDEF }");
  descriptor.m_hAtlas       = xiiResourceManager::LoadResource<xiiDecalAtlasResource>("{ ProjectDecalAtlas }");
  descriptor.m_hAlbedo      = xiiResourceManager::LoadResource<xiiTexture2DResource>("Decals/BaseColor");
  descriptor.m_hNormal      = xiiResourceManager::LoadResource<xiiTexture2DResource>("Decals/Normal");
  descriptor.m_hMaterial    = xiiResourceManager::LoadResource<xiiTexture2DResource>("Decals/Material");
  descriptor.m_hEmissive    = xiiResourceManager::LoadResource<xiiTexture2DResource>("Decals/Emissive");
  descriptor.m_vUVOffset    = {0.25f, -0.5f};
  descriptor.m_vUVScale     = {-2.0f, 3.0f}; // Tiling and mirroring are meaningful domain settings.
  descriptor.m_Tint         = xiiColor(0.2f, 0.4f, 0.6f, 0.8f);
  descriptor.m_ChannelMask  = xiiDecalChannelMask::All;
  descriptor.m_fOpacity     = 0.75f;
  descriptor.m_fNormalBlend = 0.25f;
  descriptor.m_fRoughness   = 0.3f;
  descriptor.m_fMetallic    = 0.9f;
  descriptor.m_fEmissive    = 8.0f;
  descriptor.m_uiPriority   = 42;
  XII_TEST_RESULT(descriptor.Validate());

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  if (!XII_TEST_RESULT(descriptor.Serialize(writer)))
    return;
  xiiMemoryStreamReader      reader(&storage);
  xiiDecalResourceDescriptor loaded;
  if (!XII_TEST_RESULT(loaded.Deserialize(reader)))
    return;
  XII_TEST_INT(reader.GetReadPosition(), storage.GetStorageSize64());
  XII_TEST_BOOL(loaded.m_sDecalId == descriptor.m_sDecalId);
  XII_TEST_BOOL(loaded.m_hAtlas == descriptor.m_hAtlas);
  XII_TEST_BOOL(loaded.m_hAlbedo == descriptor.m_hAlbedo);
  XII_TEST_BOOL(loaded.m_hNormal == descriptor.m_hNormal);
  XII_TEST_BOOL(loaded.m_hMaterial == descriptor.m_hMaterial);
  XII_TEST_BOOL(loaded.m_hEmissive == descriptor.m_hEmissive);
  XII_TEST_VEC2(loaded.m_vUVOffset, descriptor.m_vUVOffset, 0);
  XII_TEST_VEC2(loaded.m_vUVScale, descriptor.m_vUVScale, 0);
  XII_TEST_BOOL(loaded.m_Tint == descriptor.m_Tint);
  XII_TEST_INT(loaded.m_ChannelMask.GetValue(), descriptor.m_ChannelMask.GetValue());
  XII_TEST_FLOAT(loaded.m_fOpacity, descriptor.m_fOpacity, 0);
  XII_TEST_FLOAT(loaded.m_fNormalBlend, descriptor.m_fNormalBlend, 0);
  XII_TEST_FLOAT(loaded.m_fRoughness, descriptor.m_fRoughness, 0);
  XII_TEST_FLOAT(loaded.m_fMetallic, descriptor.m_fMetallic, 0);
  XII_TEST_FLOAT(loaded.m_fEmissive, descriptor.m_fEmissive, 0);
  XII_TEST_INT(loaded.m_uiPriority, descriptor.m_uiPriority);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Truncation, version and handle types fail transactionally")
  {
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(storage.GetStorageSize32());
    reader.SetReadPosition(0);
    XII_TEST_INT(reader.ReadBytes(bytes.GetData(), bytes.GetCount()), bytes.GetCount());
    for (xiiUInt32 size = 0; size < bytes.GetCount(); ++size)
    {
      xiiRawMemoryStreamReader truncated(bytes.GetData(), size);
      XII_TEST_BOOL(loaded.Deserialize(truncated).Failed());
      XII_TEST_BOOL(loaded.m_sDecalId == descriptor.m_sDecalId);
      XII_TEST_BOOL(loaded.m_hAtlas == descriptor.m_hAtlas);
      XII_TEST_INT(loaded.m_uiPriority, 42);
    }
    bytes[0] = 255;
    xiiRawMemoryStreamReader future(bytes.GetData(), bytes.GetCount());
    XII_TEST_BOOL(loaded.Deserialize(future).Failed());
    bytes[0]                    = 1;
    const float invalidEmissive = xiiMath::NaN<float>();
    xiiMemoryUtils::Copy(bytes.GetData() + bytes.GetCount() - sizeof(xiiUInt8) - sizeof(float),
                         reinterpret_cast<const xiiUInt8*>(&invalidEmissive), sizeof(invalidEmissive));
    xiiRawMemoryStreamReader invalidParameters(bytes.GetData(), bytes.GetCount());
    XII_TEST_BOOL(loaded.Deserialize(invalidParameters).Failed());
    XII_TEST_FLOAT(loaded.m_fEmissive, 8, 0);

    xiiDefaultMemoryStreamStorage badStorage;
    xiiMemoryStreamWriter         badWriter(&badStorage);
    badWriter.WriteVersion(1);
    badWriter << descriptor.m_sDecalId;
    badWriter << descriptor.m_hAlbedo; // Texture handle where an atlas handle is expected.
    xiiMemoryStreamReader badReader(&badStorage);
    XII_TEST_BOOL(loaded.Deserialize(badReader).Failed());
    XII_TEST_BOOL(loaded.m_hAtlas == descriptor.m_hAtlas);

    xiiDefaultMemoryStreamStorage invalidTextStorage;
    xiiMemoryStreamWriter         invalidTextWriter(&invalidTextStorage);
    invalidTextWriter.WriteVersion(1);
    invalidTextWriter << xiiUInt32(0xFFFFFFFF);
    xiiMemoryStreamReader invalidTextReader(&invalidTextStorage);
    XII_TEST_BOOL(loaded.Deserialize(invalidTextReader).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid parameters never serialize")
  {
    const auto rejects = [](const xiiDecalResourceDescriptor& invalid) {
      XII_TEST_BOOL(invalid.Validate().Failed());
      xiiDefaultMemoryStreamStorage output;
      xiiMemoryStreamWriter         stream(&output);
      XII_TEST_BOOL(invalid.Serialize(stream).Failed());
      XII_TEST_INT(output.GetStorageSize64(), 0);
    };
    auto invalid = descriptor;
    invalid.m_sDecalId.Clear();
    rejects(invalid);
    invalid              = descriptor;
    invalid.m_vUVScale.x = 0;
    rejects(invalid);
    invalid          = descriptor;
    invalid.m_Tint.r = xiiMath::NaN<float>();
    rejects(invalid);
    invalid            = descriptor;
    invalid.m_fOpacity = -0.1f;
    rejects(invalid);
    invalid                = descriptor;
    invalid.m_fNormalBlend = 1.1f;
    rejects(invalid);
    invalid              = descriptor;
    invalid.m_fRoughness = xiiMath::NaN<float>();
    rejects(invalid);
    invalid             = descriptor;
    invalid.m_fMetallic = 1.1f;
    rejects(invalid);
    invalid             = descriptor;
    invalid.m_fEmissive = -0.1f;
    rejects(invalid);
    invalid = descriptor;
    invalid.m_ChannelMask.SetValue(0x8000);
    rejects(invalid);
    FailingDecalWriter failing;
    XII_TEST_BOOL(descriptor.Serialize(failing).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Optional source handles and neutral defaults")
  {
    xiiDecalResourceDescriptor neutral;
    neutral.m_sDecalId.Assign("Neutral");
    xiiDefaultMemoryStreamStorage output;
    xiiMemoryStreamWriter         stream(&output);
    XII_TEST_RESULT(neutral.Serialize(stream));
    xiiMemoryStreamReader input(&output);
    XII_TEST_RESULT(loaded.Deserialize(input));
    XII_TEST_BOOL(!loaded.m_hAtlas.IsValid() && !loaded.m_hAlbedo.IsValid() && !loaded.m_hNormal.IsValid());
    XII_TEST_BOOL(!loaded.m_hMaterial.IsValid() && !loaded.m_hEmissive.IsValid());
  }
}
