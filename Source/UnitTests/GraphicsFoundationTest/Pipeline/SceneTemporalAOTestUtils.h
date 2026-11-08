/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Float16.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Texture/Image/Image.h>

namespace xiiSceneTemporalAOTest
{
  struct Half2
  {
    XII_DECLARE_POD_TYPE();
    xiiFloat16 x, y;
  };
  struct Half4
  {
    XII_DECLARE_POD_TYPE();
    xiiFloat16 x, y, z, w;
  };
  inline xiiVec3 DecodeNormal(float x, float y)
  {
    xiiVec3     normal(x * 2 - 1, y * 2 - 1, 1 - xiiMath::Abs(x * 2 - 1) - xiiMath::Abs(y * 2 - 1));
    const float correction = xiiMath::Max(-normal.z, 0.0f);
    normal.x += normal.x >= 0 ? -correction : correction;
    normal.y += normal.y >= 0 ? -correction : correction;
    return normal.GetNormalized();
  }
  inline void Run(bool editor, bool jittered = false)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(false)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

    const auto                      canonical = xiiLoadPipelineLibraryResource("Scene");
    xiiRenderPipelineResourceHandle instrumented;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return;
      auto descriptor = loaded->GetAuthoring()->m_Descriptor;
      descriptor.m_Interface.PushBack({"CurrentHDR", false, xiiUuid::MakeStableUuidFromString("XII.Scene/Atmosphere"), "HDRSceneColor"});
      instrumented = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("TemporalAO.InstrumentedScene", std::move(descriptor));
    }
    const auto                                 scene = xiiMakeReloadablePipelineTestResource(instrumented, "SceneAO.ReloadableScene");
    xiiResourceLock<xiiRenderPipelineResource> loaded(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
      return;
    const auto          ao         = xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiAmbientOcclusionPass");
    const auto          filter     = xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiAmbientOcclusionTemporalPass");
    const auto*         exportType = xiiGetStaticRTTI<xiiOpaqueTestExport>();
    auto                source     = scene;
    auto                sourceId   = loaded->GetAuthoring()->m_Descriptor.m_Id;
    xiiUuid             editorScene;
    const xiiStringView names[] = {"AmbientOcclusion", "RawAmbientOcclusion", "AmbientOcclusionHistory", "AmbientLighting", "DirectLighting", "HDRSceneColor", "SceneDepth", "ObjectId", "Velocity", "AmbientOcclusionSurface", "CurrentHDR"};
    if (editor)
    {
      const auto                                 editorResource = xiiLoadPipelineLibraryResource("Editor");
      xiiResourceLock<xiiRenderPipelineResource> editorLoaded(editorResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(editorLoaded.IsValid() && editorLoaded->GetAuthoring()))
        return;
      auto definition = editorLoaded->GetAuthoring()->m_Descriptor;
      for (auto& reference : definition.m_References)
        if (reference.m_Id == sourceId)
          reference.m_sResource = scene.GetResourceID();
      for (const auto& pass : definition.m_Passes)
        if (pass.m_Definition == sourceId)
          editorScene = pass.m_Id;
      if (!XII_TEST_BOOL(editorScene.IsValid()))
        return;
      for (const auto name : names)
        if (name != xiiStringView("ObjectId") && name != xiiStringView("SceneDepth"))
          definition.m_Interface.PushBack({name, false, editorScene, name});
      sourceId = definition.m_Id;
      source   = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("TemporalAO.EditorCapture", std::move(definition));
    }
    xiiRenderPipelineResourceHandle pipelines[3];
    for (xiiUInt32 variant = 0; variant < 3; ++variant)
    {
      xiiRenderPipelineResourceDescriptor descriptor;
      descriptor.m_References.PushBack({sourceId, source.GetResourceID()});
      const auto node           = descriptor.AddSubgraph("Production AO", sourceId);
      auto&      cameraOverride = descriptor.m_Passes[node].m_Overrides.ExpandAndGetRef();
      if (editor)
        cameraOverride.m_Path.PushBack(editorScene);
      cameraOverride.m_Path.PushBack(xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiTemporalCameraPass"));
      cameraOverride.m_Properties.Insert("Enabled", jittered);
      auto& filterOverride = descriptor.m_Passes[node].m_Overrides.ExpandAndGetRef();
      if (editor)
        filterOverride.m_Path.PushBack(editorScene);
      filterOverride.m_Path.PushBack(filter);
      filterOverride.m_Properties.Insert("UseHistory", variant == 0);
      filterOverride.m_Properties.Insert("DepthThreshold", 0.005f);
      if (variant == 2)
      {
        auto& disable = descriptor.m_Passes[node].m_Overrides.ExpandAndGetRef();
        if (editor)
          disable.m_Path.PushBack(editorScene);
        disable.m_Path.PushBack(ao);
        disable.m_Properties.Insert("Enabled", false);
      }
      for (const auto name : names)
      {
        const auto capture = descriptor.AddPass(name, exportType);
        XII_TEST_RESULT(descriptor.Connect(node, name, capture, "Input"));
        descriptor.m_Interface.PushBack({name, false, descriptor.m_Passes[capture].m_Id, "Output"});
      }
      xiiStringBuilder id;
      id.SetFormat("TemporalAO.Variant.{}", variant);
      pipelines[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(id, std::move(descriptor));
    }
    // No ray scene or mesh shader is required by this authored screen-space path.
    XII_TEST_BOOL(!loaded->GetPipeline()->GetDescriptor().m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::RayTracing));

    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor meshDescriptor;
    meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
    meshDescriptor.AddSection(12, 0, 0);
    meshDescriptor.ComputeBounds();
    const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("SceneAO.Box", std::move(meshDescriptor));
    xiiMaterialResourceDescriptor materialDescriptor;
    materialDescriptor.m_BaseColor = xiiColor(0.65f, 0.45f, 0.25f);
    const auto material            = xiiResourceManager::CreateResource<xiiMaterialResource>("SceneAO.Diffuse", std::move(materialDescriptor));
    struct World
    {
      xiiUniquePtr<xiiWorld>      m_World;
      xiiRenderWorldModule*       m_Module = nullptr;
      xiiCamera                   m_Camera;
      xiiView*                    m_Views[3];
      xiiSharedPtr<xiiGALTexture> m_Targets[3];
      xiiGameObjectHandle         m_Occluder;
      xiiFrameCameraConstants     m_PreviousCamera;
      xiiDynamicArray<Half4>      m_PreviousSurface;
    } worlds[2];
    const auto resize = [&](World& world, xiiUInt32 width, xiiUInt32 height) {
      for (xiiUInt32 view = 0; view < 3; ++view)
      {
        xiiGALTextureCreationDescription target;
        target.m_Type         = xiiGALResourceDimension::Texture2D;
        target.m_Size         = {width, height};
        target.m_Format       = xiiGALResourceFormat::RGBA8UNormalized;
        target.m_BindFlags    = xiiGALBindFlags::RenderTarget;
        world.m_Targets[view] = environment.GetDevice()->CreateTexture(target);
        world.m_Views[view]->SetViewport({0, 0, float(width), float(height)});
        world.m_Views[view]->SetRenderTargetView(world.m_Targets[view]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
    };
    for (xiiUInt32 index = 0; index < 2; ++index)
    {
      auto&               world = worlds[index];
      xiiWorldDescription worldDescription("AO independent world");
      world.m_World = XII_DEFAULT_NEW(xiiWorld, worldDescription);
      XII_LOCK(world.m_World->GetWriteMarker());
      world.m_World->SetWorldSimulationEnabled(false);
      world.m_Module = world.m_World->GetOrCreateModule<xiiRenderWorldModule>();
      for (xiiUInt32 objectIndex = 0; objectIndex < 2; ++objectIndex)
      {
        xiiGameObjectDescription description;
        description.m_LocalPosition = objectIndex ? xiiVec3(index ? 0.5f : 0, 0, 0.5f) : xiiVec3(0, 0, -0.1f);
        description.m_LocalScaling  = objectIndex ? xiiVec3(1) : xiiVec3(8, 8, 0.2f);
        description.m_bDynamic      = objectIndex != 0;
        xiiGameObject* object       = nullptr;
        const auto     handle       = world.m_World->CreateObject(description, object);
        if (objectIndex)
          world.m_Occluder = handle;
        xiiStaticMeshComponent* component = nullptr;
        xiiStaticMeshComponent::CreateComponent(object, component);
        component->SetMesh(mesh);
        component->SetMaterialOverride(0, material);
      }
      xiiGameObject* lightObject = nullptr;
      world.m_World->CreateObject({}, lightObject);
      xiiDirectionalLightComponent* light = nullptr;
      xiiDirectionalLightComponent::CreateComponent(lightObject, light);
      light->SetIntensityUnit(xiiPhotometricUnit::Lux);
      light->SetIntensity(0.1f);
      world.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      world.m_Camera.LookAt({-4, -5, 3}, {0, 0, 0.2f}, {0, 0, 1});
      world.m_Camera.SetExposure(8);
      for (xiiUInt32 view = 0; view < 3; ++view)
      {
        world.m_Module->CreateView("AO enabled/disabled comparison", world.m_Views[view]);
        world.m_Views[view]->SetCamera(&world.m_Camera);
        world.m_Views[view]->SetCameraUsageHint(editor ? xiiCameraUsageHint::EditorView : xiiCameraUsageHint::MainView);
        world.m_Views[view]->SetPipelineParameter("Editor.SelectionOverlay", false);
        world.m_Views[view]->SetPipelineParameter("Editor.ShapeIcons", false);
        world.m_Views[view]->SetRenderPipelineResource(pipelines[view]);
        xiiDisplayOutputSettings display;
        display.m_Exposure.m_Mode = xiiExposureMode::Manual;
        display.m_fBloomStrength  = 0;
        XII_TEST_RESULT(world.m_Views[view]->SetDisplayOutputSettings(display));
      }
      resize(world, 129, 97);
    }
    xiiDynamicArray<xiiUInt8>   previousMasks[2];
    xiiSharedPtr<xiiGALTexture> masks[2];
    for (xiiUInt32 phase = 0; phase < 16; ++phase)
    {
      if (phase == 5)
        xiiMakeReloadablePipelineTestResource(instrumented, scene.GetResourceID());
      for (xiiUInt32 index = 0; index < 2; ++index)
      {
        auto& world = worlds[index];
        XII_LOCK(world.m_World->GetWriteMarker());
        if (phase == 3)
          for (auto* view : world.m_Views)
            view->InvalidateTemporalHistory();
        if (phase == 4)
          resize(world, 97, 73);
        if (phase == 6 || phase == 7)
          world.m_Camera.LookAt(phase == 6 ? xiiVec3(-3, -4, 2.5f) : xiiVec3(-2.8f, -3.8f, 2.4f), {0, 0, 0.2f}, {0, 0, 1});
        if (phase == 8 && index == 0)
        {
          xiiGameObject* object = nullptr;
          if (world.m_World->TryGetObject(world.m_Occluder, object))
            object->SetLocalPosition(xiiVec3(1.5f, 0, 0.5f));
        }
        if (phase == 10 || phase == 13)
        {
          if (phase == 13)
            world.m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 5, 0.1f, 100);
          for (auto* view : world.m_Views)
            view->InvalidateTemporalHistory();
        }
        if (phase == 11 && index == 0)
          world.m_World->DeleteObjectNow(world.m_Occluder);
        world.m_World->Update();
        environment.GetDevice()->WaitIdle();
        for (auto* view : world.m_Views)
          if (!XII_TEST_RESULT(view->GetLastRenderResult()))
            return;
        if (phase < 2)
          continue;
        const auto image = [&](xiiUInt32 view, xiiStringView name) {
          const auto& instance = world.m_Views[view]->GetRenderPipelineInstance();
          const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
          return world.m_Views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture);
        };
        xiiDynamicArray<xiiUInt8> mask;
        masks[index] = image(0, "AmbientOcclusion");
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), masks[index], mask))
          return;
        XII_TEST_BOOL(image(2, "AmbientOcclusion") == nullptr);
        xiiDynamicArray<xiiUInt8> spatial, raw;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "AmbientOcclusion"), spatial) ||
            !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "RawAmbientOcclusion"), raw))
          return;
        const auto width          = masks[index]->GetDescription().m_Size.width;
        const auto height         = masks[index]->GetDescription().m_Size.height;
        xiiUInt32  temporalPixels = 0;
        for (xiiUInt32 pixel = 0; pixel < mask.GetCount(); ++pixel)
        {
          temporalPixels += mask[pixel] != spatial[pixel];
          xiiUInt8 minimum = raw[pixel], maximum = raw[pixel];
          for (xiiInt32 y = -1; y <= 1; ++y)
            for (xiiInt32 x = -1; x <= 1; ++x)
            {
              const auto neighbour = xiiMath::Clamp(xiiInt32(pixel / width) + y, 0, xiiInt32(height) - 1) * width + xiiMath::Clamp(xiiInt32(pixel % width) + x, 0, xiiInt32(width) - 1);
              minimum              = xiiMath::Min(minimum, raw[neighbour]);
              maximum              = xiiMath::Max(maximum, raw[neighbour]);
            }
          XII_TEST_BOOL(mask[pixel] + 1 >= minimum && mask[pixel] <= xiiUInt32(maximum) + 1);
        }
        if (phase == 3 || phase == 4 || phase == 5 || phase == 10 || phase == 13)
          XII_TEST_BOOL(mask == spatial);
        if (phase == 6 || phase == 7)
          XII_TEST_BOOL(temporalPixels > 8);
        const auto history = image(0, "AmbientOcclusionHistory");
        if (phase == 7 || phase == 8 || phase == 9 || phase == 12 || phase == 15)
        {
          xiiDynamicArray<xiiUInt8> previous;
          if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), history, previous))
            return;
          XII_TEST_BOOL(previous == previousMasks[index]);
        }
        xiiTestFramework::Output(xiiTestOutput::Details, "Temporal AO world %u phase %u: %u history-dependent pixels.", index, phase, temporalPixels);
        xiiDynamicArray<xiiVec4>   ambient[3], direct[3], hdr[3];
        xiiDynamicArray<float>     depth[3];
        xiiDynamicArray<xiiUInt32> output[3];
        for (xiiUInt32 view = 0; view < 3; ++view)
          if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "AmbientLighting"), ambient[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "DirectLighting"), direct[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "CurrentHDR"), hdr[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(view, "SceneDepth"), depth[view]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), world.m_Targets[view], output[view]))
            return;
        xiiDynamicArray<Half2> velocity;
        xiiDynamicArray<Half4> surface;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "Velocity"), velocity) ||
            !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "AmbientOcclusionSurface"), surface))
          return;
        xiiSharedPtr<const xiiTemporalCameraSnapshot> samples[3];
        for (xiiUInt32 view = 0; view < 3; ++view)
        {
          const auto& instance = world.m_Views[view]->GetRenderPipelineInstance();
          for (xiiUInt32 pass = 0; pass < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++pass)
            if (instance.GetPass(pass)->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTemporalCameraPass>())
              samples[view] = instance.GetOutputBinding(pass, "TemporalCamera")->m_pFrameData.Downcast<const xiiTemporalCameraSnapshot>();
          if (!XII_TEST_BOOL(samples[view] != nullptr))
            return;
          XII_TEST_BOOL(samples[view]->m_Sample.m_bEnabled == jittered);
          XII_TEST_BOOL(samples[view]->m_Sample.m_Jitter == samples[0]->m_Sample.m_Jitter);
          XII_TEST_BOOL(samples[view]->m_Sample.m_uiSequence == samples[0]->m_Sample.m_uiSequence);
        }
        const auto& camera = samples[0]->m_Sample.m_Camera;
        if (jittered)
        {
          XII_TEST_BOOL(!samples[0]->m_Sample.m_Jitter.IsZero());
          xiiDynamicArray<xiiVec4> resolved;
          if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(0, "HDRSceneColor"), resolved))
            return;
          xiiUInt32 reconstructed = 0, occludedReconstructed = 0;
          for (xiiUInt32 pixel = 0; pixel < resolved.GetCount(); ++pixel)
          {
            XII_TEST_BOOL(resolved[pixel].IsValid());
            const bool changed = !resolved[pixel].IsEqual(hdr[0][pixel], 0.000001f);
            reconstructed += changed;
            occludedReconstructed += changed && mask[pixel] < 250;
          }
          xiiTestFramework::Output(xiiTestOutput::Details, "Jittered AO phase %u: sequence %u, %u reconstructed HDR pixels (%u occluded).", phase, samples[0]->m_Sample.m_uiSequence, reconstructed, occludedReconstructed);
          if (phase == 2 || phase == 6 || phase == 7)
          {
            XII_TEST_BOOL(reconstructed > 16);
            XII_TEST_BOOL(occludedReconstructed > 8);
          }
        }
        if (phase == 6 || phase == 7)
        {
          xiiUInt32 verified = 0, differentCameraDepths = 0, matched = 0;
          for (xiiUInt32 pixel = 0; pixel < mask.GetCount(); ++pixel)
          {
            if (surface[pixel].w < 0.5f)
              continue;
            const float u = (float(pixel % width) + 0.5f) / width, v = (float(pixel / width) + 0.5f) / height;
            const auto  jitterDelta = samples[0]->m_Sample.m_PreviousJitter - samples[0]->m_Sample.m_Jitter;
            const float previousU = u + (jitterDelta.x - float(velocity[pixel].x)) * 0.5f, previousV = v - (jitterDelta.y - float(velocity[pixel].y)) * 0.5f;
            if (previousU <= 0 || previousV <= 0 || previousU >= 1 || previousV >= 1)
              continue;
            const auto  previousPixel = xiiUInt32(previousV * height) * width + xiiUInt32(previousU * width);
            const auto& previous      = world.m_PreviousSurface[previousPixel];
            if (previous.w < 0.5f)
              continue;
            auto position = camera.m_InverseViewProjectionMatrix * xiiVec4(u * 2 - 1, 1 - v * 2, depth[0][pixel], 1);
            position /= position.w;
            const float expectedDepth = (position.GetAsVec3() - world.m_PreviousCamera.m_CameraPositionAndNearPlane.GetAsVec3()).Dot(world.m_PreviousCamera.m_CameraForwardAndFarPlane.GetAsVec3());
            const float tolerance     = xiiMath::Max(expectedDepth * 0.005f, 0.01f);
            const auto  normal        = DecodeNormal(surface[pixel].x, surface[pixel].y);
            if (expectedDepth <= 0 || xiiMath::Abs(float(previous.z) - expectedDepth) > tolerance || normal.Dot(DecodeNormal(previous.x, previous.y)) < 0.82f)
              continue;
            ++matched;
            float minimum = raw[pixel], maximum = raw[pixel];
            bool  flat = true;
            for (xiiInt32 y = -1; y <= 1; ++y)
              for (xiiInt32 x = -1; x <= 1; ++x)
              {
                const auto neighbour = xiiMath::Clamp(xiiInt32(pixel / width) + y, 0, xiiInt32(height) - 1) * width + xiiMath::Clamp(xiiInt32(pixel % width) + x, 0, xiiInt32(width) - 1);
                if (surface[neighbour].w < 0.5f)
                  continue;
                const float difference = xiiMath::Abs(float(surface[neighbour].z) - float(surface[pixel].z));
                const float threshold  = xiiMath::Max(float(surface[pixel].z) * 0.005f, 0.01f);
                flat &= xiiMath::Abs(difference - threshold) > 0.004f;
                if (difference > threshold || normal.Dot(DecodeNormal(surface[neighbour].x, surface[neighbour].y)) < 0.82f)
                  continue;
                minimum = xiiMath::Min(minimum, float(raw[neighbour]));
                maximum = xiiMath::Max(maximum, float(raw[neighbour]));
              }
            if (!flat)
              continue;
            const float expected = xiiMath::Lerp(float(spatial[pixel]), xiiMath::Clamp(float(previousMasks[index][previousPixel]), minimum, maximum), 0.92f);
            XII_TEST_FLOAT(float(mask[pixel]), expected, 1.1f);
            ++verified;
            differentCameraDepths += xiiMath::Abs(float(previous.z) - float(surface[pixel].z)) > tolerance && xiiMath::Abs(expected - spatial[pixel]) > 1.5f;
          }
          xiiTestFramework::Output(xiiTestOutput::Details, "Temporal AO motion oracle: %u matched surfaces, %u reprojections, %u prior-camera depth corrections.", matched, verified, differentCameraDepths);
          XII_TEST_BOOL(verified > 100);
          if (phase == 6)
            XII_TEST_BOOL(differentCameraDepths > 8);
        }
        previousMasks[index]    = mask;
        world.m_PreviousSurface = surface;
        world.m_PreviousCamera  = camera;
        xiiUInt32 darkened = 0, visible = 0, affectedOutput = 0;
        xiiUInt8  minimum = 255;
        for (xiiUInt32 pixel = 0; pixel < mask.GetCount(); ++pixel)
        {
          XII_TEST_FLOAT(depth[0][pixel], depth[2][pixel], 0);
          const float visibility = float(mask[pixel]) / 255;
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
          {
            XII_TEST_FLOAT(direct[0][pixel].GetData()[channel], direct[2][pixel].GetData()[channel], 0.000001f);
            XII_TEST_FLOAT(ambient[0][pixel].GetData()[channel], ambient[2][pixel].GetData()[channel] * visibility, 0.000001f);
            XII_TEST_FLOAT(hdr[2][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], ambient[2][pixel].GetData()[channel] - ambient[0][pixel].GetData()[channel], 0.000001f);
          }
          if (depth[0][pixel] > 0)
          {
            ++visible;
            minimum = xiiMath::Min(minimum, mask[pixel]);
            darkened += mask[pixel] < 250;
            affectedOutput += output[0][pixel] != output[2][pixel];
          }
          else
            XII_TEST_INT(mask[pixel], 255);
        }
        xiiTestFramework::Output(xiiTestOutput::Details, "AO world %u phase %u: %u visible, %u occluded, minimum %u, %u final pixels changed.", index, phase, visible, darkened, minimum, affectedOutput);
        XII_TEST_BOOL(visible > 100);
        if (phase >= 11 && index == 0)
        {
          XII_TEST_INT(darkened, 0);
          XII_TEST_INT(affectedOutput, 0);
        }
        else
        {
          XII_TEST_BOOL(darkened > 8 && minimum < 230);
          XII_TEST_BOOL(affectedOutput > 8);
        }
        const auto& enabledStats  = world.m_Views[0]->GetRenderGraph()->GetStatistics();
        const auto& disabledStats = world.m_Views[2]->GetRenderGraph()->GetStatistics();
        XII_TEST_BOOL(enabledStats.m_uiCompiledPassCount >= disabledStats.m_uiCompiledPassCount + 2);
      }
      if (phase >= 2)
        XII_TEST_BOOL(masks[0] != masks[1]);
    }
  }
} // namespace xiiSceneTemporalAOTest
