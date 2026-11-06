/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>

namespace
{
  xiiVec3 CubeDirection(xiiUInt32 x, xiiUInt32 y, xiiUInt32 size, xiiUInt32 face)
  {
    const float   u = (x + 0.5f) * 2.0f / size - 1, v = (y + 0.5f) * 2.0f / size - 1;
    const xiiVec3 directions[] = {{1, -v, -u}, {-1, -v, u}, {u, 1, v}, {u, -1, -v}, {u, -v, 1}, {-u, -v, -1}};
    return directions[face].GetNormalized();
  }

  double GGXLinearEnvironmentCoefficient(double roughness)
  {
    if (roughness == 0)
      return 1;
    const double alpha2 = xiiMath::Pow(roughness, 4.0);
    double       total = 0, moment = 0;
    for (xiiUInt32 i = 0; i < 16384; ++i)
    {
      const double noL         = (i + 0.5) / 16384.0;
      const double noH2        = (1.0 + noL) * 0.5;
      const double denominator = noH2 * (alpha2 - 1.0) + 1.0;
      const double weight      = noL * alpha2 / (denominator * denominator);
      total += weight;
      moment += noL * weight;
    }
    return moment / total;
  }

  bool CheckEnvironmentImage(xiiGALDevice& device, const xiiSharedPtr<xiiGALTexture>& texture, xiiUInt32 pattern, bool diffuse)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    for (xiiUInt32 mip = 0; mip < texture->GetDescription().m_uiMipLevels; ++mip)
      for (xiiUInt32 face = 0; face < 6; ++face)
      {
        commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
        commands->Begin();
        const auto                       queued = readback.Enqueue(commands, {.m_pTexture = texture.Borrow(), .m_uiMipLevel = mip, .m_uiArraySlice = face});
        xiiGALStateTransitionDescription transition;
        transition.m_pResource       = texture.Borrow();
        transition.m_NewState        = xiiGALResourceStateFlags::ShaderResource;
        transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
        commands->End();
        if (!XII_TEST_RESULT(queued))
          return false;
        auto* queue = device.GetCommandQueue();
        queue->WaitForFenceValue(queue->Submit(commands));
        readback.WaitForNextCompleted();
        const auto capture = readback.GetCompleted();
        if (!XII_TEST_BOOL(static_cast<bool>(capture)))
          return false;
        XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
        commands->Begin();
        xiiGALMappedTextureSubresource mapped;
        if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
          return false;
        const float roughness   = float(mip) / xiiMath::Max(texture->GetDescription().m_uiMipLevels - 1, 1U);
        const float coefficient = diffuse ? 2.0f / 3.0f : float(GGXLinearEnvironmentCoefficient(roughness));
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        {
          const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiVec4*>(mapped.m_pData), y * mapped.m_uiStride);
          for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
          {
            const auto direction = CubeDirection(x, y, capture.m_uiWidth, face);
            const auto expected  = pattern == 0 ? xiiVec3(2, 0.5f, 1) : pattern == 1 ? xiiVec3(1e6f, 2e6f, 3e6f) :
                                                                                       xiiVec3(1) + direction * (0.75f * coefficient);
            XII_TEST_BOOL(row[x].IsValid());
            for (xiiUInt32 channel = 0; channel < 3; ++channel)
              XII_TEST_FLOAT(row[x].GetData()[channel], expected.GetData()[channel], pattern == 1 ? 1.0f : 0.025f);
            XII_TEST_FLOAT(row[x].w, 1, 0);
          }
        }
        XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
        commands->End();
      }
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EnvironmentConvolution);
XII_CREATE_SIMPLE_TEST(EnvironmentConvolution, AnalyticCubemapsResourceReuseAndRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  for (xiiUInt32 pattern = 0; pattern < 3; ++pattern)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    constexpr xiiUInt32      size = 17;
    xiiDynamicArray<xiiVec4> pixels;
    pixels.SetCount(size * size * 6);
    xiiGALTextureSubResourceData data[6];
    for (xiiUInt32 face = 0; face < 6; ++face)
    {
      for (xiiUInt32 y = 0; y < size; ++y)
        for (xiiUInt32 x = 0; x < size; ++x)
        {
          const auto value                     = pattern == 0 ? xiiVec3(2, 0.5f, 1) : pattern == 1 ? xiiVec3(1e6f, 2e6f, 3e6f) :
                                                                                                     xiiVec3(1) + CubeDirection(x, y, size, face) * 0.75f;
          pixels[(face * size + y) * size + x] = xiiVec4(value, 1);
        }
      data[face] = xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(pixels.GetData() + face * size * size), size * size * sizeof(xiiVec4)), size * sizeof(xiiVec4), size * size * sizeof(xiiVec4));
    }
    xiiTextureCubeResourceDescriptor descriptor;
    descriptor.m_TextureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    descriptor.m_TextureDescription.m_Size               = {size, size};
    descriptor.m_TextureDescription.m_uiArraySizeOrDepth = 6;
    descriptor.m_TextureDescription.m_uiMipLevels        = 1;
    descriptor.m_TextureDescription.m_Format             = xiiGALResourceFormat::RGBA32Float;
    descriptor.m_TextureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    descriptor.m_InitialContent                          = data;
    xiiStringBuilder name;
    name.SetFormat("Convolution.Pattern.{}", pattern);
    auto                                  resource = xiiResourceManager::CreateResource<xiiTextureCubeResource>(name, std::move(descriptor));
    xiiSharedPtr<const xiiEnvironmentMap> map;
    {
      xiiResourceLock<xiiTextureCubeResource> source(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(source.IsValid()))
        return;
      XII_TEST_STATUS(source->GetEnvironmentMap(map));
      xiiSharedPtr<const xiiEnvironmentMap> repeated;
      XII_TEST_STATUS(source->GetEnvironmentMap(repeated));
      XII_TEST_BOOL(map == repeated);
      XII_TEST_INT(source->GetMemoryUsage().m_uiMemoryGPU, map->m_uiMemoryGPU + source->GetGALTexture()->GetMemoryConsumption());
    }
    if (!XII_TEST_BOOL(map != nullptr))
      return;
    resource.Invalidate();
    xiiResourceManager::FreeAllUnusedResources();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(map->m_Ready->GetCompletedValue() >= 1);
    if (!CheckEnvironmentImage(*environment.GetDevice(), map->m_Specular, pattern, false) || !CheckEnvironmentImage(*environment.GetDevice(), map->m_Irradiance, pattern, true))
      return;
  }
}

XII_CREATE_SIMPLE_TEST(EnvironmentConvolution, ReloadPreservesOriginalLighting)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  xiiTextureCubeResourceHandle          resource;
  xiiSharedPtr<const xiiEnvironmentMap> maps[2];
  for (xiiUInt32 version = 0; version < 2; ++version)
  {
    xiiVec4 pixels[6];
    for (auto& pixel : pixels)
      pixel = version ? xiiVec4(1e6f, 2e6f, 3e6f, 1) : xiiVec4(2, 0.5f, 1, 1);
    xiiGALTextureCreationDescription description;
    description.m_Type               = xiiGALResourceDimension::TextureCube;
    description.m_Size               = {1, 1};
    description.m_uiArraySizeOrDepth = 6;
    description.m_uiMipLevels        = 1;
    description.m_Format             = xiiGALResourceFormat::RGBA32Float;
    xiiImageView image;
    image.ResetAndViewExternalStorage(description, xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(pixels), sizeof(pixels)));
    {
      xiiFileWriter file;
      if (!XII_TEST_RESULT(file.Open(":imgout/ReloadEnvironment.xiiBinTextureCube")) || !XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(file, image, xiiGALGraphicsUtilities::GetDefaultSamplerDescription(), xiiAssetFileHeader())))
        return;
    }
    if (version == 0)
      resource = xiiResourceManager::LoadResource<xiiTextureCubeResource>(":imgout/ReloadEnvironment.xiiBinTextureCube");
    else
      XII_TEST_BOOL(xiiResourceManager::ReloadResource(resource, true));
    xiiResourceLock<xiiTextureCubeResource> source(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(source.IsValid()))
      return;
    XII_TEST_STATUS(source->GetEnvironmentMap(maps[version]));
    if (!XII_TEST_BOOL(maps[version] != nullptr))
      return;
    XII_TEST_INT(maps[version]->m_Ready->GetDescription().m_Type.GetValue(), xiiGALFenceType::General);
  }
  XII_TEST_BOOL(maps[0] != maps[1] && maps[0]->m_Source != maps[1]->m_Source && maps[0]->m_Specular != maps[1]->m_Specular && maps[0]->m_Irradiance != maps[1]->m_Irradiance);
  resource.Invalidate();
  xiiResourceManager::FreeAllUnusedResources();
  environment.GetDevice()->WaitIdle();
  for (xiiUInt32 version = 0; version < 2; ++version)
    if (!CheckEnvironmentImage(*environment.GetDevice(), maps[version]->m_Specular, version, false) || !CheckEnvironmentImage(*environment.GetDevice(), maps[version]->m_Irradiance, version, true))
      return;
}
