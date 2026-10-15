/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  bool CheckFallbackDepth(xiiGALDevice& device, xiiGALTexture* texture)
  {
    xiiGALTextureReadback readback(&device);
    for (xiiUInt32 slice = 0; slice < texture->GetDescription().m_uiArraySizeOrDepth; ++slice)
    {
      auto commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      if (!XII_TEST_BOOL(commands != nullptr))
        return false;
      commands->Begin();
      const auto result = readback.Enqueue(commands, {.m_pTexture = texture, .m_uiArraySlice = slice});
      commands->End();
      if (!XII_TEST_RESULT(result))
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
      XII_TEST_FLOAT(*static_cast<const float*>(mapped.m_pData), 0.0f, 0.0f);
      const auto unmapped = commands->UnmapTextureSubresource(capture.m_pStagingTexture, {});
      commands->End();
      if (!XII_TEST_RESULT(unmapped))
        return false;
    }
    return true;
  }
} // namespace

class xiiShadowMemoryProbePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowMemoryProbePass, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, false});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Directional;
      xiiRenderGraphTextureHandle m_Local;
    };
    context.m_Graph.AddPass<Data>("Export shadow allocations", xiiGALCommandQueueFlags::Graphics, [](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Directional = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
      data.m_Local = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data.m_Directional, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data.m_Local, xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowMemoryProbePass, 1, xiiRTTIDefaultAllocator<xiiShadowMemoryProbePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiShadowAllocationFailurePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowAllocationFailurePass, xiiRenderPipelinePass);

public:
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    xiiBuiltinRenderPipelinePass shadows;
    xiiVariantDictionary         properties;
    properties.Insert("Stage", "Shadows");
    XII_SUCCEED_OR_RETURN(shadows.Configure(properties));
    const auto device = xiiGALDevice::GetDefaultDevice();
    xiiGALDevice::SetDefaultDevice(nullptr);
    XII_SCOPE_EXIT(xiiGALDevice::SetDefaultDevice(device));
    return shadows.Build(context);
  }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowAllocationFailurePass, 1, xiiRTTIDefaultAllocator<xiiShadowAllocationFailurePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST_GROUP(ShadowMemory);

XII_CREATE_SIMPLE_TEST(ShadowMemory, AtlasesFollowShadowDemand)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription worldDescription("Shadow allocation demand");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Shadow allocation demand", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 16, 16});
  xiiGALTextureCreationDescription targetDescription;
  targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  targetDescription.m_Size      = {16, 16};
  targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
  if (!XII_TEST_BOOL(target != nullptr))
    return;
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  xiiRenderPipelineResourceDescriptor descriptor;
  xiiRenderPipelineResource::GetDefaultDescriptor(descriptor);
  const xiiUInt32 last  = descriptor.m_Passes.GetCount() - 1;
  const xiiUInt32 probe = descriptor.AddPass("Shadow allocations", "xiiShadowMemoryProbePass");
  if (!XII_TEST_RESULT(descriptor.Connect(last, "After", probe, "Before")))
    return;
  const auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Shadow allocation demand", std::move(descriptor));
  view->SetRenderPipelineResource(pipeline);
  auto texture = [&](xiiStringView key) {
    auto* graph = view->GetRenderGraph();
    return graph->GetExportedTexture(graph->FindTexture(key));
  };
  auto check = [&](xiiUInt32 directionalSize, xiiUInt32 localSize) {
    world.Update();
    environment.GetDevice()->WaitIdle();
    auto directional = texture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas);
    auto local       = texture(xiiRGBlackboardKeys::k_LocalShadowAtlas);
    if (!XII_TEST_BOOL(directional != nullptr && local != nullptr))
      return false;
    return XII_TEST_INT(directional->GetDescription().m_Size.width, directionalSize) &&
      XII_TEST_INT(directional->GetDescription().m_Size.height, directionalSize) &&
      XII_TEST_INT(directional->GetDescription().m_uiArraySizeOrDepth, 4) &&
      XII_TEST_INT(local->GetDescription().m_Size.width, localSize) &&
      XII_TEST_INT(local->GetDescription().m_Size.height, localSize);
  };
  if (!check(1, 1))
    return;
  auto fallbackDirectional = texture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas);
  auto fallbackLocal       = texture(xiiRGBlackboardKeys::k_LocalShadowAtlas);
  // Seed nonzero depth to prove the subsequent frame actually clears every fallback slice.
  auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!XII_TEST_BOOL(commands != nullptr))
    return;
  commands->Begin();
  commands->ClearDepthStencilView(fallbackDirectional->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.75f, 0U);
  commands->ClearDepthStencilView(fallbackLocal->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.75f, 0U);
  commands->End();
  environment.GetDevice()->GetCommandQueue()->WaitForFenceValue(environment.GetDevice()->GetCommandQueue()->Submit(commands));
  if (!check(1, 1))
    return;
  XII_TEST_BOOL(texture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas) == fallbackDirectional);
  XII_TEST_BOOL(texture(xiiRGBlackboardKeys::k_LocalShadowAtlas) == fallbackLocal);
  if (!CheckFallbackDepth(*environment.GetDevice(), fallbackDirectional.Borrow()) || !CheckFallbackDepth(*environment.GetDevice(), fallbackLocal.Borrow()))
    return;

  xiiGameObject* object = nullptr;
  world.CreateObject({}, object);
  xiiDirectionalLightComponent* directionalLight = nullptr;
  xiiDirectionalLightComponent::CreateComponent(object, directionalLight);
  directionalLight->SetCastShadows(false);
  if (!check(1, 1))
    return;
  directionalLight->SetCastShadows(true);
  if (!check(4096, 1))
    return;
  auto fullDirectional = texture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas);
  XII_TEST_INT(fallbackDirectional->GetRefCount(), 1);
  directionalLight->SetCastShadows(false);
  if (!check(1, 1))
    return;
  XII_TEST_INT(fullDirectional->GetRefCount(), 1);

  world.CreateObject({}, object);
  xiiPointLightComponent* pointLight = nullptr;
  xiiPointLightComponent::CreateComponent(object, pointLight);
  pointLight->SetCastShadows(false);
  if (!check(1, 1))
    return;
  pointLight->SetCastShadows(true);
  if (!check(1, 4096))
    return;
  auto fullLocal = texture(xiiRGBlackboardKeys::k_LocalShadowAtlas);
  XII_TEST_INT(fallbackLocal->GetRefCount(), 1);
  pointLight->SetCastShadows(false);
  if (!check(1, 1))
    return;
  XII_TEST_INT(fullLocal->GetRefCount(), 1);
}

XII_CREATE_SIMPLE_TEST(ShadowMemory, AllocationFailureAndRecovery)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription worldDescription("Shadow allocation failure");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Shadow allocation failure", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 16, 16});
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Shadow allocation failure", "xiiShadowAllocationFailurePass");
  const auto failure = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Shadow allocation failure", std::move(descriptor));
  view->SetRenderPipelineResource(failure);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log, true);
    log.ExpectMessage("Failed to allocate 'Directional Shadow Atlas'", xiiLogMsgType::ErrorMsg);
    log.ExpectMessage("Failed to build render pipeline", xiiLogMsgType::ErrorMsg);
    world.Update();
  }
  XII_TEST_BOOL(!view->GetRenderGraph()->FindTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas).IsValid());
  xiiGALTextureCreationDescription targetDescription;
  targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  targetDescription.m_Size      = {16, 16};
  targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
  if (!XII_TEST_BOOL(target != nullptr))
    return;
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  xiiRenderPipelineResource::GetDefaultDescriptor(descriptor);
  const auto restored = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Shadow allocation restored", std::move(descriptor));
  view->SetRenderPipelineResource(restored);
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(view->GetRenderGraph()->FindTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas).IsValid());
  XII_TEST_BOOL(view->GetRenderGraph()->FindTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas).IsValid());
}
