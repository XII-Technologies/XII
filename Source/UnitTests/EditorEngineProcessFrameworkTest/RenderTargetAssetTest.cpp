/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Utilities/TextureAssetFile.h>

namespace
{
  class FailingTextureWriter : public xiiStreamWriter
  {
    xiiResult WriteBytes(const void*, xiiUInt64) override { return XII_FAILURE; }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, RenderTargetAsset)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {8, 8};
  description.m_Usage     = xiiGALResourceUsage::Mutable;
  description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  xiiGALSamplerCreationDescription sampler;
  sampler.m_MinFilter = xiiGALFilterType::Point;
  sampler.m_MagFilter = xiiGALFilterType::Point;
  sampler.m_MipFilter = xiiGALFilterType::Point;
  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(2345, 8);
  auto write = [&](xiiStringView path) -> xiiResult {
    xiiFileWriter file;
    XII_SUCCEED_OR_RETURN(file.Open(path));
    return xiiTextureAssetFile::WriteRenderTarget(file, description, sampler, header);
  };
  const xiiGALResourceFormat::Enum formats[] = {xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::RGBA8UNormalizedSRGB,
                                                xiiGALResourceFormat::RG11B10Float, xiiGALResourceFormat::RGBA16Float};
  xiiTexture2DResourceHandle       handle;
  xiiSharedPtr<xiiGALTexture>      previousTexture;
  xiiStringBuilder                 path;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(formats); ++i)
  {
    description.m_Format = formats[i];
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(xiiTextureAssetFile::WriteRenderTarget(writer, description, sampler, header));
    xiiMemoryStreamReader                reader(&storage);
    xiiTextureResourceLoader::LoadedData data;
    XII_TEST_RESULT(xiiTextureResourceLoader::LoadTexFile(reader, data));
    XII_TEST_BOOL(!data.m_Image.IsValid());
    XII_TEST_INT(data.m_TextureDescription.CalculateHash(), description.CalculateHash());
    XII_TEST_INT(data.m_SamplerDescription.CalculateHash(), sampler.CalculateHash());
    path.SetFormat(":imgout/TextureAssets/Target{}.xiiBinRenderTarget", i);
    if (!XII_TEST_RESULT(write(path)))
      return;
    handle = xiiResourceManager::LoadResource<xiiTexture2DResource>(path);
    xiiResourceLock<xiiTexture2DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
    auto texture = resource->GetGALTexture();
    if (!XII_TEST_BOOL(texture != nullptr))
      return;
    XII_TEST_INT(texture->GetDescription().m_Format.GetValue(), formats[i]);
    XII_TEST_INT(resource->GetWidth(), 8);
    XII_TEST_INT(resource->GetNumQualityLevelsLoadable(), 0);
    XII_TEST_INT(resource->GetNumQualityLevelsDiscardable(), 0);
    XII_TEST_INT(resource->GetGALSampler()->GetDescription().CalculateHash(), sampler.CalculateHash());
    auto view = texture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    if (!XII_TEST_BOOL(view != nullptr))
      return;
    auto commandList = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commandList->Begin();
    commandList->ClearRenderTargetView(view, xiiColor(0.25f, 0.5f, 0.75f, 1.0f));
    commandList->End();
    auto queue = environment.GetDevice()->GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commandList));
    // Resource state becomes Loaded before the worker publishes memory accounting and closes its stream.
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
    xiiTaskSystem::WaitForCondition([&] { return !xiiResourceManager::IsAnyLoadingInProgress() || xiiTime::Now() > deadline; });
    XII_TEST_BOOL(!xiiResourceManager::IsAnyLoadingInProgress());
    XII_TEST_BOOL(resource->GetMemoryUsage().m_uiMemoryGPU >= 8 * 8 * 4);
    if (i == 0)
    {
      xiiImage capture;
      XII_TEST_RESULT(xiiCaptureThumbnail(texture, {8, 8}, capture));
      for (xiiUInt32 pixel = 0; pixel < 64; ++pixel)
      {
        const auto* rgba = capture.GetPixelPointer<xiiUInt8>() + pixel * 4;
        XII_TEST_FLOAT(rgba[0], 64, 1);
        XII_TEST_FLOAT(rgba[1], 128, 1);
        XII_TEST_FLOAT(rgba[2], 191, 1);
        XII_TEST_INT(rgba[3], 255);
      }
    }
    previousTexture = texture;
  }
  description.m_Size   = {16, 16};
  description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
  sampler.m_AddressU   = xiiGALTextureAddressMode::Mirror;
  FailingTextureWriter failingWriter;
  XII_TEST_BOOL(xiiTextureAssetFile::WriteRenderTarget(failingWriter, description, sampler, header).Failed());
  xiiImage image;
  image.ResetAndAlloc(description);
  XII_TEST_BOOL(xiiTextureAssetFile::WriteImage(failingWriter, image, sampler, header).Failed());
  XII_TEST_RESULT(write(path));
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(handle, true));
  {
    xiiResourceLock<xiiTexture2DResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(resource && resource.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_INT(resource->GetWidth(), 16);
      XII_TEST_INT(resource->GetFormat().GetValue(), description.m_Format.GetValue());
      XII_TEST_BOOL(resource->GetGALTexture() != previousTexture);
      XII_TEST_INT(resource->GetGALSampler()->GetDescription().m_AddressU.GetValue(), xiiGALTextureAddressMode::Mirror);
    }
    XII_TEST_INT(previousTexture->GetDescription().m_Size.width, 8);
  }
  description.m_BindFlags.Remove(xiiGALBindFlags::RenderTarget);
  xiiDefaultMemoryStreamStorage invalidStorage;
  xiiMemoryStreamWriter         invalidWriter(&invalidStorage);
  XII_TEST_BOOL(xiiTextureAssetFile::WriteRenderTarget(invalidWriter, description, sampler, header).Failed());
  XII_TEST_INT(invalidStorage.GetStorageSize64(), 0);
  XII_TEST_RESULT(header.Write(invalidWriter));
  invalidWriter << description;
  invalidWriter << sampler;
  xiiMemoryStreamReader                invalidReader(&invalidStorage);
  xiiTextureResourceLoader::LoadedData invalidData;
  XII_TEST_BOOL(xiiTextureResourceLoader::LoadTexFile(invalidReader, invalidData).Failed());
}
