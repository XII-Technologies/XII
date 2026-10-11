/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

namespace
{
  xiiDecalAtlasResourceDescriptor MakeAtlas(xiiUInt8 value = 32, bool mipMaps = true)
  {
    xiiDecalAtlasResourceDescriptor atlas;
    atlas.m_AtlasSize        = {4, 4};
    atlas.m_bGenerateMipMaps = mipMaps;
    for (xiiUInt32 layer = 0; layer < 4; ++layer)
    {
      xiiGALTextureCreationDescription description;
      description.m_Type        = xiiGALResourceDimension::Texture2D;
      description.m_Size        = {4, 4};
      description.m_Format      = layer == 0 ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : xiiGALResourceFormat::RGBA8UNormalized;
      description.m_uiMipLevels = mipMaps ? 3 : 1;
      auto& image               = atlas.m_LayerImages[layer];
      image.ResetAndAlloc(description);
      auto bytes = image.GetBlobPtr<xiiUInt8>();
      for (xiiUInt64 pixel = 0; pixel < bytes.GetCount(); pixel += 4)
      {
        bytes[pixel]     = value + static_cast<xiiUInt8>(layer);
        bytes[pixel + 1] = 96;
        bytes[pixel + 2] = 160;
        bytes[pixel + 3] = 255;
      }
    }
    auto& entry = atlas.m_Entries.ExpandAndGetRef();
    entry.m_sDecalId.Assign("{ 01234567-89AB-CDEF-0123-456789ABCDEF }");
    entry.m_Size            = {2, 2};
    entry.m_vUVRect         = {0.25f, 0.25f, 0.5f, 0.5f};
    entry.m_vTextureMetrics = {0.5f, 0.5f, 4, 4};
    entry.m_ChannelMask     = xiiDecalChannelMask::All;
    return atlas;
  }

  class FailingAtlasWriter : public xiiStreamWriter
  {
    xiiResult WriteBytes(const void*, xiiUInt64) override { return XII_FAILURE; }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, DecalAtlasDescriptor)
{
  auto atlas = MakeAtlas();
  XII_TEST_RESULT(atlas.Validate());
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  if (!XII_TEST_RESULT(atlas.Serialize(writer)))
    return;
  xiiMemoryStreamReader           reader(&storage);
  xiiDecalAtlasResourceDescriptor loaded;
  if (!XII_TEST_RESULT(loaded.Deserialize(reader)))
    return;
  XII_TEST_INT(reader.GetReadPosition(), storage.GetStorageSize64());
  XII_TEST_INT(loaded.m_Entries.GetCount(), 1);
  XII_TEST_BOOL(loaded.m_Entries[0].m_sDecalId == atlas.m_Entries[0].m_sDecalId);
  XII_TEST_VEC4(loaded.m_Entries[0].m_vUVRect, atlas.m_Entries[0].m_vUVRect, 0);
  XII_TEST_VEC4(loaded.m_Entries[0].m_vTextureMetrics, atlas.m_Entries[0].m_vTextureMetrics, 0);
  for (xiiUInt32 layer = 0; layer < 4; ++layer)
  {
    XII_TEST_INT(loaded.m_LayerImages[layer].GetMipLevelCount(), 3);
    XII_TEST_BOOL(static_cast<const xiiImage&>(loaded.m_LayerImages[layer]).GetByteBlobPtr() == static_cast<const xiiImage&>(atlas.m_LayerImages[layer]).GetByteBlobPtr());
  }
  FailingAtlasWriter failing;
  XII_TEST_BOOL(atlas.Serialize(failing).Failed());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Every truncation fails without publishing partial data")
  {
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(storage.GetStorageSize32());
    reader.SetReadPosition(0);
    XII_TEST_INT(reader.ReadBytes(bytes.GetData(), bytes.GetCount()), bytes.GetCount());
    for (xiiUInt32 size = 0; size < bytes.GetCount(); ++size)
    {
      xiiRawMemoryStreamReader truncated(bytes.GetData(), size);
      XII_TEST_BOOL(loaded.Deserialize(truncated).Failed());
      XII_TEST_INT(loaded.m_Entries.GetCount(), 1);
      XII_TEST_INT(loaded.m_LayerImages[0].GetPixelPointer<xiiUInt8>()[0], 32);
    }
    bytes.PeekBack() ^= 1;
    xiiRawMemoryStreamReader corrupt(bytes.GetData(), bytes.GetCount());
    XII_TEST_BOOL(loaded.Deserialize(corrupt).Failed());
    bytes[0] = 1; // Metadata-only atlas version is not a renderable packed atlas.
    xiiRawMemoryStreamReader obsolete(bytes.GetData(), bytes.GetCount());
    XII_TEST_BOOL(loaded.Deserialize(obsolete).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid metadata and channel layouts")
  {
    auto rejects = [&](const xiiDecalAtlasResourceDescriptor& invalid) {
      XII_TEST_BOOL(invalid.Validate().Failed());
      xiiDefaultMemoryStreamStorage rejected;
      xiiMemoryStreamWriter         output(&rejected);
      XII_TEST_BOOL(invalid.Serialize(output).Failed());
      XII_TEST_INT(rejected.GetStorageSize64(), 0);
    };
    auto invalid = MakeAtlas();
    invalid.m_Entries.PushBack(atlas.m_Entries[0]);
    rejects(invalid);
    invalid = MakeAtlas();
    invalid.m_Entries[0].m_sDecalId.Clear();
    rejects(invalid);
    invalid                          = MakeAtlas();
    invalid.m_Entries[0].m_vUVRect.x = 0.75f;
    rejects(invalid);
    invalid                          = MakeAtlas();
    invalid.m_Entries[0].m_vUVRect.z = xiiMath::NaN<float>();
    rejects(invalid);
    invalid                           = MakeAtlas();
    invalid.m_Entries[0].m_Size.width = 0;
    rejects(invalid);
    invalid = MakeAtlas();
    invalid.m_Entries[0].m_ChannelMask.SetValue(0x8000);
    rejects(invalid);
    invalid                    = MakeAtlas();
    invalid.m_bGenerateMipMaps = false;
    rejects(invalid);
    invalid = MakeAtlas();
    invalid.m_LayerImages[1].Clear();
    rejects(invalid);
    invalid = MakeAtlas();
    invalid.m_LayerImages[1].ResetAndCopy(atlas.m_LayerImages[0]);
    rejects(invalid);
    invalid = MakeAtlas();
    invalid.m_LayerImages[0].ResetAndCopy(atlas.m_LayerImages[1]);
    rejects(invalid);
    invalid                  = MakeAtlas();
    auto description         = invalid.m_LayerImages[2].GetDescription();
    description.m_Size.width = 8;
    invalid.m_LayerImages[2].ResetAndAlloc(description);
    rejects(invalid);
    XII_TEST_RESULT(MakeAtlas(0, false).Validate());
    atlas.m_Entries.Clear();
    XII_TEST_RESULT(atlas.Validate()); // A valid empty project atlas still owns neutral channel images.
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DDS allocation is bounded by its enclosing payload")
  {
    xiiDefaultMemoryStreamStorage ddsStorage;
    xiiMemoryStreamWriter         ddsWriter(&ddsStorage);
    xiiDdsFileFormat              dds;
    XII_TEST_RESULT(dds.WriteImage(ddsWriter, atlas.m_LayerImages[0], "dds"));
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(ddsStorage.GetStorageSize32());
    xiiMemoryStreamReader input(&ddsStorage);
    XII_TEST_INT(input.ReadBytes(bytes.GetData(), bytes.GetCount()), bytes.GetCount());
    // DDS width and height agree with the atlas, but the advertised pixel count
    // would exceed a gigabyte. There are only 84 actual pixel bytes in this blob.
    const xiiUInt32 dimension = 16384;
    xiiMemoryUtils::Copy(bytes.GetData() + 12, reinterpret_cast<const xiiUInt8*>(&dimension), sizeof(dimension));
    xiiMemoryUtils::Copy(bytes.GetData() + 16, reinterpret_cast<const xiiUInt8*>(&dimension), sizeof(dimension));
    xiiDefaultMemoryStreamStorage malformed;
    xiiMemoryStreamWriter         output(&malformed);
    output.WriteVersion(2);
    output << xiiSizeU16(16384, 16384) << xiiUInt8(2) << true << xiiUInt32(0);
    output << bytes.GetCount();
    XII_TEST_RESULT(output.WriteBytes(bytes.GetData(), bytes.GetCount()));
    xiiMemoryStreamReader bad(&malformed);
    XII_TEST_BOOL(loaded.Deserialize(bad).Failed());
    XII_TEST_INT(loaded.m_LayerImages[0].GetWidth(), 4);
  }
}

XII_CREATE_SIMPLE_TEST(EditorViews, DecalAtlasResource)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  auto        atlas = MakeAtlas();
  const char* path  = ":imgout/Decals/Native.xiiBinTextureAtlas";
  {
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return;
    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(0x123456789ABCDEF0, 2);
    XII_TEST_RESULT(header.Write(file));
    XII_TEST_RESULT(atlas.Serialize(file));
  }
  auto                        handle = xiiResourceManager::LoadResource<xiiDecalAtlasResource>(path);
  xiiSharedPtr<xiiGALTexture> previous;
  auto                        verify = [&](const xiiDecalAtlasResource& resource, xiiUInt8 value) {
    xiiSharedPtr<xiiGALTexture> textures[] = {resource.GetAlbedoTexture(), resource.GetNormalTexture(), resource.GetMaterialTexture(), resource.GetEmissiveTexture()};
    XII_TEST_INT(resource.GetEntries().GetCount(), 1);
    const xiiDecalAtlasEntry* entry = nullptr;
    XII_TEST_BOOL(resource.TryGetAtlasEntry(atlas.m_Entries[0].m_sDecalId, entry));
    if (XII_TEST_BOOL(entry != nullptr))
      XII_TEST_VEC4(entry->m_vUVRect, atlas.m_Entries[0].m_vUVRect, 0);
    XII_TEST_BOOL(!resource.TryGetAtlasEntry(xiiTempHashedString("absent"), entry));
    XII_TEST_BOOL(entry == nullptr);
    for (xiiUInt32 layer = 0; layer < 4; ++layer)
    {
      XII_TEST_BOOL(!resource.GetDescriptor().m_LayerImages[layer].IsValid());
      if (!XII_TEST_BOOL(textures[layer] != nullptr))
        continue;
      XII_TEST_INT(textures[layer]->GetDescription().m_uiMipLevels, 3);
      xiiImage captured;
      if (!XII_TEST_RESULT(xiiCaptureThumbnail(textures[layer], {4, 4}, captured)))
        continue;
      const auto pixels = captured.GetBlobPtr<xiiUInt8>();
      for (xiiUInt32 pixel = 0; pixel < 16; ++pixel)
      {
        XII_TEST_INT(pixels[pixel * 4], value + layer);
        XII_TEST_INT(pixels[pixel * 4 + 1], 96);
        XII_TEST_INT(pixels[pixel * 4 + 2], 160);
        XII_TEST_INT(pixels[pixel * 4 + 3], 255);
      }
    }
    if (XII_TEST_BOOL(resource.GetGALSampler() != nullptr))
      XII_TEST_INT(resource.GetGALSampler()->GetDescription().m_AddressU.GetValue(), xiiGALTextureAddressMode::Clamp);
  };
  {
    xiiResourceLock<xiiDecalAtlasResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
    verify(*resource.GetPointer(), 32);
    previous = resource->GetAlbedoTexture();
  }
  atlas                                            = MakeAtlas(64);
  xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
  xiiMemoryStreamWriter                     writer(&loader->m_CustomData);
  XII_TEST_RESULT(atlas.Serialize(writer));
  xiiResourceManager::UpdateResourceWithCustomLoader(handle, std::move(loader));
  {
    xiiResourceLock<xiiDecalAtlasResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
    verify(*resource.GetPointer(), 64);
    XII_TEST_BOOL(resource->GetAlbedoTexture() != previous);
  }
  xiiImage captured;
  if (XII_TEST_RESULT(xiiCaptureThumbnail(previous, {4, 4}, captured)))
    XII_TEST_INT(captured.GetPixelPointer<xiiUInt8>()[0], 32); // An in-flight old snapshot retains its pixels.
}
