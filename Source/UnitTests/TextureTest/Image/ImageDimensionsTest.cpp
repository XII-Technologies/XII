/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TextureTest/TextureTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(ImageDimensions);

XII_CREATE_SIMPLE_TEST(ImageDimensions, MirrorOnceSampling)
{
  const xiiInt32  indices[]  = {xiiMath::MinValue<xiiInt32>(), -9, -5, -4, -2, -1, 0, 1, 3, 4, 5, 9, xiiMath::MaxValue<xiiInt32>()};
  const xiiUInt32 expected[] = {3, 3, 3, 3, 1, 0, 0, 1, 3, 3, 3, 3, 3};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(indices); ++i)
  {
    bool border = false;
    XII_TEST_INT(xiiImageUtils::GetSampleIndex(4, indices[i], xiiGALTextureAddressMode::MirrorOnce, border), expected[i]);
    XII_TEST_BOOL(!border);
  }
}

XII_CREATE_SIMPLE_TEST(ImageDimensions, LayoutAndDDS)
{
  const xiiGALResourceDimension::Enum types[] = {xiiGALResourceDimension::Texture1D, xiiGALResourceDimension::Texture1DArray,
                                                 xiiGALResourceDimension::Texture2D, xiiGALResourceDimension::Texture2DArray, xiiGALResourceDimension::Texture3D,
                                                 xiiGALResourceDimension::TextureCube, xiiGALResourceDimension::TextureCubeArray};
  for (auto type : types)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = type;
    description.m_Size               = {4, description.Is1D() ? 1U : 4U};
    description.m_uiArraySizeOrDepth = description.IsCube() ? (type == xiiGALResourceDimension::TextureCubeArray ? 12U : 6U) :
                                                              (description.Is3D() ? 4U : (description.IsArray() ? 3U : 1U));
    description.m_uiMipLevels        = 3;
    description.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    xiiImage image;
    image.ResetAndAlloc(description);
    XII_TEST_INT(image.GetDepth(), description.Is3D() ? 4 : 1);
    XII_TEST_INT(image.GetDepth(1), description.Is3D() ? 2 : 1);
    XII_TEST_INT(image.GetNumFaces(), description.IsCube() ? 6 : 1);
    XII_TEST_INT(image.GetNumArrayIndices(), description.IsCube() ? description.m_uiArraySizeOrDepth / 6U : (description.IsArray() ? description.m_uiArraySizeOrDepth : 1U));
    xiiUInt64 expectedSize = 0;
    for (xiiUInt32 mip = 0; mip < 3; ++mip)
      expectedSize += static_cast<xiiUInt64>(xiiMath::Max(4U >> mip, 1U)) * (description.Is1D() ? 1U : xiiMath::Max(4U >> mip, 1U)) *
        (description.Is3D() ? xiiMath::Max(4U >> mip, 1U) : description.m_uiArraySizeOrDepth) * 4U;
    XII_TEST_INT(image.ComputeDataSize(), expectedSize);
    XII_TEST_INT(image.GetByteBlobPtr().GetCount(), expectedSize);
    xiiHybridArray<xiiGALTextureSubResourceData, 2U> zeroSubresources;
    xiiDynamicArray<xiiUInt8>                        zeroBytes;
    auto                                             zeroData = xiiGALTextureUtilities::GetZeroMemoryInitialData(description, zeroSubresources, zeroBytes);
    XII_TEST_INT(zeroData.m_pSubResources.GetCount(), description.m_uiMipLevels * (description.Is3D() ? 1U : description.m_uiArraySizeOrDepth));
    XII_TEST_INT(zeroBytes.GetCount(), expectedSize);
    for (xiiUInt32 subresource = 0; subresource < zeroData.m_pSubResources.GetCount(); ++subresource)
    {
      const xiiUInt32 mip = subresource % description.m_uiMipLevels;
      XII_TEST_INT(zeroData.m_pSubResources[subresource].m_pData.GetCount(), image.GetDepthPitch(mip) * image.GetDepth(mip));
      XII_TEST_INT(zeroData.m_pSubResources[subresource].m_uiStride, image.GetRowPitch(mip));
      XII_TEST_INT(zeroData.m_pSubResources[subresource].m_uiDepthStride, image.GetDepthPitch(mip));
    }
    for (xiiUInt64 byte = 0; byte < expectedSize; ++byte)
      image.GetByteBlobPtr()[byte] = static_cast<xiiUInt8>(byte);

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiDdsFileFormat              dds;
    if (!XII_TEST_RESULT(dds.WriteImage(writer, image, "dds")))
      continue;
    xiiMemoryStreamReader reader(&storage);
    xiiImage              loaded;
    if (!XII_TEST_RESULT(dds.ReadImage(reader, loaded, "dds")))
      continue;
    XII_TEST_INT(loaded.GetDescription().m_Type.GetValue(), type);
    XII_TEST_INT(loaded.GetDescription().m_uiArraySizeOrDepth, description.m_uiArraySizeOrDepth);
    XII_TEST_INT(loaded.GetMipLevelCount(), 3);
    XII_TEST_BOOL(static_cast<const xiiImage&>(loaded).GetByteBlobPtr() == static_cast<const xiiImage&>(image).GetByteBlobPtr());
    for (xiiUInt32 layer = 0; layer < image.GetNumArrayIndices(); ++layer)
      for (xiiUInt32 face = 0; face < image.GetNumFaces(); ++face)
        for (xiiUInt32 mip = 0; mip < 3; ++mip)
          XII_TEST_BOOL(static_cast<const xiiImage&>(loaded).GetSubImageView(mip, face, layer).GetByteBlobPtr() == static_cast<const xiiImage&>(image).GetSubImageView(mip, face, layer).GetByteBlobPtr());

    auto floatDescription          = description;
    floatDescription.m_uiMipLevels = 1;
    floatDescription.m_Format      = xiiGALResourceFormat::RGBA32Float;
    xiiImage source;
    source.ResetAndAlloc(floatDescription);
    for (xiiUInt32 layer = 0; layer < source.GetNumArrayIndices(); ++layer)
      for (xiiUInt32 face = 0; face < source.GetNumFaces(); ++face)
        for (auto& color : source.GetSubImageView(0, face, layer).GetBlobPtr<xiiColor>())
          color = xiiColor(static_cast<float>(1 + layer * source.GetNumFaces() + face), 0.5f, 0.25f, 1.0f);
    xiiImage mipChain;
    xiiImageUtils::GenerateMipMaps(source, mipChain, {});
    XII_TEST_INT(mipChain.GetMipLevelCount(), 3);
    XII_TEST_INT(mipChain.GetDescription().m_Type.GetValue(), type);
    for (xiiUInt32 layer = 0; layer < mipChain.GetNumArrayIndices(); ++layer)
      for (xiiUInt32 face = 0; face < mipChain.GetNumFaces(); ++face)
        for (xiiUInt32 mip = 0; mip < 3; ++mip)
          for (const auto& color : mipChain.GetSubImageView(mip, face, layer).GetBlobPtr<xiiColor>())
          {
            XII_TEST_FLOAT(color.r, static_cast<float>(1 + layer * mipChain.GetNumFaces() + face), 0.0001f);
            XII_TEST_FLOAT(color.g, 0.5f, 0.0001f);
            XII_TEST_FLOAT(color.b, 0.25f, 0.0001f);
          }
  }
}

XII_CREATE_SIMPLE_TEST(ImageDimensions, CubeAndVolumeConstruction)
{
  xiiGALTextureCreationDescription description;
  description.m_Type   = xiiGALResourceDimension::Texture2D;
  description.m_Size   = {4, 4};
  description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
  xiiImage     faces[6];
  xiiImageView views[6];
  for (xiiUInt32 face = 0; face < 6; ++face)
  {
    faces[face].ResetAndAlloc(description);
    for (auto& byte : faces[face].GetBlobPtr<xiiUInt8>())
      byte = static_cast<xiiUInt8>(face * 32);
    views[face] = faces[face];
  }
  xiiImage cube;
  if (!XII_TEST_RESULT(xiiImageUtils::CreateCubemapFrom6Files(cube, views)))
    return;
  XII_TEST_INT(cube.GetDescription().m_Type.GetValue(), xiiGALResourceDimension::TextureCube);
  XII_TEST_INT(cube.GetNumFaces(), 6);
  XII_TEST_INT(cube.GetDepth(), 1);
  xiiImage scaledCube;
  if (!XII_TEST_RESULT(xiiImageUtils::Scale(cube, scaledCube, 2, 2)))
    return;
  XII_TEST_INT(scaledCube.GetNumFaces(), 6);
  for (xiiUInt32 face = 0; face < 6; ++face)
    for (auto byte : scaledCube.GetSubImageView(0, face).GetBlobPtr<xiiUInt8>())
      XII_TEST_INT(byte, face * 32);

  description.m_Size = {16, 4};
  xiiImage strip;
  strip.ResetAndAlloc(description);
  for (xiiUInt32 y = 0; y < 4; ++y)
    for (xiiUInt32 x = 0; x < 16; ++x)
      for (xiiUInt32 channel = 0; channel < 4; ++channel)
        strip.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y)[channel] = static_cast<xiiUInt8>(x / 4 * 32);
  xiiImage volume;
  if (!XII_TEST_RESULT(xiiImageUtils::CreateVolumeTextureFromSingleFile(volume, strip)))
    return;
  XII_TEST_INT(volume.GetDescription().m_Type.GetValue(), xiiGALResourceDimension::Texture3D);
  XII_TEST_INT(volume.GetDepth(), 4);
  XII_TEST_INT(volume.GetNumArrayIndices(), 1);
  for (xiiUInt32 z = 0; z < 4; ++z)
    for (auto byte : volume.GetSliceView(0, 0, 0, z).GetBlobPtr<xiiUInt8>())
      XII_TEST_INT(byte, z * 32);
  xiiImage scaledVolume;
  XII_TEST_RESULT(xiiImageUtils::Scale3D(volume, scaledVolume, 2, 2, 2));
  XII_TEST_INT(scaledVolume.GetDepth(), 2);
  XII_TEST_INT(scaledVolume.GetNumArrayIndices(), 1);
  XII_TEST_INT(scaledVolume.GetByteBlobPtr().GetCount(), 32);
}
