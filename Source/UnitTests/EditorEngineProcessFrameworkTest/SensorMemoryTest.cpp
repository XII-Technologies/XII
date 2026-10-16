/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  bool CheckEmptySensorOutput(xiiGALDevice& device, xiiGALTexture* texture, float expected)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(commands != nullptr))
      return false;
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
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
    {
      commands->End();
      return false;
    }
    for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
    {
      const float* row = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride);
      for (xiiUInt32 x = 0; x < capture.m_uiWidth * 4; ++x)
        XII_TEST_FLOAT(row[x], expected, 0.00001f);
    }
    const auto unmapped = commands->UnmapTextureSubresource(capture.m_pStagingTexture, {});
    commands->End();
    return XII_TEST_RESULT(unmapped);
  }

  bool SeedSensorOutput(xiiGALDevice& device, xiiGALTexture* texture)
  {
    const auto&            size = texture->GetDescription().m_Size;
    xiiDynamicArray<float> values;
    values.SetCount(size.width * size.height * 4);
    for (auto& value : values)
      value = 0.75f;
    const xiiUInt32 stride   = size.width * 4 * sizeof(float);
    auto            commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(commands != nullptr))
      return false;
    const auto state = texture->GetResourceState();
    commands->Begin();
    commands->UpdateTexture(texture, {}, xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {size.width, size.height, 1}), xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(values.GetData()), values.GetCount() * sizeof(float)), stride, stride * size.height));
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = texture;
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands->End();
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    return CheckEmptySensorOutput(device, texture, 0.75f);
  }

  bool ConfigureView(xiiGPUTestingEnvironment& environment, xiiWorld& world, xiiCamera& camera, xiiView*& out_view)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Calibrated sensor", out_view);
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    out_view->SetCamera(&camera);
    out_view->SetViewport({0, 0, 16, 16});
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {16, 16};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target             = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(target != nullptr))
      return false;
    out_view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    return true;
  }

  xiiSensorProfileHandle Profile(xiiUInt32 width, xiiUInt32 height)
  {
    xiiSensorProfile profile;
    profile.m_Type                   = xiiSensorType::DepthCamera;
    profile.m_NoiseModel             = xiiSensorNoiseModel::None;
    profile.m_uiResolutionX          = width;
    profile.m_uiResolutionY          = height;
    profile.m_fPrincipalPointXPixels = static_cast<float>(width) * 0.5f;
    profile.m_fPrincipalPointYPixels = static_cast<float>(height) * 0.5f;
    xiiSensorProfileHandle handle;
    XII_TEST_RESULT(xiiSensorRenderingManager::AcquireProfile(profile, handle));
    return handle;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(SensorMemory);
XII_CREATE_SIMPLE_TEST(SensorMemory, ProfileSwitchingAndReadback)
{
  auto* minimumCVar = xiiCVar::FindCVarByName("Graphics.DynamicResolution.MinimumRenderScale");
  auto* maximumCVar = xiiCVar::FindCVarByName("Graphics.DynamicResolution.MaximumRenderScale");
  if (!XII_TEST_BOOL(minimumCVar != nullptr && maximumCVar != nullptr) || !XII_TEST_BOOL(minimumCVar->GetType() == xiiCVarType::Float && maximumCVar->GetType() == xiiCVarType::Float))
    return;
  auto&       minimum    = *static_cast<xiiCVarFloat*>(minimumCVar);
  auto&       maximum    = *static_cast<xiiCVarFloat*>(maximumCVar);
  const float oldMinimum = minimum.GetValue(), oldMaximum = maximum.GetValue();
  minimum = 1.0f;
  maximum = 1.0f;
  XII_SCOPE_EXIT(minimum = oldMinimum; maximum = oldMaximum);
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  if (!XII_TEST_RESULT(xiiLoadPipelineLibraryDescriptor(definition)))
    return;
  const auto          pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Sensor retention default pipeline", std::move(definition));
  xiiWorldDescription description("Sensor retention");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiCamera cameras[2];
  xiiView*  views[2] = {};
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    if (!ConfigureView(environment, world, cameras[i], views[i]))
      return;
    views[i]->SetRenderPipelineResource(pipeline);
    XII_TEST_RESULT(views[i]->SetSensorProfile(Profile(16, 16)));
  }
  world.Update();
  environment.GetDevice()->WaitIdle();
  auto       original    = views[0]->GetSensorOutputTexture();
  const auto independent = views[1]->GetSensorOutputTexture();
  if (!XII_TEST_BOOL(original != nullptr && independent != nullptr && original != independent))
    return;
  XII_TEST_RESULT(views[0]->SetSensorProfile(views[0]->GetSensorProfile()));
  XII_TEST_BOOL(views[0]->GetSensorOutputTexture() == original);
  for (xiiUInt32 i = 0; i < 64; ++i)
  {
    const xiiUInt32 width  = 8 + i;
    const xiiUInt32 height = 8 + i % 5;
    XII_TEST_RESULT(views[0]->SetSensorProfile(Profile(width, height)));
    XII_TEST_BOOL(views[0]->GetSensorOutputTexture() == nullptr);
    world.Update();
    const auto output = views[0]->GetSensorOutputTexture();
    if (!XII_TEST_BOOL(output != nullptr))
      return;
    XII_TEST_INT(output->GetDescription().m_Size.width, width);
    XII_TEST_INT(output->GetDescription().m_Size.height, height);
    XII_TEST_BOOL(output->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float);
    XII_TEST_BOOL(views[1]->GetSensorOutputTexture() == independent && output != independent);
    XII_TEST_INT(original->GetRefCount(), 1);
  }
  environment.GetDevice()->WaitIdle();
  const auto current = views[0]->GetSensorOutputTexture();
  if (!SeedSensorOutput(*environment.GetDevice(), current.Borrow()))
    return;
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(views[0]->GetSensorOutputTexture() == current);
  CheckEmptySensorOutput(*environment.GetDevice(), current.Borrow(), 0.0f);
  XII_TEST_RESULT(views[0]->SetSensorProfile({}));
  XII_TEST_BOOL(views[0]->GetSensorOutputTexture() == nullptr);
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(current->GetRefCount(), 1);
  CheckEmptySensorOutput(*environment.GetDevice(), current.Borrow(), 0.0f);
  XII_TEST_RESULT(views[0]->SetSensorProfile(Profile(16, 16)));
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(views[0]->GetSensorOutputTexture() != nullptr && views[0]->GetSensorOutputTexture() != original);
}

class xiiSensorAllocationTestPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSensorAllocationTestPass, xiiRenderPipelinePass);

public:
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    xiiBuiltinRenderPipelinePass pass;
    xiiVariantDictionary         properties;
    properties.Insert("Stage", "ScreenEffects");
    XII_SUCCEED_OR_RETURN(pass.Configure(properties));
    const auto device = xiiGALDevice::GetDefaultDevice();
    xiiGALDevice::SetDefaultDevice(nullptr);
    XII_SCOPE_EXIT(xiiGALDevice::SetDefaultDevice(device));
    return pass.Build(context);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSensorAllocationTestPass, 1, xiiRTTIDefaultAllocator<xiiSensorAllocationTestPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(SensorMemory, AllocationFailureAndRecovery)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Sensor allocation failure");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiCamera camera;
  xiiView*  view = nullptr;
  if (!ConfigureView(environment, world, camera, view))
    return;
  XII_TEST_RESULT(view->SetSensorProfile(Profile(16, 16)));
  xiiRenderPipelineResourceDescriptor failure;
  failure.AddPass("Sensor allocation failure", "xiiSensorAllocationTestPass");
  const auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Sensor allocation failure", std::move(failure));
  view->SetRenderPipelineResource(pipeline);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log, true);
    log.ExpectMessage("Failed to allocate calibrated sensor output", xiiLogMsgType::ErrorMsg);
    log.ExpectMessage("Failed to build render pipeline", xiiLogMsgType::ErrorMsg);
    world.Update();
  }
  XII_TEST_BOOL(view->GetSensorOutputTexture() == nullptr);
  xiiRenderPipelineResourceDescriptor recovered;
  if (!XII_TEST_RESULT(xiiLoadPipelineLibraryDescriptor(recovered)))
    return;
  const auto restored = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Sensor allocation restored", std::move(recovered));
  view->SetRenderPipelineResource(restored);
  world.Update();
  environment.GetDevice()->WaitIdle();
  const auto output = view->GetSensorOutputTexture();
  if (!XII_TEST_BOOL(output != nullptr))
    return;
  CheckEmptySensorOutput(*environment.GetDevice(), output.Borrow(), 0.0f);
}
