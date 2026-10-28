/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/DebugOverlayPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

class xiiVisualizationTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualizationTestExport, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Color", xiiRenderPipelinePin::Texture, true});
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, false});
    pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Texture = builder.ReadTexture(context.GetInputTexture("Color"), xiiGALResourceStateFlags::CopySource);
      builder.ExportTexture(data.m_Texture, xiiGALResourceStateFlags::CopySource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput("Texture", emitted.first->m_Texture);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualizationTestExport, 1, xiiRTTIDefaultAllocator<xiiVisualizationTestExport>)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  bool ReadVisualization(xiiGALDevice& device, xiiGALTexture* texture, xiiColor& color)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto result = readback.Enqueue(commands, {.m_pTexture = texture});
    commands->End();
    if (!XII_TEST_RESULT(result))
      return false;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      return false;
    const auto* pixel = static_cast<const xiiFloat16*>(mapped.m_pData);
    color             = xiiColor(float(pixel[0]), float(pixel[1]), float(pixel[2]), float(pixel[3]));
    XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    commands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineResources, VisualizationChannelSemantics)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  xiiVariantDictionary                properties;
  properties.Insert("Depth", 0.5f);
  properties.Insert("Attributes", xiiColor(0.25f, 0.5f, 0.75f, 1));
  const auto source = definition.AddPass("Known channels", xiiGetStaticRTTI<xiiScreenEffectTestSource>(), properties);
  const auto camera = definition.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
  properties.Clear();
  properties.Insert("UseViewMode", true);
  const auto inspect = definition.AddPass("Visualization", xiiGetStaticRTTI<xiiViewRenderModePass>(), properties);
  const auto output  = definition.AddPass("Readback", xiiGetStaticRTTI<xiiVisualizationTestExport>());
  XII_TEST_RESULT(definition.Connect(source, "Color", inspect, "DisplayLinearColor"));
  XII_TEST_RESULT(definition.Connect(source, "Depth", inspect, "SceneDepth"));
  XII_TEST_RESULT(definition.Connect(camera, "PerFrameCamera", inspect, "PerFrameCamera"));
  for (const auto channel : {"GBufferAlbedo", "GBufferNormal", "GBufferMaterial", "GBufferEmissive", "StableAO", "DilatedVelocityBuffer", "IndirectLighting", "EnvironmentSpecular", "BloomTexture"})
    XII_TEST_RESULT(definition.Connect(source, "Attributes", inspect, channel));
  XII_TEST_RESULT(definition.Connect(inspect, "DisplayLinearColor", output, "Color"));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("VisualizationChannelSemantics", std::move(definition));
  xiiWorldDescription description("Known visualization channels");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* view   = nullptr;
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  module->CreateView("Channel semantics", view);
  view->SetCamera(&viewCamera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  const auto check = [&](xiiViewRenderMode::Enum mode, xiiColor expected) {
    view->SetViewRenderMode(mode);
    world.Update();
    environment.GetDevice()->WaitIdle();
    if (!XII_TEST_RESULT(view->GetLastRenderResult()))
      return;
    const auto* binding = view->GetRenderPipelineInstance().GetOutputBinding(output, "Texture");
    if (!XII_TEST_BOOL(binding != nullptr))
      return;
    auto     texture = view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
    xiiColor actual;
    if (!XII_TEST_BOOL(texture != nullptr) || !ReadVisualization(*environment.GetDevice(), texture.Borrow(), actual))
      return;
    XII_TEST_FLOAT(actual.r, expected.r, 0.001f);
    XII_TEST_FLOAT(actual.g, expected.g, 0.001f);
    XII_TEST_FLOAT(actual.b, expected.b, 0.001f);
    XII_TEST_FLOAT(actual.a, 1.0f, 0.001f);
  };
  for (xiiUInt32 repeat = 0; repeat < 2; ++repeat)
  {
    check(xiiViewRenderMode::BaseColor, {0.25f, 0.5f, 0.75f});
    check(xiiViewRenderMode::Metallic, {0.5f, 0.5f, 0.5f});
    check(xiiViewRenderMode::Roughness, {0.25f, 0.25f, 0.25f});
    check(xiiViewRenderMode::Specular, {0.75f, 0.75f, 0.75f});
    check(xiiViewRenderMode::AmbientOcclusion, {0.25f, 0.25f, 0.25f});
    check(xiiViewRenderMode::NormalMap, {0.146447f, 0.5f, 0.853553f});
    for (const auto mode : {xiiViewRenderMode::Emissive, xiiViewRenderMode::IndirectLighting, xiiViewRenderMode::ReflectionContribution, xiiViewRenderMode::Bloom})
      check(mode, {0.2f, 1.0f / 3.0f, 3.0f / 7.0f});
    check(xiiViewRenderMode::DepthNonLinear, {0.5f, 0.5f, 0.5f});
    check(xiiViewRenderMode::MotionVectors, {1.0f, 0.0f, 0.5f});
    check(xiiViewRenderMode::DepthLinear, {0.000999001f, 0.000999001f, 0.000999001f});
    viewCamera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, -10, 10);
    check(xiiViewRenderMode::DepthLinear, {0.5f, 0.5f, 0.5f});
    check(xiiViewRenderMode::Default, {4, 4, 4});
    viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  }
  module->DestroyView(view->GetHandle());
}

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentVisualizationModes)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  xiiVariantDictionary                properties;
  properties.Insert("Depth", 0.5f);
  properties.Insert("Attributes", xiiColor(0.25f, 0.5f, 0.75f, 1));
  const auto          source = leaf.AddPass("Inputs", xiiGetStaticRTTI<xiiScreenEffectTestSource>(), properties);
  const auto          camera = leaf.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
  xiiSurfaceQueryPass surface;
  surface.m_bOnlyViewWireframe = true;
  XII_TEST_RESULT(surface.GetProperties(properties));
  leaf.AddPass("Inactive wireframe", xiiGetStaticRTTI<xiiSurfaceQueryPass>(), properties);
  xiiViewRenderModePass visualization;
  visualization.m_Mode = xiiViewRenderMode::BaseColor;
  XII_TEST_RESULT(visualization.GetProperties(properties));
  const auto          inspect = leaf.AddPass("Inspect channels", xiiGetStaticRTTI<xiiViewRenderModePass>(), properties);
  xiiDebugOverlayPass overlay;
  overlay.m_bEnabled = false;
  XII_TEST_RESULT(overlay.GetProperties(properties));
  const auto drawing = leaf.AddPass("Optional overlays", xiiGetStaticRTTI<xiiDebugOverlayPass>(), properties);
  const auto output  = leaf.AddPass("Read visualization", xiiGetStaticRTTI<xiiVisualizationTestExport>());
  XII_TEST_RESULT(leaf.Connect(source, "Color", inspect, "DisplayLinearColor"));
  XII_TEST_RESULT(leaf.Connect(source, "Depth", inspect, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(source, "Depth", drawing, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", inspect, "GBufferAlbedo"));
  XII_TEST_RESULT(leaf.Connect(source, "Attributes", inspect, "GBufferMaterial"));
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", inspect, "PerFrameCamera"));
  XII_TEST_RESULT(leaf.Connect(inspect, "DisplayLinearColor", drawing, "DisplayLinearColor"));
  XII_TEST_RESULT(leaf.Connect(drawing, "DisplayLinearColor", output, "Color"));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("Base color", definition.m_Id);
  root.AddSubgraph("Metallic", definition.m_Id);
  const auto metallicId = root.m_Passes[1].m_Id;
  auto&      override   = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[inspect].m_Id);
  visualization.m_Mode = xiiViewRenderMode::Metallic;
  XII_TEST_RESULT(visualization.GetProperties(override.m_Properties));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent visualization", std::move(root));
  xiiWorldDescription worldDescription("Channel visualization only");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view   = nullptr;
  auto*      module = world.GetOrCreateModule<xiiRenderWorldModule>();
  const auto handle = module->CreateView("Explicit channel inputs", view);
  view->SetCamera(&viewCamera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  for (xiiUInt32 frame = 0; frame < 2; ++frame)
  {
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* compiled = instance.GetPipeline();
    if (!XII_TEST_BOOL(compiled != nullptr))
      return;
    XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
    XII_TEST_BOOL(!xiiMaterialManager::IsInitialized());
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized());
    xiiSharedPtr<xiiGALTexture> outputs[2];
    xiiUInt32                   count = 0;
    for (xiiUInt32 index = 0; index < compiled->GetDescriptor().m_Passes.GetCount(); ++index)
    {
      if (!xiiDynamicCast<const xiiVisualizationTestExport*>(instance.GetPass(index)))
        continue;
      ++count;
      const bool  metallic = compiled->GetProvenance()[index].m_InstancePath[0] == metallicId;
      const auto* binding  = instance.GetOutputBinding(index, "Texture");
      if (!XII_TEST_BOOL(binding != nullptr))
        return;
      auto texture = view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
      if (!XII_TEST_BOOL(texture != nullptr))
        return;
      outputs[metallic ? 1 : 0] = texture;
      xiiGALTextureReadback readback(environment.GetDevice());
      auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture});
      commands->End();
      if (!XII_TEST_RESULT(enqueued))
        return;
      auto* queue = environment.GetDevice()->GetCommandQueue();
      queue->WaitForFenceValue(queue->Submit(commands));
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return;
      commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      commands->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        {
          const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiFloat16*>(mapped.m_pData), y * mapped.m_uiStride);
          for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
          {
            XII_TEST_FLOAT(static_cast<float>(row[x * 4]), metallic ? 0.5f : 0.25f, 0.001f);
            XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 1]), 0.5f, 0.001f);
            XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 2]), metallic ? 0.5f : 0.75f, 0.001f);
            XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 3]), 1.0f, 0.001f);
          }
        }
        XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
      }
      commands->End();
      readback.RecycleStagingTexture(capture.m_pStagingTexture);
    }
    XII_TEST_INT(count, 2);
    XII_TEST_BOOL(outputs[0] && outputs[1] && outputs[0] != outputs[1]);
  }
  module->DestroyView(handle);
}
