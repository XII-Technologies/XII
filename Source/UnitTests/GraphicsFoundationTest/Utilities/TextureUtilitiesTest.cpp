/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_CREATE_SIMPLE_TEST(Utilities, TextureUtilities)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component mappings and format metadata")
  {
    xiiGALTextureComponentMapping mapping;
    XII_TEST_BOOL(xiiGALTextureUtilities::IsIdentityComponentMapping(mapping));
    mapping.m_R = xiiGALTextureComponentSwizzle::B;
    XII_TEST_BOOL(!xiiGALTextureUtilities::IsIdentityComponentMapping(mapping));

    XII_TEST_INT(xiiGALTextureUtilities::GetComponentCount(xiiGALResourceFormat::RGBA8UNormalized), 4U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBitsPerComponent(xiiGALResourceFormat::RGBA8UNormalized), 8U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBytesPerBlock(xiiGALResourceFormat::RGBA8UNormalized), 4U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBitsPerPixel(xiiGALResourceFormat::RGBA8UNormalized), 32U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBlockWidth(xiiGALResourceFormat::RGBA8UNormalized), 1U);
    XII_TEST_BOOL(!xiiGALTextureUtilities::IsCompressed(xiiGALResourceFormat::RGBA8UNormalized));

    XII_TEST_BOOL(xiiGALTextureUtilities::IsCompressed(xiiGALResourceFormat::BC1UNormalized));
    XII_TEST_INT(xiiGALTextureUtilities::GetBytesPerBlock(xiiGALResourceFormat::BC1UNormalized), 8U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBitsPerPixel(xiiGALResourceFormat::BC1UNormalized), 4U);
    XII_TEST_FLOAT(xiiGALTextureUtilities::GetExactBitsPerPixel(xiiGALResourceFormat::BC1UNormalized), 4.0f, 0.0f);
    XII_TEST_INT(xiiGALTextureUtilities::GetBlockWidth(xiiGALResourceFormat::BC1UNormalized), 4U);
    XII_TEST_INT(xiiGALTextureUtilities::GetBlockHeight(xiiGALResourceFormat::BC1UNormalized), 4U);
    XII_TEST_BOOL(xiiGALTextureUtilities::RequiresFirstLevelBlockAlignment(xiiGALResourceFormat::BC1UNormalized));

    XII_TEST_BOOL(xiiGALResourceFormat::IsMultiplanar(xiiGALResourceFormat::NV12));
    XII_TEST_INT(xiiGALTextureUtilities::GetComponentCount(xiiGALResourceFormat::NV12), 2U);
    XII_TEST_BOOL(xiiGALTextureUtilities::GetPlaneSubFormat(xiiGALResourceFormat::NV12, 0U) == xiiGALResourceFormat::R8UNormalized);
    XII_TEST_BOOL(xiiGALTextureUtilities::GetPlaneSubFormat(xiiGALResourceFormat::NV12, 1U) == xiiGALResourceFormat::RG8UNormalized);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mip dimensions and storage")
  {
    xiiGALTextureCreationDescription description = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    description.m_Size                           = xiiSizeU32(10U, 6U);
    description.m_Format                         = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_uiMipLevels                    = 4U;

    XII_TEST_INT(xiiGALTextureUtilities::GetMipSize(10U, 0U), 10U);
    XII_TEST_INT(xiiGALTextureUtilities::GetMipSize(10U, 1U), 5U);
    XII_TEST_INT(xiiGALTextureUtilities::GetMipSize(10U, 4U), 1U);
    XII_TEST_INT(xiiGALTextureUtilities::GetMipLevelCount(description), 4U);

    const xiiVec3U32 mipSize = xiiGALTextureUtilities::GetMipLevelSize(1U, description);
    XII_TEST_BOOL(mipSize == xiiVec3U32(5U, 3U, 1U));

    const xiiGALMipLevelProperties mip = xiiGALTextureUtilities::GetMipLevelProperties(description, 1U);
    XII_TEST_BOOL(mip.m_LogicalSize == xiiSizeU32(5U, 3U));
    XII_TEST_BOOL(mip.m_StorageSize == xiiSizeU32(5U, 3U));
    XII_TEST_INT(mip.m_uiDepth, 1U);
    XII_TEST_INT(mip.m_uiRowSize, 20U);
    XII_TEST_INT(mip.m_uiDepthSliceSize, 60U);
    XII_TEST_INT(mip.m_uiMipSize, 60U);

    description.m_Format                         = xiiGALResourceFormat::BC1UNormalized;
    const xiiGALMipLevelProperties compressedMip = xiiGALTextureUtilities::GetMipLevelProperties(description, 0U);
    XII_TEST_BOOL(compressedMip.m_LogicalSize == xiiSizeU32(10U, 6U));
    XII_TEST_BOOL(compressedMip.m_StorageSize == xiiSizeU32(12U, 8U));
    XII_TEST_INT(compressedMip.m_uiRowSize, 24U);
    XII_TEST_INT(compressedMip.m_uiDepthSliceSize, 48U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Staging offsets and copy layout")
  {
    xiiGALTextureCreationDescription description = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    description.m_Type                           = xiiGALResourceDimension::Texture2DArray;
    description.m_Size                           = xiiSizeU32(8U, 4U);
    description.m_uiArraySizeOrDepth             = 2U;
    description.m_uiMipLevels                    = 2U;
    description.m_Format                         = xiiGALResourceFormat::RGBA8UNormalized;

    XII_TEST_INT(xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(description, 0U, 0U, 64U), 0U);
    XII_TEST_INT(xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(description, 0U, 1U, 64U), 128U);
    XII_TEST_INT(xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(description, 1U, 0U, 64U), 192U);
    XII_TEST_INT(xiiGALTextureUtilities::GetStagingTextureDataSize(description, 64U), 384U);
    XII_TEST_INT(xiiGALTextureUtilities::GetSubResourceCount(description), 4U);
    XII_TEST_INT(xiiGALTextureUtilities::GetRequiredRowPitch(description, 0U), 32U);
    XII_TEST_INT(xiiGALTextureUtilities::GetRequiredSlicePitch(description, 0U), 128U);

    xiiBoundingBoxU32 region;
    region.m_vMin                                   = xiiVec3U32(0U, 0U, 0U);
    region.m_vMax                                   = xiiVec3U32(5U, 3U, 2U);
    const xiiGALBufferToTextureCopyDescription copy = xiiGALTextureUtilities::GetBufferToTextureCopyDescription(xiiGALResourceFormat::RGBA8UNormalized, region, 16U);
    XII_TEST_INT(copy.m_uiRowSize, 20U);
    XII_TEST_INT(copy.m_uiRowStride, 32U);
    XII_TEST_INT(copy.m_uiRowStrideInTexels, 8U);
    XII_TEST_INT(copy.m_uiRowCount, 3U);
    XII_TEST_INT(copy.m_uiDepthStride, 96U);
    XII_TEST_INT(copy.m_uiMemorySize, 192U);

    const xiiGALBufferToTextureCopyDescription compressedCopy = xiiGALTextureUtilities::GetBufferToTextureCopyDescription(xiiGALResourceFormat::BC1UNormalized, region, 16U);
    XII_TEST_INT(compressedCopy.m_uiRowSize, 16U);
    XII_TEST_INT(compressedCopy.m_uiRowCount, 1U);
    XII_TEST_INT(compressedCopy.m_uiMemorySize, 32U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CPU subresource copies honor row and depth strides")
  {
    xiiUInt8 source[32];
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(source); ++i)
      source[i] = static_cast<xiiUInt8>(i + 1U);

    xiiGALTextureSubResourceData sourceData(xiiConstByteBlobPtr(source), 8U, 16U);
    xiiUInt8                     destination[48] = {};
    xiiGALTextureUtilities::CopyTextureSubresource(sourceData, 2U, 2U, 4U, destination, 12U, 24U);

    XII_TEST_INT(destination[0], 1U);
    XII_TEST_INT(destination[3], 4U);
    XII_TEST_INT(destination[12], 9U);
    XII_TEST_INT(destination[15], 12U);
    XII_TEST_INT(destination[24], 17U);
    XII_TEST_INT(destination[27], 20U);
    XII_TEST_INT(destination[36], 25U);
    XII_TEST_INT(destination[39], 28U);
    XII_TEST_INT(destination[4], 0U);
    XII_TEST_INT(destination[16], 0U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default descriptors and view formats")
  {
    const xiiGALTextureCreationDescription texture1D   = xiiGALTextureUtilities::GetDefaultTexture1DDescription();
    const xiiGALTextureCreationDescription texture2D   = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    const xiiGALTextureCreationDescription texture3D   = xiiGALTextureUtilities::GetDefaultTexture3DDescription();
    const xiiGALTextureCreationDescription textureCube = xiiGALTextureUtilities::GetDefaultTextureCubeDescription();
    XII_TEST_BOOL(texture1D.m_Type == xiiGALResourceDimension::Texture1D);
    XII_TEST_BOOL(texture2D.m_Type == xiiGALResourceDimension::Texture2D);
    XII_TEST_BOOL(texture3D.m_Type == xiiGALResourceDimension::Texture3D);
    XII_TEST_BOOL(textureCube.m_Type == xiiGALResourceDimension::TextureCube);
    XII_TEST_INT(textureCube.m_uiArraySizeOrDepth, 6U);
    XII_TEST_BOOL(texture2D.m_Usage == xiiGALResourceUsage::Immutable);

    XII_TEST_BOOL(xiiGALTextureUtilities::GetDefaultTextureViewFormat(xiiGALResourceFormat::RGBA8Typeless, xiiGALTextureViewType::ShaderResource, xiiGALBindFlags::ShaderResource) == xiiGALResourceFormat::RGBA8UNormalizedSRGB);
    XII_TEST_BOOL(xiiGALTextureUtilities::GetDefaultTextureViewFormat(xiiGALResourceFormat::R32Typeless, xiiGALTextureViewType::DepthStencil, xiiGALBindFlags::DepthStencil) == xiiGALResourceFormat::D32Float);
    XII_TEST_BOOL(xiiGALTextureUtilities::GetDefaultTextureViewFormat(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALTextureViewType::RenderTarget, xiiGALBindFlags::RenderTarget) == xiiGALResourceFormat::RGBA8UNormalizedSRGB);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Zero-memory initial data")
  {
    xiiGALTextureCreationDescription description = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    description.m_Size                           = xiiSizeU32(4U, 2U);
    description.m_Format                         = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_uiMipLevels                    = 2U;

    xiiHybridArray<xiiGALTextureSubResourceData, 2U> subresources;
    xiiDynamicArray<xiiUInt8>                        data;
    const xiiGALTextureData                          initialData = xiiGALTextureUtilities::GetZeroMemoryInitialData(description, subresources, data);

    XII_TEST_INT(subresources.GetCount(), 2U);
    XII_TEST_INT(initialData.m_pSubResources.GetCount(), 2U);
    XII_TEST_INT(data.GetCount(), 40U);
    XII_TEST_INT(subresources[0].m_uiStride, 16U);
    XII_TEST_INT(subresources[0].m_uiDepthStride, 32U);
    XII_TEST_INT(subresources[1].m_uiStride, 8U);
    XII_TEST_INT(subresources[1].m_uiDepthStride, 8U);
    for (xiiUInt8 value : data)
      XII_TEST_INT(value, 0U);
  }
}
