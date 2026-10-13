/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginAssets/DecalAsset/DecalAtlasBuilder.h>

XII_CREATE_SIMPLE_TEST(EditorViews, DecalAtlasBuilder)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto writeImage = [](xiiStringView path, xiiSizeU32 size, xiiColor color, xiiGALResourceFormat::Enum format) {
    xiiGALTextureCreationDescription description;
    description.m_Type   = xiiGALResourceDimension::Texture2D;
    description.m_Size   = size;
    description.m_Format = xiiGALResourceFormat::RGBA32Float;
    xiiImage image;
    image.ResetAndAlloc(description);
    for (auto& pixel : image.GetBlobPtr<xiiColor>())
      pixel = color;
    XII_TEST_RESULT(image.Convert(format));
    XII_TEST_RESULT(image.SaveTo(path));
  };
  const char* albedo   = ":imgout/DecalBuilder/Albedo.dds";
  const char* mask     = ":imgout/DecalBuilder/Mask.dds";
  const char* normal   = ":imgout/DecalBuilder/Normal.dds";
  const char* material = ":imgout/DecalBuilder/ORM.dds";
  const char* emissive = ":imgout/DecalBuilder/Emissive.dds";
  writeImage(albedo, {8, 4}, xiiColor(0.25f, 0.5f, 0.75f, 0.8f), xiiGALResourceFormat::RGBA8UNormalized);
  writeImage(mask, {2, 2}, xiiColor(0.25f, 0, 0, 1), xiiGALResourceFormat::RGBA8UNormalized);
  writeImage(normal, {4, 4}, xiiColor(0.5f, 0.5f, 0, 1), xiiGALResourceFormat::RG8UNormalized);
  writeImage(material, {4, 4}, xiiColor(0.25f, 0.5f, 0.75f, 1), xiiGALResourceFormat::RGBA8UNormalized);
  writeImage(emissive, {4, 4}, xiiColor(2, 4, 8, 1), xiiGALResourceFormat::RGBA32Float);

  xiiDecalAtlasSource sources[2];
  sources[0].m_sDecalId.Assign("Decal B");
  sources[0].m_sAlbedo     = albedo;
  sources[0].m_sAlphaMask  = mask;
  sources[0].m_sNormal     = normal;
  sources[0].m_sORM        = material;
  sources[0].m_sEmissive   = emissive;
  sources[0].m_ChannelMask = xiiDecalChannelMask::All;
  sources[1].m_sDecalId.Assign("Decal A");
  sources[1].m_sAlbedo = albedo;
  xiiDecalAtlasBuildSettings settings;
  settings.m_uiMaxEntrySize  = 4;
  settings.m_uiMaxSourceSize = 16;
  settings.m_uiMaxAtlasSize  = 64;
  settings.m_uiPadding       = 2;
  xiiDecalAtlasResourceDescriptor atlas;
  const auto                      built = xiiDecalAtlasBuilder::Build(sources, settings, atlas);
  XII_TEST_STATUS(built);
  if (built.Failed())
    return;
  XII_TEST_RESULT(atlas.Validate());
  XII_TEST_INT(atlas.m_Entries.GetCount(), 2);
  XII_TEST_STRING(atlas.m_Entries[0].m_sDecalId.GetString(), "Decal A");
  const auto& entry = atlas.m_Entries[1];
  XII_TEST_INT(entry.m_Size.width, 4);
  XII_TEST_INT(entry.m_Size.height, 2);
  const auto  x    = xiiUInt32(entry.m_vUVRect.x * atlas.m_AtlasSize.width);
  const auto  y    = xiiUInt32(entry.m_vUVRect.y * atlas.m_AtlasSize.height);
  const auto* base = atlas.m_LayerImages[0].GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
  XII_TEST_INT(base[0], 64);
  XII_TEST_INT(base[1], 128);
  XII_TEST_INT(base[2], 191);
  XII_TEST_INT(base[3], 64);
  const auto* padding = atlas.m_LayerImages[0].GetPixelPointer<xiiUInt8>(0, 0, 0, x - 2, y - 2);
  XII_TEST_BOOL(xiiMemoryUtils::IsEqual(base, padding, 4));
  const auto* orm = atlas.m_LayerImages[2].GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
  XII_TEST_INT(orm[0], 128);
  XII_TEST_INT(orm[1], 191);
  XII_TEST_INT(orm[2], 64);
  const auto* n = atlas.m_LayerImages[1].GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
  XII_TEST_FLOAT(n[0], 128, 1);
  XII_TEST_FLOAT(n[1], 128, 1);
  XII_TEST_INT(n[2], 255);
  xiiImage linearEmissive;
  linearEmissive.ResetAndCopy(atlas.m_LayerImages[3]);
  XII_TEST_RESULT(linearEmissive.Convert(xiiGALResourceFormat::RGBA32Float));
  const auto* hdr = linearEmissive.GetPixelPointer<xiiColor>(0, 0, 0, x, y);
  XII_TEST_FLOAT(hdr->r, 2, 0.01f);
  XII_TEST_FLOAT(hdr->g, 4, 0.01f);
  XII_TEST_FLOAT(hdr->b, 8, 0.01f);
  const auto& neutralEntry    = atlas.m_Entries[0];
  const auto  nx              = xiiUInt32(neutralEntry.m_vUVRect.x * atlas.m_AtlasSize.width);
  const auto  ny              = xiiUInt32(neutralEntry.m_vUVRect.y * atlas.m_AtlasSize.height);
  const auto* neutralMaterial = atlas.m_LayerImages[2].GetPixelPointer<xiiUInt8>(0, 0, 0, nx, ny);
  XII_TEST_INT(neutralMaterial[0], 128);
  XII_TEST_INT(neutralMaterial[1], 0);
  XII_TEST_INT(neutralMaterial[2], 255);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Deterministic input order and complete mip chains")
  {
    xiiMath::Swap(sources[0], sources[1]);
    xiiDecalAtlasResourceDescriptor reversed;
    XII_TEST_STATUS(xiiDecalAtlasBuilder::Build(sources, settings, reversed));
    for (xiiUInt32 layer = 0; layer < 4; ++layer)
    {
      XII_TEST_BOOL(static_cast<const xiiImage&>(reversed.m_LayerImages[layer]).GetByteBlobPtr() == static_cast<const xiiImage&>(atlas.m_LayerImages[layer]).GetByteBlobPtr());
      XII_TEST_INT(reversed.m_LayerImages[layer].GetMipLevelCount(), 5); // The two padded 8x6 tiles fit in a 16x16 atlas.
    }
    XII_TEST_VEC4(reversed.m_Entries[1].m_vUVRect, entry.m_vUVRect, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failure preserves the previous output")
  {
    auto reject = [&]() {
      XII_TEST_BOOL(xiiDecalAtlasBuilder::Build(sources, settings, atlas).Failed());
      XII_TEST_INT(atlas.m_Entries.GetCount(), 2);
      XII_TEST_STRING(atlas.m_Entries[0].m_sDecalId.GetString(), "Decal A");
    };
    sources[1].m_sDecalId = sources[0].m_sDecalId;
    reject();
    sources[1].m_sDecalId.Assign("Decal B");
    settings.m_uiMaxAtlasSize = 4;
    reject();
    settings.m_uiMaxAtlasSize  = 64;
    settings.m_uiMaxSourceSize = 4;
    reject();
    settings.m_uiMaxSourceSize = 16;
    sources[1].m_sNormal.Clear();
    reject();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty project and disabled mip generation")
  {
    settings.m_bGenerateMipMaps = false;
    xiiDecalAtlasResourceDescriptor empty;
    XII_TEST_STATUS(xiiDecalAtlasBuilder::Build({}, settings, empty));
    XII_TEST_RESULT(empty.Validate());
    XII_TEST_INT(empty.m_Entries.GetCount(), 0);
    XII_TEST_INT(empty.m_LayerImages[0].GetMipLevelCount(), 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Packed output uploads through the native resource")
  {
    auto                                   handle = xiiResourceManager::CreateResource<xiiDecalAtlasResource>("PackedDecalBuilderAtlas", std::move(atlas));
    xiiResourceLock<xiiDecalAtlasResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
    xiiImage captured;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(resource->GetAlbedoTexture(), {16, 16}, captured)))
    {
      const auto* pixel = captured.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
      XII_TEST_INT(pixel[0], 64);
      XII_TEST_INT(pixel[1], 128);
      XII_TEST_INT(pixel[2], 191);
      XII_TEST_INT(pixel[3], 64);
    }
    XII_TEST_BOOL(resource->GetEmissiveTexture()->GetDescription().m_Format == xiiGALResourceFormat::RGBA16Float);
  }
}
