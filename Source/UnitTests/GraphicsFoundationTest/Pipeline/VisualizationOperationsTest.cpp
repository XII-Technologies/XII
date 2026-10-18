/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
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
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

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
  const auto          source = leaf.AddPass("Inputs", "xiiScreenEffectTestSource", properties);
  const auto          camera = leaf.AddPass("Camera", "xiiViewConstantsPass");
  xiiSurfaceQueryPass surface;
  surface.m_bOnlyViewWireframe = true;
  XII_TEST_RESULT(surface.GetProperties(properties));
  leaf.AddPass("Inactive wireframe", "xiiSurfaceQueryPass", properties);
  xiiViewRenderModePass visualization;
  visualization.m_Mode = xiiViewRenderMode::BaseColor;
  XII_TEST_RESULT(visualization.GetProperties(properties));
  const auto          inspect = leaf.AddPass("Inspect channels", "xiiViewRenderModePass", properties);
  xiiDebugOverlayPass overlay;
  overlay.m_bEnabled = false;
  XII_TEST_RESULT(overlay.GetProperties(properties));
  const auto drawing = leaf.AddPass("Optional overlays", "xiiDebugOverlayPass", properties);
  const auto output  = leaf.AddPass("Read visualization", "xiiVisualizationTestExport");
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
