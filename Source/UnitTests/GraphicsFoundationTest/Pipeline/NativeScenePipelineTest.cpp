/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Geometry.h>
#include <Foundation/Math/Float16.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeScenePipeline);
XII_CREATE_SIMPLE_TEST(NativeScenePipeline, DisplayRecordingRetirementAndExposureEpoch)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                  resource = xiiLoadPipelineLibraryResource("HdrDisplay");
  xiiWorldDescription         worldDescription("Display recording owners");
  xiiUniquePtr<xiiWorld>      world = XII_DEFAULT_NEW(xiiWorld, worldDescription);
  xiiRenderGraphContext       contexts[2];
  xiiRenderPipelineInstance   instances[2];
  xiiCamera                   cameras[2];
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  xiiSharedPtr<xiiGALBuffer>  histories[2];
  {
    XII_LOCK(world->GetWriteMarker());
    auto* module = world->GetOrCreateModule<xiiRenderWorldModule>();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      cameras[i].SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      cameras[i].SetExposure(i ? 0.5f : 1.0f);
      module->CreateView("Retired display view", views[i]);
      views[i]->SetCamera(&cameras[i]);
      views[i]->SetViewport({0, 0, 8, 8});
      xiiDisplayOutputSettings settings;
      settings.m_Exposure.m_fAdaptationSpeedBright = 0;
      settings.m_Exposure.m_fAdaptationSpeedDark   = 0;
      settings.m_fBloomStrength                    = 0;
      XII_TEST_RESULT(views[i]->SetDisplayOutputSettings(settings));
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {8, 8};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[i]         = environment.GetDevice()->CreateTexture(target);
      views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      if (!XII_TEST_RESULT(contexts[i].Initialize("Retained display recording")) || !XII_TEST_RESULT(instances[i].Synchronize(resource)))
        return;
    }
  }
  float firstExposure = 0;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    const float luminance = phase == 0 ? 4.0f : phase == 1 ? 400.0f :
                                                             0.2f;
    if (phase == 2)
      for (auto* view : views)
      {
        auto settings              = view->GetDisplayOutputSettings();
        settings.m_Exposure.m_Mode = xiiExposureMode::Manual;
        XII_TEST_RESULT(view->SetDisplayOutputSettings(settings));
      }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiRenderGraphBufferHandle exposures[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& graph = contexts[i].GetGraph();
      auto& cache = contexts[i].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      struct Source
      {
        xiiRenderGraphTextureHandle m_Texture;
        float                       m_Luminance;
      };
      const auto source = graph.AddPass<Source>("Test HDR input", xiiGALCommandQueueFlags::Graphics, [&](Source& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription desc;desc.m_Type=xiiGALResourceDimension::Texture2D;desc.m_Size={8,8};desc.m_Format=xiiGALResourceFormat::RGBA32Float;desc.m_BindFlags=xiiGALBindFlags::RenderTarget|xiiGALBindFlags::ShaderResource;
        data.m_Texture=builder.WriteTexture("HDR fixture",desc,xiiGALResourceStateFlags::CopyDestination);data.m_Luminance=luminance; }, [](const Source& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(data.m_Luminance, data.m_Luminance, data.m_Luminance, 1)); });
      // The control always starts fresh. The subject must stay invalid after an old recording executes.
      if (i == 1)
        instances[i].InvalidateTemporalHistory();
      const xiiRenderPipelineInputBinding input(graph, instances[i].GetPipeline()->FindInterface("HDRSceneColor", true), source.first->m_Texture);
      if (!XII_TEST_RESULT_MSG(instances[i].Build(views[i], graph, contexts[i].GetBlackboard(), {}, xiiMakeArrayPtr(&input, 1)), "%s", instances[i].GetLastBuildError().GetStartPointer()))
        return;
      exposures[i] = instances[i].GetInterfaceBinding(instances[i].GetPipeline()->FindInterface("CurrentExposure", false))->m_hBuffer;
      graph.AddPass<xiiUInt32>("Test exposure export", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) { builder.ExportBuffer(builder.ReadBuffer(exposures[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      if (phase == 0 && i == 0)
        instances[i].InvalidateTemporalHistory();
      if (phase == 2)
        instances[i].Clear();
      graph.EndSetup();
    }
    // Both recordings must survive destruction of their pass objects, views and world.
    if (phase == 2)
      world.Clear();
    float values[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& graph = contexts[i].GetGraph();
      auto& cache = contexts[i].GetResourceCache();
      XII_SCOPE_EXIT(cache.EndFrame());
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[i].GetBlackboard(), &cache)))
        return;
      environment.GetDevice()->WaitIdle();
      const auto exposure = graph.GetExportedBuffer(exposures[i]);
      if (phase == 0)
        histories[i] = exposure;
      XII_TEST_BOOL(exposure == histories[i]);
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), exposure.Borrow(), 0, values[i]))
        return;
      if (phase == 2)
      {
        XII_TEST_FLOAT(values[i], i ? 0.5f : 1.0f, 0.0001f);
        const float x = luminance * values[i], linear = xiiMath::Saturate(x * (2.51f * x + 0.03f) / (x * (2.43f * x + 0.59f) + 0.14f));
        const float encoded = linear < 0.0031308f ? linear * 12.92f : 1.055f * xiiMath::Pow(linear, 1.0f / 2.4f) - 0.055f;
        xiiUInt32   pixel   = 0;
        if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[i].Borrow(), 4, 4, pixel))
          return;
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
          XII_TEST_FLOAT(float((pixel >> (channel * 8)) & 255) / 255, encoded, 0.005f);
      }
    }
    if (phase < 2)
    {
      XII_TEST_FLOAT(values[0], values[1], 0.000001f);
      XII_TEST_BOOL(values[0] > 0 && xiiMath::IsFinite(values[0]));
      if (phase == 0)
        firstExposure = values[0];
      else
        XII_TEST_BOOL(values[0] < firstExposure * 0.02f);
    }
    XII_TEST_BOOL(histories[0] && histories[1] && histories[0] != histories[1]);
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    frame.End();
  }
}

class xiiScenePipelineTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScenePipelineTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input;
  xiiRenderPipelineTextureOutputPin m_Output;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto input = context.GetInputTexture(m_Input);
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) { builder.ExportTexture(builder.ReadTexture(input, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, input);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScenePipelineTestExport, 1, xiiRTTIDefaultAllocator<xiiScenePipelineTestExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class ScenePipelineMemoryLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };
  bool ReloadOpaqueAmbient()
  {
    // This is the exact nested resource ID serialized by DeferredOpaque, not a rooted test alias.
    const auto                          resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("RenderPipelines/WorldDeferredLighting.xiiBinRenderPipeline");
    xiiRenderPipelineResourceDescriptor descriptor;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return false;
      descriptor = loaded->GetAuthoring()->m_Descriptor;
    }
    bool changed = false;
    for (auto& pass : descriptor.m_Passes)
      if (pass.m_sType == "xiiDiffuseSurfaceLightingPass")
      {
        pass.m_Properties.Insert("Scale", 0.5f);
        changed = true;
      }
    if (!XII_TEST_BOOL(changed))
      return false;
    const auto                              closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<ScenePipelineMemoryLoader> loader = XII_DEFAULT_NEW(ScenePipelineMemoryLoader);
    loader->m_Closed                               = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << xiiString(resource.GetResourceID());
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Write(stream)))
      return false;
    stream << xiiUInt8(3);
    if (!XII_TEST_RESULT(descriptor.Serialize(stream)))
      return false;
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
    return XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
  }
} // namespace
XII_CREATE_SIMPLE_TEST(NativeScenePipeline, OpaqueWorldCompositionAndIndependentViews)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("DeferredOpaque");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto scene = root.AddSubgraph("Authored opaque scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
  for (const xiiStringView name : {"HDRSceneColor", "DirectLighting", "AmbientLighting", "ObjectId", "Albedo", "Emissive"})
  {
    const auto sink = root.AddPass(xiiStringBuilder("Capture ", name), "xiiScenePipelineTestExport");
    XII_TEST_RESULT(root.Connect(scene, name, sink, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
  }
  for (const xiiStringView name : {"CurrentExposure", "LightListOverflow", "VisibleMeshletOverflow"})
  {
    const auto sink = root.AddPass(xiiStringBuilder("Capture ", name), "xiiBufferExportPass");
    XII_TEST_RESULT(root.Connect(scene, name, sink, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
  }
  const auto  pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Authored opaque scene captures", std::move(root));
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto                mesh     = xiiResourceManager::CreateResource<xiiMeshResource>("NativeScene.SharedBox", std::move(meshDescription));
  const xiiColor            colors[] = {xiiColor(0.8f, 0.1f, 0.05f), xiiColor(0.05f, 0.2f, 0.8f), xiiColor(0.1f, 0.8f, 0.05f)};
  xiiMaterialResourceHandle materials[3];
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiMaterialResourceDescriptor description;
    description.m_BaseColor     = colors[i];
    description.m_EmissiveColor = xiiColor(0.02f, 0.01f, 0.005f);
    xiiMaterialResourceDescriptor::Parameter specular;
    specular.m_Name.Assign("Specular");
    specular.m_Value = 0.0f;
    description.m_Parameters.PushBack(specular);
    xiiStringBuilder name;
    name.SetFormat("NativeScene.Material{}", i);
    materials[i] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(description));
  }
  struct Scene
  {
    xiiUniquePtr<xiiWorld>      m_World;
    xiiRenderWorldModule*       m_Module = nullptr;
    xiiGameObjectHandle         m_Object;
    xiiComponentHandle          m_Mesh, m_Point, m_Sun;
    xiiCamera                   m_Cameras[2];
    xiiView*                    m_Views[2];
    xiiSharedPtr<xiiGALTexture> m_Targets[2];
    xiiUInt32                   m_Identity = 0;
  } scenes[2];
  const auto& hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NativeScene.HiddenLight");
  const auto  target = [&](Scene& s, xiiUInt32 v, xiiUInt32 width, xiiUInt32 height) {
    xiiGALTextureCreationDescription desc;
    desc.m_Type      = xiiGALResourceDimension::Texture2D;
    desc.m_Size      = {width, height};
    desc.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    desc.m_BindFlags = xiiGALBindFlags::RenderTarget;
    s.m_Targets[v]   = environment.GetDevice()->CreateTexture(desc);
    s.m_Views[v]->SetViewport({0, 0, float(width), float(height)});
    s.m_Views[v]->SetRenderTargetView(s.m_Targets[v]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  };
  const auto createMesh = [&](Scene& s, xiiUInt32 material) {
    xiiGameObjectDescription desc;
    desc.m_bDynamic                   = true;
    xiiGameObject* object             = nullptr;
    s.m_Object                        = s.m_World->CreateObject(desc, object);
    xiiStaticMeshComponent* component = nullptr;
    s.m_Mesh                          = xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, materials[material]);
  };
  for (xiiUInt32 w = 0; w < 2; ++w)
  {
    auto&               s = scenes[w];
    xiiWorldDescription worldDescription("Independent opaque world");
    s.m_World = XII_DEFAULT_NEW(xiiWorld, worldDescription);
    XII_LOCK(s.m_World->GetWriteMarker());
    s.m_World->SetWorldSimulationEnabled(false);
    s.m_Module = s.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    createMesh(s, w);
    xiiGameObject*           object = nullptr;
    xiiGameObjectDescription desc;
    desc.m_LocalPosition = xiiVec3(-2, 0, 0);
    desc.m_Tags.Set(hidden);
    s.m_World->CreateObject(desc, object);
    xiiPointLightComponent* point = nullptr;
    s.m_Point                     = xiiPointLightComponent::CreateComponent(object, point);
    point->SetIntensityUnit(xiiPhotometricUnit::Candela);
    point->SetIntensity(2);
    point->SetRange(10);
    s.m_World->CreateObject({}, object);
    xiiDirectionalLightComponent* sun = nullptr;
    s.m_Sun                           = xiiDirectionalLightComponent::CreateComponent(object, sun);
    sun->SetIntensityUnit(xiiPhotometricUnit::Lux);
    sun->SetIntensity(2);
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      s.m_Cameras[v].SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      s.m_Cameras[v].LookAt({-3, 0, 0}, {0, 0, 0}, {0, 0, 1});
      s.m_Cameras[v].SetExposure(v ? 0.5f : 1.0f);
      s.m_Module->CreateView("Authored opaque view", s.m_Views[v]);
      s.m_Views[v]->SetCamera(&s.m_Cameras[v]);
      s.m_Views[v]->SetRenderPipelineResource(pipeline);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(s.m_Views[v]->SetDisplayOutputSettings(display));
      target(s, v, v ? 48 : 32, 32);
      if (v)
        s.m_Views[v]->m_ExcludeTags.Set(hidden);
    }
  }
  bool      sawResident    = false;
  xiiUInt32 finalPixels[2] = {};
  for (xiiUInt32 phase = 0; phase < 18; ++phase)
  {
    if (phase == 9)
      scenes[0].m_World.Clear();
    if (phase == 15 && !ReloadOpaqueAmbient())
      return;
    for (xiiUInt32 w = 0; w < 2; ++w)
    {
      auto& s = scenes[w];
      if (!s.m_World)
        continue;
      XII_LOCK(s.m_World->GetWriteMarker());
      if (phase == 3 && w == 0)
      {
        xiiStaticMeshComponent* component = nullptr;
        if (!XII_TEST_BOOL(s.m_World->TryGetComponent(s.m_Mesh, component)))
          return;
        component->SetMaterialOverride(0, materials[2]);
      }
      if (phase == 4)
      {
        xiiDirectionalLightComponent* sun = nullptr;
        if (!XII_TEST_BOOL(s.m_World->TryGetComponent(s.m_Sun, sun)))
          return;
        sun->SetIntensity(4);
      }
      if (phase == 6 && w == 0)
        s.m_World->DeleteObjectNow(s.m_Object);
      if (phase == 7 && w == 0)
        createMesh(s, 2);
      if (phase == 8)
      {
        xiiPointLightComponent* point = nullptr;
        if (!XII_TEST_BOOL(s.m_World->TryGetComponent(s.m_Point, point)))
          return;
        point->SetActiveFlag(false);
      }
      if (phase == 9)
        target(s, 0, 80, 48);
      if (phase == 10)
      {
        xiiGameObject* object = nullptr;
        if (!XII_TEST_BOOL(s.m_World->TryGetObject(s.m_Object, object)))
          return;
        object->SetLocalPosition(xiiVec3(0.1f, 0, 0));
      }
      s.m_World->Update();
      environment.GetDevice()->WaitIdle();
      const auto* data = s.m_Module->GetGpuInstanceData();
      if (!XII_TEST_BOOL(data != nullptr))
        return;
      XII_TEST_BOOL(s.m_Module->GetExtractionRequirements() == (xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights));
      const bool absent = phase == 6 && w == 0;
      XII_TEST_INT(data->GetInstanceCount(), absent ? 0 : 1);
      xiiGpuInstanceSource resolved;
      if (absent)
        XII_TEST_BOOL(data->Resolve(s.m_Identity, resolved).Failed());
      else
      {
        const auto identity = data->GetInstances()[0].m_uiObjectId;
        if (phase == 7 && w == 0)
        {
          XII_TEST_BOOL(identity != s.m_Identity);
          XII_TEST_BOOL(data->Resolve(s.m_Identity, resolved).Failed());
        }
        s.m_Identity = identity;
        XII_TEST_RESULT(data->Resolve(identity, resolved));
        XII_TEST_BOOL(resolved.m_hOwnerObject == s.m_Object && resolved.m_hOwnerComponent == s.m_Mesh);
        if (w == 1)
        {
          XII_TEST_FLOAT(data->GetInstances()[0].m_GlobalTransform.GetTranslationVector().x, phase >= 10 ? 0.1f : 0.0f, 0.00001f);
          XII_TEST_FLOAT(data->GetInstances()[0].m_PreviousGlobalTransform.GetTranslationVector().x, phase >= 11 ? 0.1f : 0.0f, 0.00001f);
        }
      }
      if ((phase == 5 && w == 1) || phase == 17)
      {
        XII_TEST_INT(data->GetUploadStatistics().m_uiBytesUploaded, 0);
        XII_TEST_INT(data->GetUploadStatistics().m_uiUploadPasses, 0);
      }
      if (phase == 10)
      {
        XII_TEST_BOOL(data->GetUploadStatistics().m_uiBytesUploaded > 0);
        XII_TEST_BOOL(data->GetUploadStatistics().m_uiBytesUploaded <= sizeof(xiiGpuSceneInstance));
      }
      const bool resident = !absent && xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount > 0;
      sawResident |= resident;
      xiiSharedPtr<xiiGALBuffer> histories[2];
      xiiVec4                    directValues[2];
      for (xiiUInt32 v = 0; v < 2; ++v)
      {
        const auto* view     = s.m_Views[v];
        const auto& instance = view->GetRenderPipelineInstance();
        const auto* graph    = view->GetRenderGraph();
        if (!XII_TEST_BOOL(instance.GetPipeline() != nullptr && graph != nullptr))
          return;
        const auto binding = [&](xiiStringView name) {
          return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        };
        const auto texture = [&](xiiStringView name) {
          return graph->GetExportedTexture(binding(name)->m_hTexture);
        };
        const auto      width = s.m_Targets[v]->GetDescription().m_Size.width, height = s.m_Targets[v]->GetDescription().m_Size.height;
        const xiiUInt32 x = width / 2, y = height / 2;
        xiiUInt32       objectId = 0, albedo = 0, output = 0;
        xiiVec4         hdr, ambient, direct;
        struct Half4
        {
          XII_DECLARE_POD_TYPE();
          xiiFloat16 x, y, z, w;
        } emission;
        if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("ObjectId").Borrow(), x, y, objectId) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Albedo").Borrow(), x, y, albedo) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("HDRSceneColor").Borrow(), x, y, hdr) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("DirectLighting").Borrow(), x, y, direct) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("AmbientLighting").Borrow(), x, y, ambient) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Emissive").Borrow(), x, y, emission) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), s.m_Targets[v].Borrow(), x, y, output))
          return;
        XII_TEST_INT(objectId, resident ? s.m_Identity : 0);
        if (phase == 17)
          finalPixels[v] = output;
        histories[v]   = graph->GetExportedBuffer(binding("CurrentExposure")->m_hBuffer);
        float exposure = 0;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), histories[v].Borrow(), 0, exposure))
          return;
        XII_TEST_FLOAT(exposure, v ? 0.5f : 1.0f, 0.0001f);
        directValues[v]                  = direct;
        const float ambientScale         = phase >= 15 ? 0.5f : 1.0f;
        const float ambientCoefficient[] = {0.03f, 0.035f, 0.04f}, emissionValues[] = {float(emission.x), float(emission.y), float(emission.z)};
        const auto  expectedColor = colors[w == 0 && phase >= 3 ? 2 : w];
        for (xiiUInt32 c = 0; c < 3; ++c)
        {
          const float a = float((albedo >> (8 * c)) & 255) / 255;
          if (resident)
            XII_TEST_FLOAT(a, (&expectedColor.r)[c], 0.005f);
          XII_TEST_FLOAT(ambient.GetData()[c], resident ? a * ambientCoefficient[c] * ambientScale / xiiMath::Pi<float>() : 0, 0.0002f);
          XII_TEST_FLOAT(hdr.GetData()[c], direct.GetData()[c] + ambient.GetData()[c] + emissionValues[c], 0.0001f);
          const float value = xiiMath::Max(hdr.GetData()[c] * exposure, 0.0f), linear = xiiMath::Saturate(value * (2.51f * value + 0.03f) / (value * (2.43f * value + 0.59f) + 0.14f));
          const float encoded = linear < 0.0031308f ? linear * 12.92f : 1.055f * xiiMath::Pow(linear, 1.0f / 2.4f) - 0.055f;
          XII_TEST_FLOAT(float((output >> (8 * c)) & 255) / 255, encoded, 0.01f);
        }
        if (resident)
          XII_TEST_BOOL(direct.x > 0 && direct.y > 0 && direct.z > 0);
        for (const xiiStringView name : {"LightListOverflow", "VisibleMeshletOverflow"})
        {
          xiiUInt32 overflow = 1;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph->GetExportedBuffer(binding(name)->m_hBuffer).Borrow(), 0, overflow))
            return;
          XII_TEST_INT(overflow, 0);
        }
        if (phase == 2)
          xiiTestFramework::Output(xiiTestOutput::Details, "Opaque world %u view %u: %u operations, %u submissions, %u barriers.", w, v, graph->GetStatistics().m_uiCompiledPassCount, graph->GetStatistics().m_uiQueueSubmissionCount, graph->GetStatistics().m_uiTotalBarrierCount);
      }
      XII_TEST_BOOL(histories[0] && histories[1] && histories[0] != histories[1]);
      if (resident && phase < 8)
        XII_TEST_BOOL(directValues[0].x > directValues[1].x * 1.05f);
      if (phase >= 2)
        XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount, 1);
    }
    if (phase >= 2 && phase < 6)
    {
      xiiGpuInstanceSource source;
      XII_TEST_BOOL(scenes[0].m_Module->GetGpuInstanceData()->Resolve(scenes[1].m_Identity, source).Failed());
    }
  }
  // Exercise the unmodified application-facing resource too, without capture operations.
  auto& remaining = scenes[1];
  {
    XII_LOCK(remaining.m_World->GetWriteMarker());
    for (auto* view : remaining.m_Views)
      view->SetRenderPipelineResource(asset);
    remaining.m_World->Update();
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      const auto& size  = remaining.m_Targets[v]->GetDescription().m_Size;
      xiiUInt32   pixel = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), remaining.m_Targets[v].Borrow(), size.width / 2, size.height / 2, pixel))
        return;
      for (xiiUInt32 c = 0; c < 3; ++c)
        XII_TEST_FLOAT(float((pixel >> (8 * c)) & 255), float((finalPixels[v] >> (8 * c)) & 255), 1.0f);
      const auto& statistics = remaining.m_Views[v]->GetRenderGraph()->GetStatistics();
      xiiTestFramework::Output(xiiTestOutput::Details, "Unmodified DeferredOpaque view %u: %u operations, %u submissions, %u barriers.", v, statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount);
    }
  }
  XII_TEST_BOOL(sawResident);
}
