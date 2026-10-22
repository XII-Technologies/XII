/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST(EditorViews, TexturePreview)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const xiiUInt8               mip0[] = {32, 96, 160, 128, 32, 96, 160, 128, 32, 96, 160, 128, 32, 96, 160, 128};
  const xiiUInt8               mip1[] = {0, 255, 0, 255};
  xiiGALTextureSubResourceData content[2];
  content[0].m_pData         = xiiConstByteBlobPtr(mip0, sizeof(mip0));
  content[0].m_uiStride      = 8;
  content[0].m_uiDepthStride = sizeof(mip0);
  content[1].m_pData         = xiiConstByteBlobPtr(mip1, sizeof(mip1));
  content[1].m_uiStride      = 4;
  content[1].m_uiDepthStride = sizeof(mip1);
  xiiTexture2DResourceDescriptor texture;
  texture.m_TextureDescription.m_Size        = {2, 2};
  texture.m_TextureDescription.m_uiMipLevels = 2;
  texture.m_TextureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  texture.m_InitialContent                   = content;
  auto hTexture                              = xiiResourceManager::CreateResource<xiiTexture2DResource>("EditorTexturePreviewTest", std::move(texture));

  xiiWorldDescription description("Texture pipeline test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view   = nullptr;
  module->CreateView("Texture preview", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.0001f, 50);
  camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 64, 64});
  xiiGALTextureCreationDescription outputDescription;
  outputDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  outputDescription.m_Size      = {64, 64};
  outputDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  outputDescription.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto output                   = environment.GetDevice()->CreateTexture(outputDescription);
  view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  view->SetRenderPipelineResource(xiiLoadPipelineLibraryResource("TexturePreview"));
  view->SetPipelineParameter("Texture", hTexture.GetResourceID());
  view->SetPipelineParameter("Mip", 0);

  auto CheckCenter = [&](xiiUInt8 red, xiiUInt8 green, xiiUInt8 blue) {
    world.Update();
    xiiImage image;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(output, {64, 64}, image)))
    {
      const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, 32);
      XII_TEST_FLOAT(pixel[0], red, 1);
      XII_TEST_FLOAT(pixel[1], green, 1);
      XII_TEST_FLOAT(pixel[2], blue, 1);
      XII_TEST_INT(pixel[3], 255);
    }
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource-driven channel, alpha, coverage, and mip selection")
  {
    XII_TEST_BOOL(xiiLoadPipelineLibraryResource("TexturePreview") == view->GetRenderPipelineResource());
    view->SetPipelineParameter("Channel", 1);
    CheckCenter(32, 96, 160);
    view->SetPipelineParameter("Channel", 2);
    CheckCenter(32, 32, 32);
    view->SetPipelineParameter("Channel", 3);
    CheckCenter(96, 96, 96);
    view->SetPipelineParameter("Channel", 4);
    CheckCenter(160, 160, 160);
    view->SetPipelineParameter("Channel", 5);
    CheckCenter(128, 128, 128);
    view->SetPipelineParameter("Channel", 6);
    view->SetPipelineParameter("AlphaThreshold", 0.5f);
    CheckCenter(0, 0, 0);
    view->SetPipelineParameter("Channel", 7);
    CheckCenter(255, 255, 255);
    view->SetPipelineParameter("Channel", 1);
    view->SetPipelineParameter("Mip", 1);
    CheckCenter(0, 255, 0);
    view->SetPipelineParameter("Mip", 31); // Clamps to the actual last mip.
    CheckCenter(0, 255, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared definition with independent per-view settings and linear output")
  {
    xiiView* other = nullptr;
    module->CreateView("Independent texture preview", other);
    other->SetCamera(&camera);
    other->SetViewport({0, 0, 64, 64});
    outputDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    auto otherOutput           = environment.GetDevice()->CreateTexture(outputDescription);
    other->SetRenderTargetView(otherOutput->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    other->SetRenderPipelineResource(view->GetRenderPipelineResource());
    other->SetPipelineParameter("Texture", hTexture.GetResourceID());
    other->SetPipelineParameter("Mip", 0);
    other->SetPipelineParameter("Channel", 4);
    CheckCenter(0, 255, 0);
    xiiImage image;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(otherOutput, {64, 64}, image)))
    {
      const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, 32);
      for (xiiUInt32 component = 0; component < 3; ++component)
        XII_TEST_FLOAT(pixel[component], 160, 1);
    }
    module->DestroyView(other->GetHandle());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cube inspection preserves the engine cube face convention")
  {
    const xiiUInt8               faces[6][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 0, 255}, {0, 255, 255, 255}, {255, 0, 255, 255}};
    xiiUInt8                     facePixels[6][64];
    xiiGALTextureSubResourceData faceContent[6];
    for (xiiUInt32 face = 0; face < 6; ++face)
    {
      for (xiiUInt32 pixel = 0; pixel < 16; ++pixel)
        xiiMemoryUtils::Copy(facePixels[face] + pixel * 4, faces[face], 4);
      faceContent[face].m_pData         = xiiConstByteBlobPtr(facePixels[face], 64);
      faceContent[face].m_uiStride      = 16;
      faceContent[face].m_uiDepthStride = 64;
    }
    xiiTextureCubeResourceDescriptor cube;
    cube.m_TextureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    cube.m_TextureDescription.m_Size               = {4, 4};
    cube.m_TextureDescription.m_uiArraySizeOrDepth = 6;
    cube.m_TextureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    cube.m_InitialContent                          = faceContent;
    auto hCube                                     = xiiResourceManager::CreateResource<xiiTextureCubeResource>("EditorCubePreviewTest", std::move(cube));
    view->SetRenderPipelineResource(xiiLoadPipelineLibraryResource("TextureCubePreview"));
    view->SetPipelineParameter("Texture", hCube.GetResourceID());
    view->SetPipelineParameter("Mip", -1);
    view->SetPipelineParameter("Channel", 1);
    const xiiVec3 directions[] = {xiiVec3(1, 0, 0), xiiVec3(-1, 0, 0), xiiVec3(0, 0, 1), xiiVec3(0, 0, -1), xiiVec3(0, -1, 0), xiiVec3(0, 1, 0)};
    for (xiiUInt32 face = 0; face < 6; ++face)
    {
      camera.LookAt(directions[face] * 2.0f, xiiVec3::MakeZero(), face == 2 || face == 3 ? xiiVec3::MakeAxisY() : xiiVec3::MakeAxisZ());
      CheckCenter(faces[face][0], faces[face][1], faces[face][2]);
    }

    // Reload changes the reflected pass configuration while the view keeps the same resource handle.
    auto reloadable = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("EditorTexturePreviewReload");
    auto Publish    = [&](bool cubePreview) {
      xiiRenderPipelineResourceDescriptor descriptor;
      xiiVariantDictionary                properties;
      properties.Insert("Cube", cubePreview);
      descriptor.AddPass("Inspection", xiiGetStaticRTTI<xiiTexturePreviewPass>(), properties);
      xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
      xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
      stream << "EditorTexturePreviewReload.xiiBinRenderPipeline";
      xiiAssetFileHeader header;
      XII_TEST_RESULT(header.Write(stream));
      stream << static_cast<xiiUInt8>(3);
      XII_TEST_RESULT(descriptor.Serialize(stream));
      xiiResourceManager::UpdateResourceWithCustomLoader(reloadable, std::move(loader));
      xiiResourceLock<xiiRenderPipelineResource> resource(reloadable, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(resource && resource->GetPipeline());
    };
    Publish(true);
    view->SetRenderPipelineResource(reloadable);
    CheckCenter(255, 0, 255);
    Publish(false);
    view->SetPipelineParameter("Texture", hTexture.GetResourceID());
    view->SetPipelineParameter("Mip", 0);
    camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    CheckCenter(32, 96, 160);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid settings fail graph construction")
  {
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Synchronize(xiiLoadPipelineLibraryResource("TexturePreview")));
    xiiRenderGraph           graph("Invalid texture preview");
    xiiRenderGraphBlackboard blackboard;
    xiiVariantDictionary     parameters;
    parameters.Insert("Texture", hTexture.GetResourceID());
    parameters.Insert("Channel", 8);
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("Channel", -1);
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("Channel", "not a channel");
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("Channel", 0);
    parameters.Insert("Mip", -2);
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("Mip", 32);
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("Mip", 0);
    parameters.Insert("AlphaThreshold", xiiMath::NaN<float>());
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("AlphaThreshold", 1.5f);
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
    parameters.Insert("AlphaThreshold", 0.5f);
    parameters.Insert("Texture", "");
    XII_TEST_BOOL(instance.Build(view, graph, blackboard, parameters).Failed());
  }
}
