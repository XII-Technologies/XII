/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Lights/SkyLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace xiiSceneProbeTest
{
  inline xiiTextureCubeResourceHandle Cube(xiiStringView name, xiiColor coarse, xiiColor fine, bool directional = false)
  {
    xiiTextureCubeResourceDescriptor descriptor;
    auto&                            image = descriptor.m_TextureDescription;
    image.m_Type                           = xiiGALResourceDimension::TextureCube;
    image.m_Size                           = {2, 2};
    image.m_uiArraySizeOrDepth             = 6;
    image.m_uiMipLevels                    = 2;
    image.m_Format                         = xiiGALResourceFormat::RGBA32Float;
    image.m_BindFlags                      = xiiGALBindFlags::ShaderResource;
    xiiVec4                      pixels[6][5];
    xiiGALTextureSubResourceData data[12];
    for (xiiUInt32 face = 0; face < 6; ++face)
    {
      const auto base = directional && face == 5 ? xiiColor(0, 2, 0) : fine;
      for (xiiUInt32 i = 0; i < 4; ++i)
        pixels[face][i] = xiiVec4(base.r, base.g, base.b, 1);
      const auto mipColor = directional ? base : coarse;
      pixels[face][4]     = xiiVec4(mipColor.r, mipColor.g, mipColor.b, 1);
      data[face * 2]     = xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(pixels[face]), 4 * sizeof(xiiVec4)), 2 * sizeof(xiiVec4), 4 * sizeof(xiiVec4));
      data[face * 2 + 1] = xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&pixels[face][4]), sizeof(xiiVec4)), sizeof(xiiVec4), sizeof(xiiVec4));
    }
    descriptor.m_InitialContent = data;
    return xiiResourceManager::CreateResource<xiiTextureCubeResource>(name, std::move(descriptor));
  }

  inline void Run(bool editor, bool restricted = false)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(false, !restricted)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    const auto                                 sceneResource = xiiLoadPipelineLibraryResource("Scene");
    xiiResourceLock<xiiRenderPipelineResource> scene(sceneResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(scene.IsValid() && scene->GetAuthoring()))
      return;
    auto    source   = sceneResource;
    auto    sourceId = scene->GetAuthoring()->m_Descriptor.m_Id;
    xiiUuid editorScene;
    if (editor)
    {
      const auto                                 editorResource = xiiLoadPipelineLibraryResource("Editor");
      xiiResourceLock<xiiRenderPipelineResource> loaded(editorResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return;
      auto    descriptor = loaded->GetAuthoring()->m_Descriptor;
      xiiUuid occurrence;
      for (const auto& pass : descriptor.m_Passes)
        if (pass.m_Definition == sourceId)
          occurrence = pass.m_Id;
      editorScene = occurrence;
      if (!XII_TEST_BOOL(occurrence.IsValid()))
        return;
      for (const xiiStringView name : {"EnvironmentSpecular", "EnvironmentIrradiance", "ReflectionLighting", "TransparentAccumulation", "HDRSceneColor"})
        descriptor.m_Interface.PushBack({name, false, occurrence, name});
      sourceId = descriptor.m_Id;
      source   = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Probes.EditorCapture", std::move(descriptor));
    }
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({sourceId, source.GetResourceID()});
    const auto renderer = root.AddSubgraph("Authored renderer", sourceId);
    for (const xiiStringView name : {"EnvironmentSpecular", "EnvironmentIrradiance", "ReflectionLighting", "HDRSceneColor", "ObjectId", "SceneDepth", "TransparentAccumulation"})
    {
      const auto capture = root.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(renderer, name, capture, "Input"));
      root.m_Interface.PushBack({name, false, root.m_Passes[capture].m_Id, "Output"});
    }
    if (restricted)
      for (const xiiStringView type : {"xiiWorldReflectionProbesPass", "xiiAmbientOcclusionPass"})
      {
        auto& policy = root.m_Passes[renderer].m_Overrides.ExpandAndGetRef();
        if (editor)
          policy.m_Path.PushBack(editorScene);
        policy.m_Path.PushBack(xiiFindPipelineTestPass(scene->GetAuthoring()->m_Descriptor, type));
        policy.m_Properties.Insert("Enabled", false);
      }
    const auto                     pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Probes.ProductionCapture", std::move(root));
    const auto                     red      = Cube("Probes.Red", {2, 0, 0}, {2, 0, 0});
    const auto                     blue     = Cube("Probes.Blue", {0, 0, 2}, {0, 0, 2});
    const auto                     mipMap   = Cube("Probes.SourceFaces", {2, 0, 0}, {2, 0, 0}, true);
    xiiTexture2DResourceDescriptor whiteDescriptor;
    whiteDescriptor.m_TextureDescription.m_Size        = {1, 1};
    whiteDescriptor.m_TextureDescription.m_uiMipLevels = 1;
    whiteDescriptor.m_TextureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
    const xiiUInt8               whitePixel[]          = {255, 255, 255, 255};
    xiiGALTextureSubResourceData whiteData(xiiConstByteBlobPtr(whitePixel, sizeof(whitePixel)), 4, 4);
    whiteDescriptor.m_InitialContent = xiiMakeArrayPtr(&whiteData, 1);
    const auto  white                = xiiResourceManager::CreateResource<xiiTexture2DResource>("Probes.White", std::move(whiteDescriptor));
    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor meshDescriptor;
    meshDescriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    meshDescriptor.AddSection(12, 0, 0);
    meshDescriptor.ComputeBounds();
    const auto mesh     = xiiResourceManager::CreateResource<xiiMeshResource>("Probes.Box", std::move(meshDescriptor));
    const auto material = [&](xiiStringView name, float roughness, bool transparent, bool authored = false, float metallic = 1.0f) {
      xiiMaterialResourceDescriptor descriptor;
      descriptor.m_BaseColor  = xiiColor(0.8f, 0.8f, 0.8f, transparent ? 0.5f : 1.0f);
      descriptor.m_fMetallic  = metallic;
      descriptor.m_fRoughness = roughness;
      descriptor.m_BlendMode  = transparent ? xiiMaterialBlendMode::Translucent : xiiMaterialBlendMode::Opaque;
      if (authored)
      {
        descriptor.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Materials/DefaultMaterial.xiiShader");
        auto& blend          = descriptor.m_PermutationVariables.ExpandAndGetRef();
        blend.m_sName.Assign("BLEND_MODE");
        blend.m_sValue.Assign("BLEND_MODE_TRANSPARENT");
        for (const xiiStringView name : {"BaseTexture", "NormalTexture", "OcclusionTexture", "RoughnessTexture", "MetallicTexture", "OrmTexture", "EmissiveTexture"})
        {
          auto& binding = descriptor.m_Texture2DBindings.ExpandAndGetRef();
          binding.m_Name.Assign(name);
          binding.m_Value = white;
        }
        for (const xiiStringView parameter : {"BaseColor", "MetallicValue", "RoughnessValue"})
        {
          auto& value = descriptor.m_Parameters.ExpandAndGetRef();
          value.m_Name.Assign(parameter);
          value.m_Value = parameter == "BaseColor" ? xiiVariant(xiiColor(0.8f, 0.8f, 0.8f, 0.5f)) : parameter == "MetallicValue" ? xiiVariant(metallic) :
                                                                                                                                   xiiVariant(0.02f);
        }
      }
      return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(descriptor));
    };
    const auto          mirror              = material("Probes.Mirror", 0.02f, false);
    const auto          rough               = material("Probes.Rough", 1, false);
    const auto          translucent         = material("Probes.Translucent", 0.02f, true);
    const auto          authoredTranslucent = material("Probes.AuthoredTranslucent", 0.02f, true, true);
    const auto          dielectric            = material("Probes.Dielectric", 0.02f, false, false, 0);
    const auto          dielectricTransparent = material("Probes.DielectricTransparent", 0.02f, true, false, 0);
    const auto          dielectricAuthored    = material("Probes.DielectricAuthored", 0.02f, true, true, 0);
    xiiWorldDescription description("Authored reflection probe world");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiComponentHandle  surfaces[2], probes[2];
    xiiGameObjectHandle owners[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_bDynamic = true;
      xiiGameObject* object        = nullptr;
      world.CreateObject(objectDescription, object);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, i ? translucent : mirror);
      component->SetActiveFlag(i == 0);
      surfaces[i]                            = component->GetHandle();
      xiiGameObject* captureObject           = nullptr;
      owners[i]                              = world.CreateObject(objectDescription, captureObject);
      xiiReflectionCaptureComponent* capture = nullptr;
      xiiReflectionCaptureComponent::CreateComponent(captureObject, capture);
      capture->SetReflectionMap(i ? blue : red);
      capture->SetHalfExtents(xiiVec3(4));
      capture->SetBlendDistance(0);
      capture->SetPriority(xiiMath::MinValue<xiiInt32>());
      capture->SetActiveFlag(restricted && i == 0);
      probes[i] = capture->GetHandle();
    }
    xiiComponentHandle skies[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiGameObjectDescription distant;
      distant.m_bDynamic      = true;
      distant.m_LocalPosition = xiiVec3(5000, 8000, -3000);
      distant.m_LocalScaling  = xiiVec3(2, 0.5f, 3);
      xiiGameObject* owner    = nullptr;
      world.CreateObject(distant, owner);
      xiiSkyLightComponent* sky = nullptr;
      xiiSkyLightComponent::CreateComponent(owner, sky);
      sky->SetEnvironmentMap(i ? blue : red);
      sky->SetPriority(xiiMath::MinValue<xiiInt32>());
      sky->SetActiveFlag(false);
      skies[i] = sky->GetHandle();
    }
    xiiGameObjectDescription atmosphereObject;
    xiiGameObject*           atmosphereOwner = nullptr;
    world.CreateObject(atmosphereObject, atmosphereOwner);
    xiiSkyAtmosphereComponent* atmosphere = nullptr;
    xiiSkyAtmosphereComponent::CreateComponent(atmosphereOwner, atmosphere);
    xiiAtmosphereSettings extinction;
    extinction.m_vRayleighScattering = xiiVec3::MakeZero();
    extinction.m_vMieScattering      = xiiVec3::MakeZero();
    extinction.m_vOzoneAbsorption    = xiiVec3::MakeZero();
    extinction.m_vMieAbsorption      = xiiVec3(100);
    extinction.m_fMieScaleHeightKm   = 1000000;
    atmosphere->SetAtmosphereSettings(extinction);
    atmosphere->SetActiveFlag(false);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 0.9f, 0.1f, 100);
    camera.LookAt({-4, 0, 0}, {0, 0, 0}, {0, 0, 1});
    camera.SetExposure(0.25f);
    xiiView* view = nullptr;
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Production probe lighting", view);
    view->SetCamera(&camera);
    view->SetCameraUsageHint(editor ? xiiCameraUsageHint::EditorView : xiiCameraUsageHint::MainView);
    view->SetViewport({0, 0, 33, 25});
    view->SetRenderPipelineResource(pipeline);
    view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3::MakeZero());
    view->SetPipelineParameter("Editor.SelectionOverlay", false);
    view->SetPipelineParameter("Editor.ShapeIcons", false);
    xiiDisplayOutputSettings display;
    display.m_Exposure.m_Mode = xiiExposureMode::Manual;
    display.m_fBloomStrength  = 0;
    XII_TEST_RESULT(view->SetDisplayOutputSettings(display));
    xiiGALTextureCreationDescription targetDescription;
    targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    targetDescription.m_Size      = {33, 25};
    targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
    view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    const auto image = [&](xiiStringView name) {
      const auto& instance = view->GetRenderPipelineInstance();
      const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
      return binding && binding->m_hTexture.IsValid() ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
    };
    struct Timing
    {
      double    m_fMilliseconds = 0;
      xiiUInt32 m_uiSamples     = 0;
    };
    xiiMap<xiiString, Timing>                                                              timings;
    xiiMutex                                                                               timingMutex;
    xiiEvent<const xiiRenderGraphTimestampProfiler::TimingSample&, xiiMutex>::Unsubscriber subscription;
    xiiRenderGraphTimestampProfiler::s_TimingEvents.AddEventHandler([&](const auto& sample) {
      if (sample.m_bAvailable && !sample.m_bSubmission && (sample.m_sName.GetView().FindSubString("Probe") || sample.m_sName.GetView().FindSubString("EnvironmentSpecular")))
      {
        XII_LOCK(timingMutex);
        auto& timing = timings[xiiString(sample.m_sName.GetView())];
        timing.m_fMilliseconds += sample.m_fDurationMs;
        ++timing.m_uiSamples;
      }
    },
                                                                    subscription);
    xiiTime   cpuUpdates;
    xiiUInt32 cpuSamples = 0;
    xiiVec4   center[42];
    for (xiiUInt32 phase = 0; phase < (restricted ? 1U : 42U); ++phase)
    {
      xiiReflectionCaptureComponent *first = nullptr, *second = nullptr;
      xiiMeshComponent *             opaque = nullptr, *transparent = nullptr;
      if (!XII_TEST_BOOL(world.TryGetComponent(probes[0], first) && world.TryGetComponent(probes[1], second) && world.TryGetComponent(surfaces[0], opaque) && world.TryGetComponent(surfaces[1], transparent)))
        return;
      xiiSkyLightComponent *sky = nullptr, *otherSky = nullptr;
      if (!XII_TEST_BOOL(world.TryGetComponent(skies[0], sky) && world.TryGetComponent(skies[1], otherSky)))
        return;
      if (phase == 1)
        first->SetActiveFlag(true);
      if (phase == 2)
        second->SetActiveFlag(true);
      if (phase == 3)
        first->SetPriority(10);
      if (phase == 4)
        first->SetActiveFlag(false);
      if (phase == 5)
      {
        first->SetActiveFlag(true);
        first->SetInfluenceShape(xiiReflectionProbeInfluenceShape::Sphere);
        first->SetSphereRadius(1);
        first->SetBlendDistance(1);
      }
      if (phase == 6)
      {
        first->SetReflectionMap(blue);
        second->SetActiveFlag(false);
      }
      if (phase == 7)
        first->SetSaturation(0);
      if (phase == 8)
        first->SetActiveFlag(false);
      if (phase == 9)
      {
        first->SetActiveFlag(true);
        first->SetReflectionMap(mipMap);
        first->SetSaturation(1);
        first->SetInfluenceShape(xiiReflectionProbeInfluenceShape::Box);
        first->SetBlendDistance(0);
        first->SetParallaxCorrected(false);
      }
      if (phase == 10)
        first->GetOwner()->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90)));
      if (phase == 11)
      {
        first->GetOwner()->SetLocalRotation(xiiQuat::MakeIdentity());
        opaque->SetMaterialOverride(0, rough);
      }
      if (phase == 12)
      {
        first->GetOwner()->SetLocalScaling(xiiVec3(2, 1, 0.5f));
        opaque->SetMaterialOverride(0, mirror);
      }
      if (phase == 13)
        first->SetActiveFlag(false);
      if (phase == 14)
      {
        first->SetActiveFlag(true);
        first->SetReflectionMap(red);
        opaque->SetActiveFlag(false);
        transparent->SetActiveFlag(true);
      }
      if (phase == 15)
        first->SetReflectionMap(blue);
      if (phase == 16)
      {
        opaque->SetActiveFlag(true);
        transparent->SetActiveFlag(false);
        atmosphere->SetActiveFlag(true);
      }
      if (phase == 18)
      {
        opaque->SetActiveFlag(false);
        transparent->SetActiveFlag(true);
        transparent->SetMaterialOverride(0, authoredTranslucent);
        first->SetReflectionMap(red);
      }
      if (phase == 19)
        first->SetReflectionMap(blue);
      if (phase == 20)
      {
        opaque->SetActiveFlag(true);
        transparent->SetActiveFlag(false);
        first->SetReflectionMap(mipMap);
        first->SetHalfExtents(xiiVec3(1, 10, 1));
        first->GetOwner()->SetLocalPosition(xiiVec3(0, -5, 0));
        first->SetParallaxCorrected(false);
      }
      if (phase == 21)
        first->SetParallaxCorrected(true);
      if (phase == 22)
      {
        first->GetOwner()->SetLocalPosition(xiiVec3::MakeZero());
        first->GetOwner()->SetLocalScaling(xiiVec3(1));
        first->SetHalfExtents(xiiVec3(4));
        first->SetReflectionMap(red);
        opaque->SetMaterialOverride(0, dielectric);
        view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3(20, 30, 40));
      }
      if (phase == 23)
      {
        first->SetIntensity(0);
        view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3::MakeZero());
      }
      if (phase == 24)
      {
        first->SetActiveFlag(false);
        view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3::MakeZero());
      }
      if (phase == 25)
      {
        first->SetActiveFlag(true);
        first->SetIntensity(1);
        opaque->SetActiveFlag(false);
        transparent->SetActiveFlag(true);
        transparent->SetMaterialOverride(0, dielectricTransparent);
      }
      if (phase == 26)
        transparent->SetMaterialOverride(0, dielectricAuthored);
      if (phase == 27)
      {
        first->SetActiveFlag(false);
        second->SetActiveFlag(false);
        sky->SetActiveFlag(true);
      }
      if (phase == 28)
      {
        opaque->SetActiveFlag(true);
        opaque->SetMaterialOverride(0, dielectric);
        transparent->SetActiveFlag(false);
        view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3(20, 30, 40));
      }
      if (phase == 29)
        otherSky->SetActiveFlag(true);
      if (phase == 30)
        otherSky->SetPriority(10);
      if (phase == 31)
      {
        otherSky->SetActiveFlag(false);
        sky->SetEnvironmentMap(mipMap);
        opaque->SetMaterialOverride(0, mirror);
      }
      if (phase == 32)
        sky->GetOwner()->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90)));
      if (phase == 33)
      {
        sky->GetOwner()->SetLocalRotation(xiiQuat::MakeIdentity());
        sky->SetSaturation(0);
      }
      if (phase == 34)
        sky->SetIntensity(0.5f);
      if (phase == 35)
      {
        sky->SetIntensity(1);
        sky->SetSaturation(1);
        sky->SetEnvironmentMap(blue);
        first->SetActiveFlag(true);
        first->SetInfluenceShape(xiiReflectionProbeInfluenceShape::Sphere);
        first->SetSphereRadius(1);
        first->SetBlendDistance(1);
        opaque->SetMaterialOverride(0, dielectric);
      }
      if (phase == 36)
        first->SetBlendDistance(0);
      if (phase == 37)
      {
        first->SetActiveFlag(false);
        sky->SetIntensity(0);
        view->SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3::MakeZero());
      }
      if (phase == 38)
      {
        sky->SetIntensity(1);
        sky->SetActiveFlag(false);
      }
      if (phase == 39)
        sky->SetActiveFlag(true);
      if (phase == 40)
      {
        opaque->SetActiveFlag(false);
        transparent->SetActiveFlag(true);
        transparent->SetMaterialOverride(0, dielectricTransparent);
      }
      if (phase == 41)
        transparent->SetMaterialOverride(0, dielectricAuthored);
      if (phase == 17)
        atmosphere->SetActiveFlag(false);
      if (phase == 12)
      {
        targetDescription.m_Size = {129, 97};
        target                   = environment.GetDevice()->CreateTexture(targetDescription);
        view->SetViewport({0, 0, 129, 97});
        view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      }
      const auto updateStart = xiiTime::Now();
      world.Update();
      if (phase > 1)
      {
        cpuUpdates += xiiTime::Now() - updateStart;
        ++cpuSamples;
      }
      for (const auto* packet : view->GetExtractedRenderData()->GetAllRenderData())
        if (const auto* probe = xiiDynamicCast<const xiiReflectionCaptureRenderData*>(packet))
        {
          const xiiGameObject* owner = nullptr;
          if (!XII_TEST_BOOL(world.TryGetObject(probe->m_hOwnerObject, owner)))
            return;
          XII_TEST_BOOL(probe->m_GlobalTransform.IsEqual(owner->GetGlobalTransform(), 1e-5f));
        }
      environment.GetDevice()->WaitIdle();
      if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      xiiDynamicArray<xiiVec4> hdr;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr))
        return;
      const auto      size        = target->GetDescription().m_Size;
      const xiiUInt32 centerIndex = (size.height / 2) * size.width + size.width / 2;
      center[phase]               = hdr[centerIndex];
      const auto color            = center[phase];
      XII_TEST_BOOL(color.IsValid());
      if (phase == 0 || phase == 8 || phase == 13 || phase == 23 || phase == 24 || phase == 37 || phase == 38)
      {
        XII_TEST_BOOL(!image("EnvironmentSpecular"));
        XII_TEST_BOOL(!image("ReflectionLighting"));
        XII_TEST_FLOAT(color.GetAsVec3().GetLength(), 0, 1e-6f);
      }
      else if (phase < 14 || phase == 16 || phase == 17 || phase == 20 || phase == 21)
      {
        xiiDynamicArray<xiiVec4> environmentLight, reflected;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("EnvironmentSpecular"), environmentLight) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ReflectionLighting"), reflected))
          return;
        for (xiiUInt32 i = 0; i < hdr.GetCount(); ++i)
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
          {
            XII_TEST_FLOAT(hdr[i].GetData()[channel], environmentLight[i].GetData()[channel] * (phase == 16 ? xiiMath::Exp(-0.35f) : 1.0f), phase == 16 ? 0.004f : 1e-5f);
            XII_TEST_FLOAT(reflected[i].GetData()[channel], environmentLight[i].GetData()[channel], 1e-5f);
          }
      }
      else if (phase == 22)
      {
        xiiDynamicArray<xiiVec4> specular, irradiance;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("EnvironmentSpecular"), specular) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("EnvironmentIrradiance"), irradiance))
          return;
        for (xiiUInt32 i = 0; i < hdr.GetCount(); ++i)
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
            XII_TEST_FLOAT(hdr[i].GetData()[channel], specular[i].GetData()[channel] + irradiance[i].GetData()[channel] * irradiance[i].w * 0.8f / xiiMath::Pi<float>(), 0.0001f);
        XII_TEST_FLOAT(color.x, phase == 22 ? 1.616f : 0, 0.003f);
        XII_TEST_FLOAT(color.y, 0, 0.0001f);
        XII_TEST_FLOAT(color.z, 0, 0.0001f);
      }
      else if (phase <= 27 || phase == 40 || phase == 41)
      {
        xiiDynamicArray<xiiVec4> accumulation;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("TransparentAccumulation"), accumulation))
          return;
        const auto layer = accumulation[centerIndex];
        XII_TEST_BOOL(layer.w > 0 && layer.GetAsVec3().GetLength() > 0.01f);
      }
      if (phase == 1 || phase == 3 || phase == 9 || phase == 12 || phase == 14 || phase == 20)
        XII_TEST_BOOL(color.x > color.z + color.y + 0.1f);
      if (phase == 4 || phase == 6 || phase == 15)
        XII_TEST_BOOL(color.z > color.x + color.y + 0.1f);
      if (phase == 2 || phase == 5)
      {
        XII_TEST_FLOAT(color.x, color.z, 0.02f);
        XII_TEST_BOOL(color.x > 0.1f);
      }
      if (phase == 7)
      {
        XII_TEST_FLOAT(color.x, color.y, 0.001f);
        XII_TEST_FLOAT(color.y, color.z, 0.001f);
        XII_TEST_BOOL(color.x > 0.01f);
      }
      if (phase == 10 || phase == 21)
        XII_TEST_BOOL(color.y > color.x + color.z + 0.1f);
      if (phase == 11)
      {
        XII_TEST_BOOL(color.y > 0.01f && color.x > color.y);
        XII_TEST_BOOL(color.x + color.y < center[9].x + center[9].y);
      }
      if (phase == 18 || phase == 19)
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
          XII_TEST_FLOAT(color.GetData()[channel], center[phase - 4].GetData()[channel], 0.005f);
      if (phase == 25 || phase == 26)
      {
        XII_TEST_FLOAT(color.x, center[22].x * 0.5f, 0.003f);
        XII_TEST_FLOAT(color.y, 0, 0.0001f);
        XII_TEST_FLOAT(color.z, 0, 0.0001f);
      }
      if (phase == 27)
        XII_TEST_VEC3(color.GetAsVec3(), center[26].GetAsVec3(), 0.003f);
      if (phase == 28 || phase == 36)
        XII_TEST_VEC3(color.GetAsVec3(), center[22].GetAsVec3(), 0.003f);
      if (phase == 29)
      {
        const bool firstWins = skies[0].GetInternalID().m_Data < skies[1].GetInternalID().m_Data;
        XII_TEST_VEC3(color.GetAsVec3(), firstWins ? center[28].GetAsVec3() : xiiVec3(0, 0, center[28].x), 0.003f);
      }
      if (phase == 30 || phase == 39)
        XII_TEST_VEC3(color.GetAsVec3(), xiiVec3(0, 0, center[28].x), 0.003f);
      if (phase == 31)
        XII_TEST_BOOL(color.x > color.y + color.z + 0.1f);
      if (phase == 32)
        XII_TEST_BOOL(color.y > color.x + color.z + 0.1f);
      if (phase == 33)
      {
        XII_TEST_FLOAT(color.x, color.y, 0.001f);
        XII_TEST_FLOAT(color.y, color.z, 0.001f);
        XII_TEST_BOOL(color.x > 0.01f);
      }
      if (phase == 34)
        XII_TEST_VEC3(color.GetAsVec3(), center[33].GetAsVec3() * 0.5f, 0.001f);
      if (phase == 35)
        XII_TEST_VEC3(color.GetAsVec3(), xiiVec3(center[28].x * 0.5f, 0, center[28].x * 0.5f), 0.003f);
      if (phase == 40 || phase == 41)
        XII_TEST_VEC3(color.GetAsVec3(), center[39].GetAsVec3() * 0.5f, 0.003f);
      xiiUInt32 presented;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), target.Borrow(), size.width / 2, size.height / 2, presented))
        return;
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        const float value   = color.GetData()[channel] * camera.GetExposure();
        const float linear  = xiiMath::Saturate(value * (2.51f * value + 0.03f) / (value * (2.43f * value + 0.59f) + 0.14f));
        const float encoded = linear < 0.0031308f ? linear * 12.92f : 1.055f * xiiMath::Pow(linear, 1.0f / 2.4f) - 0.055f;
        XII_TEST_FLOAT(float((presented >> (channel * 8)) & 255) / 255, encoded, 0.005f);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "%s probes phase %u: center %.4f %.4f %.4f.", editor ? "Editor" : "Scene", phase, color.x, color.y, color.z);
    }
    if (!restricted)
    {
      xiiSkyLightComponent* sky = nullptr;
      if (!XII_TEST_BOOL(world.TryGetComponent(skies[0], sky)))
        return;
      sky->SetActiveFlag(false);
      xiiReflectionCaptureComponent* original = nullptr;
      if (!XII_TEST_BOOL(world.TryGetComponent(probes[0], original)))
        return;
      original->SetActiveFlag(true);
      xiiHybridArray<xiiComponentHandle, 64> extraProbes;
      for (xiiUInt32 index = 0; index < 64; ++index)
      {
        xiiGameObjectDescription description;
        xiiGameObject*           object = nullptr;
        world.CreateObject(description, object);
        xiiReflectionCaptureComponent* probe = nullptr;
        xiiReflectionCaptureComponent::CreateComponent(object, probe);
        probe->SetReflectionMap(red);
        probe->SetHalfExtents(xiiVec3(4));
        probe->SetBlendDistance(0);
        extraProbes.PushBack(probe->GetHandle());
        if (index == 62)
        {
          world.Update();
          XII_TEST_RESULT(view->GetLastRenderResult());
        }
      }
      {
        xiiTestLogInterface   expected;
        xiiTestLogSystemScope expectedScope(&expected);
        expected.ExpectMessage("reflection probes exceed the supported descriptor capacity", xiiLogMsgType::ErrorMsg, 1);
        expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
        world.Update();
        XII_TEST_BOOL(view->GetLastRenderResult().Failed());
      }
      for (const auto handle : extraProbes)
      {
        xiiReflectionCaptureComponent* probe = nullptr;
        if (!XII_TEST_BOOL(world.TryGetComponent(handle, probe)))
          return;
        probe->SetActiveFlag(false);
      }
      world.Update();
      XII_TEST_RESULT(view->GetLastRenderResult());
    }
    if (cpuSamples)
      xiiTestFramework::Output(xiiTestOutput::Details, "Probe fixture Scene update: %.4f ms average over %u CPU samples.", cpuUpdates.GetMilliseconds() / cpuSamples, cpuSamples);
    {
      XII_LOCK(timingMutex);
      for (const auto& timing : timings)
        xiiTestFramework::Output(xiiTestOutput::Details, "%s: %.4f ms average over %u GPU samples.", timing.Key().GetData(), timing.Value().m_fMilliseconds / timing.Value().m_uiSamples, timing.Value().m_uiSamples);
    }
  }
} // namespace xiiSceneProbeTest
