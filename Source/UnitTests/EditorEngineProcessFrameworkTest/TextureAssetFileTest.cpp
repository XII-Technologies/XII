/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Utilities/TextureAssetFile.h>

XII_CREATE_SIMPLE_TEST(EditorViews, TextureAssetFile)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(123, 2);
  xiiGALSamplerCreationDescription sampler;
  sampler.m_AddressU  = xiiGALTextureAddressMode::Mirror;
  sampler.m_AddressV  = xiiGALTextureAddressMode::Border;
  sampler.m_AddressW  = xiiGALTextureAddressMode::Clamp;
  sampler.m_MipFilter = xiiGALFilterType::Point;
  xiiStringBuilder inputFile;
  for (xiiUInt32 kind = 0; kind < 5; ++kind)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = kind == 0 ? xiiGALResourceDimension::Texture2D : (kind == 2 ? xiiGALResourceDimension::TextureCube : xiiGALResourceDimension::Texture3D);
    description.m_Size               = {4, 4};
    description.m_uiArraySizeOrDepth = kind == 0 || kind == 4 ? 1 : (kind == 2 ? 6 : 4);
    description.m_Format             = kind == 3 ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : xiiGALResourceFormat::RGBA8UNormalized;
    description.m_uiMipLevels        = 3;
    xiiImage image;
    image.ResetAndAlloc(description);
    auto pixels = image.GetBlobPtr<xiiUInt8>();
    for (xiiUInt64 i = 0; i < pixels.GetCount(); i += 4)
    {
      pixels[i]     = static_cast<xiiUInt8>(32 + i / 4);
      pixels[i + 1] = 96;
      pixels[i + 2] = 160;
      pixels[i + 3] = 255;
    }
    if (kind == 0)
    {
      if (!XII_TEST_RESULT(image.SaveTo(":imgout/TextureAssetSource.dds")) ||
          !XII_TEST_RESULT(xiiFileSystem::ResolvePath(":imgout/TextureAssetSource.dds", &inputFile, nullptr)))
        return;
    }
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(writer, image, sampler, header));
    xiiMemoryStreamReader                reader(&storage);
    xiiTextureResourceLoader::LoadedData loaded;
    XII_TEST_RESULT(xiiTextureResourceLoader::LoadTexFile(reader, loaded));
    XII_TEST_INT(loaded.m_TextureDescription.m_Size.width, 0);
    XII_TEST_INT(loaded.m_Image.GetImageFormat().GetValue(), description.m_Format.GetValue());
    XII_TEST_INT(loaded.m_Image.GetDepth(), image.GetDepth());
    XII_TEST_INT(loaded.m_Image.GetNumFaces(), image.GetNumFaces());
    XII_TEST_INT(loaded.m_SamplerDescription.CalculateHash(), sampler.CalculateHash());
    XII_TEST_BOOL(static_cast<const xiiImage&>(loaded.m_Image).GetByteBlobPtr() == static_cast<const xiiImage&>(image).GetByteBlobPtr());
    const char* paths[] = {":imgout/TextureAssets/Flat.xiiBinTexture2D", ":imgout/TextureAssets/Volume.xiiBinTexture3D",
                           ":imgout/TextureAssets/Cube.xiiBinTextureCube", ":imgout/TextureAssets/Lookup.xiiBinLUT", ":imgout/TextureAssets/FlatVolume.xiiBinTexture3D"};
    {
      xiiFileWriter file;
      if (!XII_TEST_RESULT(file.Open(paths[kind])))
        return;
      XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(file, image, sampler, header));
    }
    auto verify = [&](xiiGALTexture* texture, xiiGALSampler* gpuSampler) {
      if (!XII_TEST_BOOL(texture != nullptr && gpuSampler != nullptr))
        return;
      XII_TEST_INT(texture->GetDescription().m_Type.GetValue(), description.m_Type.GetValue());
      XII_TEST_INT(texture->GetDescription().m_uiArraySizeOrDepth, description.m_uiArraySizeOrDepth);
      XII_TEST_INT(texture->GetDescription().m_Format.GetValue(), description.m_Format.GetValue());
      XII_TEST_INT(texture->GetDescription().m_uiMipLevels, description.m_uiMipLevels);
      XII_TEST_INT(gpuSampler->GetDescription().CalculateHash(), sampler.CalculateHash());
    };
    if (kind == 0)
    {
      const auto                            handle = xiiResourceManager::LoadResource<xiiTexture2DResource>(paths[kind]);
      xiiResourceLock<xiiTexture2DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      {
        verify(resource->GetGALTexture().Borrow(), resource->GetGALSampler().Borrow());
        xiiImage captured;
        XII_TEST_RESULT(xiiCaptureThumbnail(resource->GetGALTexture().Borrow(), {4, 4}, captured));
        XII_TEST_BOOL(static_cast<const xiiImage&>(captured).GetByteBlobPtr() == static_cast<const xiiImage&>(image).GetSubImageView().GetByteBlobPtr());
      }
    }
    else if (kind == 2)
    {
      const auto                              handle = xiiResourceManager::LoadResource<xiiTextureCubeResource>(paths[kind]);
      xiiResourceLock<xiiTextureCubeResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
        verify(resource->GetGALTexture().Borrow(), resource->GetGALSampler().Borrow());
    }
    else
    {
      const auto                            handle = xiiResourceManager::LoadResource<xiiTexture3DResource>(paths[kind]);
      xiiResourceLock<xiiTexture3DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
        verify(resource->GetGALTexture().Borrow(), resource->GetGALSampler().Borrow());
    }
  }
  xiiDefaultMemoryStreamStorage emptyStorage;
  xiiMemoryStreamWriter         emptyWriter(&emptyStorage);
  xiiImage                      empty;
  XII_TEST_BOOL(xiiTextureAssetFile::WriteImage(emptyWriter, empty, sampler, header).Failed());
  XII_TEST_INT(emptyStorage.GetStorageSize64(), 0);

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Actual converter output uses native envelopes and sampler presets")
  {
    const char* presets[] = {"Nearest", "Linear", "Trilinear", "Aniso2x", "Aniso4x", "Aniso8x", "Aniso16x", "Lowest", "Low", "Default", "High", "Highest"};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(presets); ++i)
    {
      xiiStringBuilder output;
      if (!XII_TEST_RESULT(xiiFileSystem::ResolvePath(":imgout/ConvertedTexture.xiiBinTexture2D", &output, nullptr)))
        return;
      xiiProcessOptions options;
      xiiStringBuilder  executable(xiiOSFile::GetApplicationDirectory(), "/xiiTextureConverter");
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
      executable.Append(".exe");
#  endif
      options.m_sProcess = executable;
      options.AddArgument("-in0");
      options.AddArgument(inputFile);
      options.AddArgument("-out");
      options.AddArgument(output);
      options.AddCommandLine("-type 2D -compression None -mipmaps None -usage Linear -minRes 4 -maxRes 4 -rgba in0 -addressU Mirror -addressV ClampBorder -assetHashLow 0x12345678 -assetHashHigh 0x1 -assetVersion 2");
      options.AddArgument("-filter");
      options.AddArgument(presets[i]);
      xiiStringBuilder childOutput;
      options.m_onStdOut = [&](xiiStringView text) {
        childOutput.Append(text);
      };
      options.m_onStdError = options.m_onStdOut;
      xiiProcess process;
      if (!XII_TEST_RESULT(process.Launch(options)) || !XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
        return;
      if (!XII_TEST_INT(process.GetExitCode(), 0))
      {
        XII_TEST_FAILURE("TextureConverter failed: {}", childOutput);
        return;
      }
      xiiFileReader file;
      if (!XII_TEST_RESULT(file.Open(output)))
        return;
      xiiTextureResourceLoader::LoadedData result;
      if (!XII_TEST_RESULT(xiiTextureResourceLoader::LoadTexFile(file, result)))
        return;
      const auto& nativeSampler = result.m_SamplerDescription;
      const bool  nearest       = i == 0 || i == 7;
      const bool  anisotropic   = (i >= 3 && i <= 6) || i >= 10;
      XII_TEST_INT(nativeSampler.m_AddressU.GetValue(), xiiGALTextureAddressMode::Mirror);
      XII_TEST_INT(nativeSampler.m_AddressV.GetValue(), xiiGALTextureAddressMode::Border);
      XII_TEST_INT(nativeSampler.m_MinFilter.GetValue(), nearest ? xiiGALFilterType::Point : (anisotropic ? xiiGALFilterType::Anisotropic : xiiGALFilterType::Linear));
      XII_TEST_INT(nativeSampler.m_MagFilter.GetValue(), nativeSampler.m_MinFilter.GetValue());
      XII_TEST_INT(nativeSampler.m_MipFilter.GetValue(), nearest || i == 1 || i == 8 ? xiiGALFilterType::Point : (anisotropic ? xiiGALFilterType::Anisotropic : xiiGALFilterType::Linear));
      XII_TEST_INT(nativeSampler.m_uiMaxAnisotropy, anisotropic ? (i <= 6 ? 1U << (i - 2) : (i == 10 ? 4U : 16U)) : 0U);
      XII_TEST_INT(result.m_Image.GetWidth(), 4);
      XII_TEST_INT(result.m_Image.GetPixelPointer<xiiUInt8>()[0], 32);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Custom filtering survives the converter, resource loader and GPU sampler")
  {
    xiiStringBuilder executable(xiiOSFile::GetApplicationDirectory(), "/xiiTextureConverter");
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    executable.Append(".exe");
#  endif
    xiiUInt32 combination = 0;
    auto      convert     = [&](xiiStringView extra, bool success, xiiStringBuilder& output, xiiStringView secondInput = {}) -> bool {
      xiiStringBuilder path;
      path.SetFormat(":imgout/CustomSampler{}.xiiBinTexture2D", combination++);
      if (!XII_TEST_RESULT(xiiFileSystem::ResolvePath(path, &output, nullptr)))
        return false;
      xiiProcessOptions options;
      options.m_sProcess = executable;
      options.AddArgument("-in0");
      options.AddArgument(inputFile);
      if (!secondInput.IsEmpty())
      {
        options.AddArgument("-in1");
        options.AddArgument(secondInput);
      }
      options.AddArgument("-out");
      options.AddArgument(output);
      options.AddCommandLine("-type 2D -compression None -mipmaps Linear -usage Linear -minRes 4 -maxRes 4 -rgba in0 -addressU Mirror -addressV Clamp -addressW MirrorOnce -assetHashLow 0x12345678 -assetHashHigh 0x1 -assetVersion 2");
      options.AddCommandLine(extra);
      xiiStringBuilder childOutput;
      options.m_onStdOut = [&](xiiStringView text) {
        childOutput.Append(text);
      };
      options.m_onStdError = options.m_onStdOut;
      xiiProcess process;
      if (!XII_TEST_RESULT(process.Launch(options)) || !XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
        return false;
      if (!XII_TEST_BOOL_MSG((process.GetExitCode() == 0) == success, "%s: %s", xiiString(extra).GetData(), childOutput.GetData()))
        return false;
      if (!success)
        XII_TEST_BOOL(!xiiOSFile::ExistsFile(output));
      return true;
    };
    for (xiiUInt32 minFilter : {1U, 2U})
      for (xiiUInt32 magFilter : {1U, 2U})
        for (xiiUInt32 mipMode = 0; mipMode < 3; ++mipMode)
        {
          const xiiUInt32  mipFilter = mipMode == 2 ? 2 : 1;
          const float      maxLOD    = mipMode == 0 ? 0 : xiiMath::MaxValue<float>();
          xiiStringBuilder extra, output;
          extra.SetFormat("-minFilter {} -magFilter {} -mipFilter {} -maxAnisotropy 0 -mipLODBias -0.375 -maxLOD {}", minFilter, magFilter, mipFilter, mipMode == 0 ? "0" : "3.40282347e38");
          if (!convert(extra, true, output))
            return;
          const auto                            handle = xiiResourceManager::LoadResource<xiiTexture2DResource>(output);
          xiiResourceLock<xiiTexture2DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
          if (!XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
            return;
          const auto& sampler = resource->GetGALSampler()->GetDescription();
          XII_TEST_INT(sampler.m_MinFilter.GetValue(), minFilter);
          XII_TEST_INT(sampler.m_MagFilter.GetValue(), magFilter);
          XII_TEST_INT(sampler.m_MipFilter.GetValue(), mipFilter);
          XII_TEST_INT(sampler.m_AddressW.GetValue(), xiiGALTextureAddressMode::MirrorOnce);
          XII_TEST_FLOAT(sampler.m_fMipLODBias, -0.375f, 0);
          XII_TEST_FLOAT(sampler.m_fMaxLOD, maxLOD, 0);
          XII_TEST_INT(resource->GetGALTexture()->GetDescription().m_uiMipLevels, 3);
        }
    xiiStringBuilder output;
    if (convert("-minFilter 3 -magFilter 3 -mipFilter 3 -maxAnisotropy 8 -minLOD 1 -maxLOD 2", true, output))
    {
      const auto                            handle = xiiResourceManager::LoadResource<xiiTexture2DResource>(output);
      xiiResourceLock<xiiTexture2DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      {
        const auto& sampler = resource->GetGALSampler()->GetDescription();
        XII_TEST_INT(sampler.m_uiMaxAnisotropy, 8);
        XII_TEST_FLOAT(sampler.m_fMinLOD, 1, 0);
        XII_TEST_FLOAT(sampler.m_fMaxLOD, 2, 0);
      }
    }
    // Importing separate scalar maps must retain independent input dependencies and use
    // neutral white for absent factors. Color conversion must not alter linear mask values.
    xiiGALTextureCreationDescription maskDescription;
    maskDescription.m_Type   = xiiGALResourceDimension::Texture2D;
    maskDescription.m_Size   = {4, 4};
    maskDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    xiiImage mask;
    mask.ResetAndAlloc(maskDescription);
    for (auto& value : mask.GetBlobPtr<xiiUInt8>())
      value = 192;
    xiiStringBuilder maskPath;
    if (!XII_TEST_RESULT(mask.SaveTo(":imgout/MetallicMask.dds")) ||
        !XII_TEST_RESULT(xiiFileSystem::ResolvePath(":imgout/MetallicMask.dds", &maskPath, nullptr)))
      return;
    for (xiiUInt32 mapping = 0; mapping < 3; ++mapping)
    {
      xiiStringBuilder extra;
      extra.SetFormat("-r white -g {} -b {}", mapping == 2 ? "white" : "in0.r", mapping == 1 ? "white" : "in1.r");
      if (!convert(extra, true, output, maskPath))
        return;
      xiiFileReader file;
      if (!XII_TEST_RESULT(file.Open(output)))
        return;
      xiiTextureResourceLoader::LoadedData data;
      if (!XII_TEST_RESULT(xiiTextureResourceLoader::LoadTexFile(file, data)))
        return;
      const auto* pixel = data.m_Image.GetPixelPointer<xiiUInt8>();
      XII_TEST_INT(pixel[0], 255);
      XII_TEST_INT(pixel[1], mapping == 2 ? 255 : 32);
      XII_TEST_INT(pixel[2], mapping == 1 ? 255 : 192);
    }
    for (const char* invalid : {"-minFilter 3", "-minFilter 0", "-magFilter 257", "-mipFilter 4", "-maxAnisotropy 2",
                                "-minLOD -1", "-minLOD 3 -maxLOD 2", "-maxLOD 1e99", "-minLOD 1garbage", "-magFilter junk", "-maxAnisotropy -1"})
      if (!convert(invalid, false, output))
        return;
  }
#endif
}
