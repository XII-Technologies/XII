/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

namespace
{
  class MaterialPreviewTestContext : public xiiMaterialContext
  {
  public:
    using xiiMaterialContext::CreateViewContext;
    using xiiMaterialContext::DestroyViewContext;
    using xiiMaterialContext::UpdateThumbnailViewContext;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialPreview)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!XII_TEST_BOOL(xiiLoadPipelineLibraryResource("Editor").IsValid()))
    return;
  const auto       guid = xiiUuid::MakeStableUuidFromString("MaterialPreview.NativeSurface");
  xiiStringBuilder identity;
  xiiConversionUtils::ToString(guid, identity);
  xiiMaterialResourceDescriptor description;
  description.m_BaseColor     = xiiColor(0.18f, 0.18f, 0.18f);
  const auto         material = xiiResourceManager::CreateResource<xiiMaterialResource>(identity, std::move(description));
  xiiPlatformProfile profile;
  profile.AddMissingConfigs();
  MaterialPreviewTestContext document;
  const auto                 initialized = document.Initialize(guid, {}, nullptr, "Material");
  XII_TEST_STATUS(initialized);
  if (initialized.Failed())
    return;
  XII_SCOPE_EXIT(document.Deinitialize());
  auto* context = document.CreateViewContext();
  if (!XII_TEST_BOOL(context != nullptr))
    return;
  XII_SCOPE_EXIT(document.DestroyViewContext(context));
  context->SetRenderPipelineProfile(&profile);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {256, 256};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto output        = device->CreateTexture(target);
  if (!XII_TEST_BOOL(output != nullptr))
    return;
  context->SetupRenderTarget(nullptr, output->GetDefaultView(xiiGALTextureViewType::RenderTarget), 256, 256);
  document.UpdateThumbnailViewContext(context);
  auto* world = document.GetWorld();
  XII_LOCK(world->GetWriteMarker());
  world->SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  if (!XII_TEST_BOOL(world->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(context->GetViewHandle(), view)))
    return;
  xiiUInt32  captureIndex = 0;
  const auto render       = [&](const xiiPreviewLightingSettings& settings, xiiUInt32& pixel) {
    xiiSimpleDocumentConfigMsgToEngine lighting;
    lighting.m_sWhatToDo    = "PreviewLighting";
    lighting.m_PayloadValue = settings.ToVariant();
    document.HandleMessage(&lighting);
    for (xiiUInt32 frame = 0; frame < 4; ++frame)
    {
      context->Redraw(false);
      world->Update();
      device->WaitIdle();
    }
    XII_TEST_RESULT(view->GetLastRenderResult());
    XII_TEST_INT(world->GetObjectCount(), 3);
    if (!xiiGpuTestReadback::ReadTexturePixel(*device, output.Borrow(), 128, 128, pixel))
      return false;
    xiiDynamicArray<xiiColorGammaUB> pixels;
    if (!xiiGpuTestReadback::ReadTexturePixels(*device, output, pixels))
      return false;
    xiiImage image;
    image.ResetAndAlloc(output->GetDescription());
    xiiMemoryUtils::Copy(image.GetPixelPointer<xiiColorGammaUB>(), pixels.GetData(), pixels.GetCount());
    xiiStringBuilder path(":imgout/MaterialPreviewImages");
    path.AppendFormat("/Preview{}.png", captureIndex++);
    return XII_TEST_RESULT(image.SaveTo(path));
  };
  xiiUInt32 neutral = 0, low = 0, brighter = 0;
  auto      settings = xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::NeutralStudio);
  if (!render(settings, neutral))
    return;
  for (xiiUInt32 channel = 0; channel < 3; ++channel)
  {
    const auto value = (neutral >> (channel * 8)) & 255;
    XII_TEST_BOOL_MSG(value >= 60 && value <= 220, "A lit middle-gray material must be visible without clipping (channel %u = %u).", channel, value);
  }
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::LowLightStudio), low))
    return;
  for (xiiUInt32 channel = 0; channel < 3; ++channel)
    XII_TEST_BOOL_MSG(xiiMath::Abs(static_cast<xiiInt32>((neutral >> (channel * 8)) & 255) - static_cast<xiiInt32>((low >> (channel * 8)) & 255)) < 8, "Equivalent physical light/exposure ratios should remain visually comparable.");
  settings.m_fExposureCompensation = 1;
  if (!render(settings, brighter))
    return;
  XII_TEST_BOOL((brighter & 255) > (neutral & 255));
  for (xiiUInt32 model = 0; model < 3; ++model)
  {
    xiiDocumentConfigMsgToEngine scene;
    scene.m_sWhatToDo = "PreviewModel";
    scene.m_iValue    = model;
    document.HandleMessage(&scene);
    xiiUInt32 pixel = 0;
    if (!render(settings, pixel))
      return;
    XII_TEST_BOOL((pixel & 0x00FFFFFFU) != 0);
  }
  xiiUInt32 outdoor = 0;
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::OutdoorDaylight), outdoor))
    return;
  XII_TEST_BOOL((outdoor & 0x00FFFFFFU) != 0);
  view->SetViewRenderMode(xiiViewRenderMode::BaseColor);
  xiiUInt32 diagnostic = 0;
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::Diagnostic), diagnostic))
    return;
  XII_TEST_BOOL((diagnostic & 0x00FFFFFFU) != 0);
}
