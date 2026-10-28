/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneTemporal);
static void TestSceneTemporal(bool extendedFormats)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, extendedFormats)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  auto instrumented = loaded->GetAuthoring()->m_Descriptor;
  if (!extendedFormats)
    for (auto& pass : instrumented.m_Passes)
      if (pass.m_pType && pass.m_pType->GetTypeName() == xiiStringView("xiiAmbientOcclusionPass"))
        pass.m_Properties.Insert("Enabled", false);
  const auto cameraId = xiiFindPipelineTestPass(instrumented, "xiiTemporalCameraPass");
  const auto taaId    = xiiFindPipelineTestPass(instrumented, "xiiTemporalAntiAliasingPass");
  instrumented.m_Interface.PushBack({"RawColor", false, xiiUuid::MakeStableUuidFromString("XII.Scene/Atmosphere"), "HDRSceneColor"});
  const auto                      source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Temporal.InstrumentedScene", std::move(instrumented));
  const auto                      scene  = xiiMakeReloadablePipelineTestResource(source, "Temporal.ReloadableScene");
  xiiRenderPipelineResourceHandle pipelines[3];
  for (xiiUInt32 variant = 0; variant < 3; ++variant)
  {
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, scene.GetResourceID()});
    const auto node   = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    auto&      enable = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    enable.m_Path.PushBack(cameraId);
    enable.m_Properties.Insert("Enabled", variant != 0);
    if (variant == 2)
    {
      auto& raw = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      raw.m_Path.PushBack(taaId);
      raw.m_Properties.Insert("BaseBlendAlpha", 1.0f);
    }
    for (const xiiStringView name : {"RawColor", "HDRSceneColor", "Velocity", "SceneDepth", "ObjectId"})
    {
      const auto capture = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, name, capture, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[capture].m_Id, "Output"});
    }
    pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant == 0 ? "Temporal.Disabled" : variant == 1 ? "Temporal.Enabled" :
                                                                                                                                           "Temporal.Current",
                                                                                       std::move(root));
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescriptor;
  meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  auto vertexBytes = meshDescriptor.GetMeshBufferDescriptor().GetVertexData();
  auto vertices    = xiiMakeArrayPtr(reinterpret_cast<xiiMeshPackedVertex*>(vertexBytes.GetPtr()), vertexBytes.GetCount() / sizeof(xiiMeshPackedVertex));
  for (auto& vertex : vertices)
  {
    vertex.m_vBoneIndices0 = xiiVec4U16::MakeZero();
    vertex.m_BoneWeights0  = xiiColorLinearUB(255, 0, 0, 0);
  }
  meshDescriptor.m_UsageFlags.Add(xiiMeshResourceUsageFlags::Skinned);
  meshDescriptor.AddSection(12, 0, 0);
  meshDescriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Temporal.Box", std::move(meshDescriptor));
  xiiMaterialResourceDescriptor materialDescriptor;
  materialDescriptor.m_BaseColor         = xiiColor(0.8f, 0.16f, 0.06f);
  const auto                    material = xiiResourceManager::CreateResource<xiiMaterialResource>("Temporal.Red", std::move(materialDescriptor));
  xiiMaterialResourceDescriptor floorMaterial;
  floorMaterial.m_BaseColor = xiiColor(0.1f, 0.55f, 0.8f);
  const auto floorSurface   = xiiResourceManager::CreateResource<xiiMaterialResource>("Temporal.Blue", std::move(floorMaterial));
  struct World
  {
    xiiUniquePtr<xiiWorld>            m_World;
    xiiCamera                         m_Camera;
    xiiView*                          m_Views[3];
    xiiSharedPtr<xiiGALTexture>       m_Targets[3];
    xiiGameObjectHandle               m_Box;
    xiiTemporalCameraSnapshot::Sample m_LastSample;
    xiiDynamicArray<float>            m_LastDepth;
    xiiDynamicArray<xiiUInt32>        m_LastIds, m_LastFinal[2];
    double                            m_fRawVariation = 0, m_fResolvedVariation = 0;
  } worlds[2];
  const auto resize = [&](World& world, xiiUInt32 width, xiiUInt32 height) {
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {width, height};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      world.m_Targets[i] = environment.GetDevice()->CreateTexture(target);
      world.m_Views[i]->SetViewport({0, 0, float(width), float(height)});
      world.m_Views[i]->SetRenderTargetView(world.m_Targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  };
  for (auto& world : worlds)
  {
    xiiWorldDescription description("Temporal independent world");
    world.m_World = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(world.m_World->GetWriteMarker());
    world.m_World->SetWorldSimulationEnabled(false);
    const auto addMesh = [&](xiiVec3 position, xiiVec3 scale, const xiiMaterialResourceHandle& surface, bool dynamic) {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_bDynamic      = dynamic;
      objectDescription.m_LocalPosition = position;
      objectDescription.m_LocalScaling  = scale;
      xiiGameObject*        object      = nullptr;
      const auto            handle      = world.m_World->CreateObject(objectDescription, object);
      xiiMeshComponentBase* component   = nullptr;
      if (dynamic)
      {
        xiiSkinnedMeshComponent* skinned = nullptr;
        xiiSkinnedMeshComponent::CreateComponent(object, skinned);
        const auto pose = xiiMat4::MakeIdentity();
        XII_TEST_RESULT(skinned->SetSkinningMatrices(xiiMakeArrayPtr(&pose, 1)));
        component = skinned;
      }
      else
      {
        xiiStaticMeshComponent* fixed = nullptr;
        xiiStaticMeshComponent::CreateComponent(object, fixed);
        component = fixed;
      }
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, surface);
      return handle;
    };
    addMesh({0, 0, -0.1f}, {12, 12, 0.2f}, floorSurface, false);
    world.m_Box = addMesh({0, 0, 0.75f}, {0.8f, 0.8f, 1.5f}, material, true);
    xiiGameObjectDescription sun;
    sun.m_LocalRotation   = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.8f, -0.6f, -1).GetNormalized());
    xiiGameObject* object = nullptr;
    world.m_World->CreateObject(sun, object);
    xiiDirectionalLightComponent* light = nullptr;
    xiiDirectionalLightComponent::CreateComponent(object, light);
    light->SetIntensityUnit(xiiPhotometricUnit::Lux);
    light->SetIntensity(0.1f);
    world.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    world.m_Camera.LookAt({-4, -5, 3}, {0, 0, 0.3f}, {0, 0, 1});
    world.m_Camera.SetExposure(8);
    auto* module = world.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      module->CreateView("Temporal comparison", world.m_Views[i]);
      world.m_Views[i]->SetCamera(&world.m_Camera);
      world.m_Views[i]->SetRenderPipelineResource(pipelines[i]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(world.m_Views[i]->SetDisplayOutputSettings(display));
    }
    resize(world, 129, 97);
  }
  for (xiiUInt32 phase = 0; phase < 24; ++phase)
  {
    if (phase == 18)
      xiiMakeReloadablePipelineTestResource(source, scene.GetResourceID());
    if (phase == 22)
      worlds[0].m_World.Clear();
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto& world = worlds[index];
      if (!world.m_World)
        continue;
      XII_LOCK(world.m_World->GetWriteMarker());
      if (phase == 10 && index == 0)
        world.m_Camera.LookAt({-3.8f, -5, 3}, {0.2f, 0, 0.3f}, {0, 0, 1});
      if (phase == 12 && index == 0)
      {
        xiiGameObject* object = nullptr;
        if (XII_TEST_BOOL(world.m_World->TryGetObject(world.m_Box, object)))
          object->SetLocalPosition(xiiVec3(1.5f, 0, 0.75f));
      }
      if ((phase == 11 || phase == 13) && index == 0)
      {
        xiiGameObject* object = nullptr;
        if (!XII_TEST_BOOL(world.m_World->TryGetObject(world.m_Box, object)))
          return;
        xiiSkinnedMeshComponent* skinned = nullptr;
        if (!XII_TEST_BOOL(object->TryGetComponentOfBaseType(skinned)))
          return;
        const auto pose = xiiMat4::MakeTranslation({0, phase == 11 ? 0.6f : 1.2f, 0});
        XII_TEST_RESULT(skinned->SetSkinningMatrices(xiiMakeArrayPtr(&pose, 1)));
      }
      if (phase == 14)
        for (auto* view : world.m_Views)
          view->InvalidateTemporalHistory();
      if (phase == 16)
        resize(world, 97, 73);
      if (phase == 20)
      {
        world.m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 5, 0.1f, 100);
        for (auto* view : world.m_Views)
          view->InvalidateTemporalHistory();
      }
      if (index == 1 && phase >= 22)
        world.m_Views[1]->SetRenderEnabled(phase != 22);
      world.m_World->Update();
      environment.GetDevice()->WaitIdle();
      for (auto* view : world.m_Views)
        if (view->IsRenderEnabled() && !XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
      if (index == 1 && phase == 22)
        continue; // Other views advance this world while one temporal consumer is paused.
      const auto& disabledInstance = world.m_Views[0]->GetRenderPipelineInstance();
      for (const auto& operation : disabledInstance.GetPipeline()->GetDescriptor().m_Passes)
        if (operation.m_pType == xiiGetStaticRTTI<xiiTemporalCameraPass>() || operation.m_pType == xiiGetStaticRTTI<xiiTemporalAntiAliasingPass>())
          for (const auto& pass : world.m_Views[0]->GetRenderGraph()->GetCompiledPasses())
            if (pass.m_sName.GetView() == operation.m_sName)
              XII_TEST_BOOL(pass.m_bIsCulled == (operation.m_pType == xiiGetStaticRTTI<xiiTemporalAntiAliasingPass>()));
      // Disabling jitter/reconstruction must still preserve camera-motion history.
      // Only the state commit remains live; disabled TAA emits no GPU reconstruction.
      for (xiiUInt32 pass = 0; pass < disabledInstance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++pass)
        if (disabledInstance.GetPipeline()->GetDescriptor().m_Passes[pass].m_pType == xiiGetStaticRTTI<xiiTemporalCameraPass>())
        {
          const auto* binding = disabledInstance.GetOutputBinding(pass, "TemporalCamera");
          const auto* camera  = binding ? xiiDynamicCast<const xiiTemporalCameraSnapshot*>(binding->m_pFrameData.Borrow()) : nullptr;
          if (XII_TEST_BOOL(camera != nullptr))
          {
            XII_TEST_BOOL(!camera->m_Sample.m_bEnabled);
            XII_TEST_VEC2(camera->m_Sample.m_Jitter, xiiVec2::MakeZero(), 0);
            XII_TEST_VEC2(camera->m_Sample.m_PreviousJitter, xiiVec2::MakeZero(), 0);
          }
        }
      const auto image = [&](xiiUInt32 variant, xiiStringView name) {
        auto*       view     = world.m_Views[variant];
        auto&       instance = view->GetRenderPipelineInstance();
        const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        return binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      };
      xiiTemporalCameraSnapshot::Sample sample;
      {
        auto&       instance = world.m_Views[1]->GetRenderPipelineInstance();
        const auto& passes   = instance.GetPipeline()->GetDescriptor().m_Passes;
        for (xiiUInt32 i = 0; i < passes.GetCount(); ++i)
          if (passes[i].m_pType == xiiGetStaticRTTI<xiiTemporalCameraPass>())
          {
            const auto* binding  = instance.GetOutputBinding(i, "TemporalCamera");
            const auto* snapshot = binding ? xiiDynamicCast<const xiiTemporalCameraSnapshot*>(binding->m_pFrameData.Borrow()) : nullptr;
            if (!XII_TEST_BOOL(snapshot != nullptr))
              return;
            sample = snapshot->m_Sample;
          }
      }
      const bool reset = phase == 0 || phase == 14 || phase == 16 || phase == 18 || phase == 20 || (index == 1 && phase == 23);
      XII_TEST_BOOL(sample.m_bEnabled && sample.m_bHistoryValid != reset);
      if (!reset)
      {
        XII_TEST_BOOL(sample.m_PreviousViewProjection.IsEqual(world.m_LastSample.m_Camera.m_ViewProjectionMatrix, 0));
        XII_TEST_BOOL(sample.m_PreviousJitter == world.m_LastSample.m_Jitter);
        XII_TEST_BOOL(sample.m_uiPreviousFrame == world.m_LastSample.m_uiFrame);
      }
      xiiDynamicArray<xiiVec4>   raw, resolved, disabledRaw, disabled;
      xiiDynamicArray<float>     depth;
      xiiDynamicArray<xiiUInt32> ids, final[3];
      struct Motion
      {
        XII_DECLARE_POD_TYPE();
        xiiFloat16 x, y;
      };
      xiiDynamicArray<Motion> velocity;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "RawColor"), raw) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), resolved) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "RawColor"), disabledRaw) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), disabled) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "SceneDepth"), depth) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "ObjectId"), ids) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "Velocity"), velocity))
        return;
      for (xiiUInt32 i = 0; i < 3; ++i)
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), world.m_Targets[i], final[i]))
          return;
      XII_TEST_BOOL(disabled == disabledRaw);
      xiiUInt32 changed = 0, moving = 0, rejected = 0, geometryPixels = 0;
      for (xiiUInt32 pixel = 0; pixel < raw.GetCount(); ++pixel)
      {
        changed += final[1][pixel] != final[2][pixel];
        moving += depth[pixel] > 0 && (xiiMath::Abs(float(velocity[pixel].x)) + xiiMath::Abs(float(velocity[pixel].y))) > 0.001f;
        geometryPixels += depth[pixel] > 0;
        XII_TEST_BOOL(resolved[pixel].IsValid());
        if (reset)
          XII_TEST_BOOL(resolved[pixel] == raw[pixel]);
        if (phase >= 4 && phase < 10)
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
          {
            const auto shift = channel * 8;
            const int  mask  = 255;
            world.m_fResolvedVariation += xiiMath::Abs(int((final[1][pixel] >> shift) & mask) - int((world.m_LastFinal[0][pixel] >> shift) & mask));
            world.m_fRawVariation += xiiMath::Abs(int((final[2][pixel] >> shift) & mask) - int((world.m_LastFinal[1][pixel] >> shift) & mask));
          }
        if ((phase >= 10 && phase <= 13) && index == 0 && depth[pixel] > 0)
        {
          const auto    width = sample.m_Size.width, height = sample.m_Size.height;
          const xiiVec2 ndc((pixel % width + 0.5f) / width * 2 - 1, 1 - (pixel / width + 0.5f) / height * 2);
          auto          worldPoint = sample.m_Camera.m_InverseViewProjectionMatrix * xiiVec4(ndc.x, ndc.y, depth[pixel], 1);
          worldPoint /= worldPoint.w;
          xiiGpuInstanceSource source;
          if (!XII_TEST_RESULT(world.m_Views[1]->GetGpuInstanceData()->Resolve(ids[pixel], source)))
            return;
          if (source.m_hOwnerObject == world.m_Box)
          {
            if (phase == 12)
              worldPoint.x -= 1.5f;
            if (phase == 11 || phase == 13)
              worldPoint.y -= 0.48f;
          }
          const auto oldClip   = sample.m_PreviousViewProjection * worldPoint;
          const auto expectedX = ndc.x - sample.m_Jitter.x - (oldClip.x / oldClip.w - sample.m_PreviousJitter.x);
          const auto expectedY = ndc.y - sample.m_Jitter.y - (oldClip.y / oldClip.w - sample.m_PreviousJitter.y);
          // RG16Float storage allows one half-precision ULP plus reconstruction error.
          XII_TEST_FLOAT(float(velocity[pixel].x), expectedX, 0.0001f + xiiMath::Abs(expectedX) * 0.001f);
          XII_TEST_FLOAT(float(velocity[pixel].y), expectedY, 0.0001f + xiiMath::Abs(expectedY) * 0.001f);
        }
        if (phase == 12 && index == 0 && depth[pixel] > 0)
        {
          const auto    width = sample.m_Size.width, height = sample.m_Size.height;
          const xiiVec2 uv((pixel % width + 0.5f) / width, (pixel / width + 0.5f) / height);
          float         closest = 0;
          auto          dilated = velocity[pixel];
          for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
            {
              const auto sx = xiiMath::Clamp(int(pixel % width) + x, 0, int(width) - 1), sy = xiiMath::Clamp(int(pixel / width) + y, 0, int(height) - 1);
              const auto neighbour = xiiUInt32(sy) * width + xiiUInt32(sx);
              if (depth[neighbour] > closest)
              {
                closest = depth[neighbour];
                dilated = velocity[neighbour];
              }
            }
          const xiiVec2 previousUV = uv - xiiVec2(float(dilated.x) * 0.5f, float(dilated.y) * -0.5f) + xiiVec2((sample.m_PreviousJitter.x - sample.m_Jitter.x) * 0.5f, (sample.m_PreviousJitter.y - sample.m_Jitter.y) * -0.5f);
          if (previousUV.x >= 0 && previousUV.y >= 0 && previousUV.x < 1 && previousUV.y < 1)
          {
            const auto oldPixel = xiiUInt32(previousUV.y * height) * width + xiiUInt32(previousUV.x * width);
            if (ids[pixel] != world.m_LastIds[oldPixel] && world.m_LastDepth[oldPixel] > depth[pixel] * 1.1f)
            {
              ++rejected;
              XII_TEST_BOOL(resolved[pixel] == raw[pixel]);
            }
          }
        }
      }
      XII_TEST_BOOL(geometryPixels > 100);
      if ((phase >= 10 && phase <= 13) && index == 0)
        XII_TEST_BOOL(moving > 20);
      if (phase == 12 && index == 0)
        XII_TEST_BOOL(rejected > 8);
      if (phase == 9)
      {
        XII_TEST_BOOL(changed > 8);
        XII_TEST_BOOL(world.m_fResolvedVariation < world.m_fRawVariation * 0.9);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "Temporal world %u phase %u: %u final pixels, %u moving, %u disoccluded; variation %.0f / %.0f.", index, phase, changed, moving, rejected, world.m_fResolvedVariation, world.m_fRawVariation);
      if (phase == 9 && index == 0 && extendedFormats)
        for (xiiUInt32 variant = 0; variant < 3; ++variant)
        {
          xiiImage capture;
          capture.ResetAndAlloc(world.m_Targets[variant]->GetDescription());
          xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
          xiiStringBuilder path;
          path.SetFormat(":imgout/SceneTemporal-{}.png", variant == 0 ? "Disabled" : variant == 1 ? "Resolved" :
                                                                                                    "Current");
          XII_TEST_RESULT(capture.SaveTo(path));
        }
      world.m_LastSample   = sample;
      world.m_LastDepth    = depth;
      world.m_LastIds      = ids;
      world.m_LastFinal[0] = final[1];
      world.m_LastFinal[1] = final[2];
    }
  }
  xiiRenderGraphTestContext retained;
  if (!XII_TEST_RESULT(retained.Initialize("Retained temporal Scene")))
    return;
  xiiRenderPipelineInstance recording;
  if (!XII_TEST_RESULT(recording.Synchronize(pipelines[1])))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = retained.GetGraph();
  auto& cache = retained.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  graph.BeginSetup(frame.GetFrameIndex());
  {
    XII_LOCK(worlds[1].m_World->GetWriteMarker());
    if (!XII_TEST_RESULT(recording.Build(worlds[1].m_Views[1], graph, retained.GetBlackboard())))
      return;
  }
  const auto output = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("HDRSceneColor", false))->m_hTexture;
  graph.EndSetup();
  recording.Clear();
  worlds[1].m_World.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &retained.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  xiiDynamicArray<xiiVec4> pixels;
  if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(output), pixels))
    return;
  xiiUInt32 lit = 0;
  for (const auto& pixel : pixels)
  {
    XII_TEST_BOOL(pixel.IsValid());
    lit += pixel.x > 0.001f;
  }
  XII_TEST_BOOL(lit > 100);
}

XII_CREATE_SIMPLE_TEST(SceneTemporal, CameraObjectMotionReconstructionAndWorldIsolation)
{
  TestSceneTemporal(true);
}
XII_CREATE_SIMPLE_TEST(SceneTemporal, WithoutExtendedStorageFormatsOrMeshShaders)
{
  TestSceneTemporal(false);
}

XII_CREATE_SIMPLE_TEST(SceneTemporal, RetainedHistorySlotsAndInvalidation)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          source = descriptor.AddPass("Known HDR samples", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
  const auto                          taa    = descriptor.AddPass("Retained TAA", xiiGetStaticRTTI<xiiTemporalAntiAliasingPass>());
  const auto                          sink   = descriptor.AddPass("Read reconstructed samples", xiiGetStaticRTTI<xiiOpaqueTestExport>());
  XII_TEST_RESULT(descriptor.Connect(source, "Color", taa, "HDRSceneColor"));
  XII_TEST_RESULT(descriptor.Connect(source, "Attributes", taa, "DilatedVelocityBuffer"));
  XII_TEST_RESULT(descriptor.Connect(source, "Depth", taa, "SceneDepth"));
  XII_TEST_RESULT(descriptor.Connect(taa, "TAAResolved", sink, "Input"));
  descriptor.m_Interface.PushBack({"Output", false, descriptor.m_Passes[sink].m_Id, "Output"});
  descriptor.m_Interface.PushBack({"Previous", false, descriptor.m_Passes[taa].m_Id, "PreviousColor"});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Temporal.Ownership", std::move(descriptor));
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)))
    return;
  xiiRenderGraphTestContext contexts[3];
  for (auto& context : contexts)
    if (!XII_TEST_RESULT(context.Initialize("TAA retained recordings")))
      return;
  xiiSharedPtr<xiiGALTexture> images[3];
  for (xiiUInt32 phase = 0; phase < 5; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    const auto index   = xiiMath::Min(phase, 2U);
    auto&      context = contexts[index];
    auto&      graph   = context.GetGraph();
    auto&      cache   = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    if (!XII_TEST_RESULT_MSG(instance.Build(nullptr, graph, context.GetBlackboard()), "%s", instance.GetLastBuildError().GetStartPointer()))
      return;
    const auto output   = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Output", false))->m_hTexture;
    const auto previous = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Previous", false))->m_hTexture;
    XII_TEST_BOOL(previous.IsValid() == (phase > 0 && phase < 4));
    if (phase == 3)
      instance.InvalidateTemporalHistory();
    graph.EndSetup();
    if (phase == 4)
      instance.Clear();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    environment.GetDevice()->WaitIdle();
    const auto image = graph.GetExportedTexture(output);
    if (!XII_TEST_BOOL(image != nullptr))
      return;
    for (xiiUInt32 prior = 0; prior < xiiMath::Min(phase, 2U); ++prior)
      XII_TEST_BOOL(image != images[prior]);
    if (phase < 3)
      images[phase] = image;
    struct Pixel
    {
      XII_DECLARE_POD_TYPE();
      xiiFloat16 rgba[4];
    };
    xiiDynamicArray<Pixel> pixels;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image, pixels))
      return;
    for (const auto& pixel : pixels)
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(float(pixel.rgba[channel]), 4, 0);
  }
  // These completed recordings are still alive while the same occurrence advances.
  // Its later writes must not reuse either retained output allocation.
  XII_TEST_BOOL(images[0] != images[1] && images[0] != images[2] && images[1] != images[2]);
}
