/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Types/TagRegistry.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SphereAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Components/Lights/TubeAreaLightComponent.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>

namespace xiiRayAnalyticLightingTest
{
  inline xiiUInt32 Hash(xiiUInt32 value)
  {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    return value ^ (value >> 16);
  }
  inline float Random(xiiUInt32& state)
  {
    state = Hash(state);
    return float(state & 0x00ffffffU) / 16777216.0f;
  }

  // Independent enclosure integral: intersect the six planes and evaluate a
  // point source's inverse-square/range law and Fresnel-attenuated Lambert lobe.
  inline float PointOracle(const xiiVec3& receiver, xiiUInt32 pixel, const xiiGpuLightData& light)
  {
    float sum = 0;
    for (xiiUInt32 sample = 0; sample < 32; ++sample)
    {
      xiiUInt32     state = Hash(pixel + (sample + 1) * 0x9e3779b9U);
      const float   u = Random(state), v = Random(state);
      const float   radius = xiiMath::Sqrt(u);
      const auto    phi    = xiiAngle::MakeFromRadian(2 * xiiMath::Pi<float>() * v);
      const xiiVec3 direction(-xiiMath::Sqrt(1 - u), -radius * xiiMath::Cos(phi), radius * xiiMath::Sin(phi));
      const auto    origin   = receiver + xiiVec3(-0.02f, 0, 0);
      float         distance = 100;
      xiiVec3       normal   = xiiVec3::MakeZero();
      for (xiiUInt32 axis = 0; axis < 3; ++axis)
      {
        if (xiiMath::Abs(direction.GetData()[axis]) < 1e-8f)
          continue;
        const float plane = direction.GetData()[axis] > 0 ? 5.0f : -5.0f;
        const float t     = (plane - origin.GetData()[axis]) / direction.GetData()[axis];
        if (t > 0.001f && t < distance)
        {
          distance = t;
          normal.SetZero();
          normal.GetData()[axis] = direction.GetData()[axis] > 0 ? -1.0f : 1.0f;
        }
      }
      const auto  hit     = origin + direction * distance;
      const auto  toLight = light.m_PositionAndInvRange.GetAsVec3() - hit;
      const float d       = toLight.GetLength();
      const auto  l = toLight / d, view = -direction;
      const auto  half    = (view + l).GetNormalized();
      const float fresnel = 0.04f + 0.96f * xiiMath::Pow(xiiMath::Saturate(1 - view.Dot(half)), 5.0f);
      const float window  = xiiMath::Saturate(1 - xiiMath::Square(d / light.m_AttenuationAndSize.x));
      const float brdf    = (1 - fresnel) * 0.5f / xiiMath::Pi<float>() * xiiMath::Max(normal.Dot(l), 0.0f);
      sum += brdf * light.m_ColorAndIntensity.w * window * window / xiiMath::Max(d * d, 1e-6f);
    }
    return sum * xiiMath::Pi<float>() / 32;
  }

  inline void Run(bool editor)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(false, false)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
    {
      xiiTestFramework::Output(xiiTestOutput::Message, "Analytic ray GI requires hardware ray tracing.");
      return;
    }
    const auto scene = xiiLoadPipelineLibraryResource("Scene"), library = xiiLoadPipelineLibraryResource("SceneScreenSpaceIrradiance");
    xiiUuid    ray, gate;
    {
      xiiResourceLock<xiiRenderPipelineResource> loaded(library, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return;
      const auto& definition = loaded->GetAuthoring()->m_Descriptor;
      ray                    = xiiFindPipelineTestPass(definition, "xiiRayTracedIrradiancePass");
      for (const auto& edge : definition.m_Connections)
        if (definition.m_Passes[edge.m_uiTarget].m_Id == ray && edge.m_sInput == "Depth")
          gate = definition.m_Passes[edge.m_uiSource].m_Id;
    }
    if (!XII_TEST_BOOL(gate.IsValid()))
      return;
    constexpr xiiUInt32             count = 6;
    xiiRenderPipelineResourceHandle resources[count];
    const xiiUInt32                 budgets[count] = {0, 1, 4, 0, 4, 4};
    for (xiiUInt32 variant = 0; variant < count; ++variant)
    {
      xiiResourceLock<xiiRenderPipelineResource> original(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      auto                                       definition = original->GetAuthoring()->m_Descriptor;
      for (auto& pass : definition.m_Passes)
      {
        const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
        if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass")
          pass.m_Properties.Insert("Enabled", false);
        if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"))
        {
          auto& enable = pass.m_Overrides.ExpandAndGetRef();
          enable.m_Path.PushBack(gate);
          enable.m_Properties.Insert("Enabled", true);
          auto& settings = pass.m_Overrides.ExpandAndGetRef();
          settings.m_Path.PushBack(ray);
          settings.m_Properties.Insert("MaximumHits", 1U);
          settings.m_Properties.Insert("AnalyticLightSamples", budgets[variant]);
          settings.m_Properties.Insert("PowerWeightedLights", variant != 1 && variant != 5);
        }
      }
      xiiStringBuilder name;
      name.SetFormat("AnalyticGI.Scene.{}", variant);
      auto source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
      if (editor)
      {
        const auto                                 editorResource = xiiLoadPipelineLibraryResource("Editor");
        xiiResourceLock<xiiRenderPipelineResource> loaded(editorResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
          return;
        auto       root    = loaded->GetAuthoring()->m_Descriptor;
        const auto sceneId = original->GetAuthoring()->m_Descriptor.m_Id;
        xiiUuid    occurrence;
        for (const auto& pass : root.m_Passes)
          if (pass.m_Definition == sceneId)
            occurrence = pass.m_Id;
        if (!XII_TEST_BOOL(occurrence.IsValid()))
          return;
        for (auto& reference : root.m_References)
          if (reference.m_Id == sceneId)
            reference.m_sResource = source.GetResourceID();
        for (const xiiStringView pin : {"ScreenIrradiance", "HDRSceneColor", "Albedo"})
          root.m_Interface.PushBack({pin, false, occurrence, pin});
        name.Append(".Editor");
        source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
      }
      xiiResourceLock<xiiRenderPipelineResource> loaded(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
        return;
      xiiRenderPipelineResourceDescriptor root;
      root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
      const auto node = root.AddSubgraph("Authored transport", loaded->GetAuthoring()->m_Descriptor.m_Id);
      for (const xiiStringView pin : {"ScreenIrradiance", "HDRSceneColor", "Albedo", "SceneDepth"})
      {
        const auto sink = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
        XII_TEST_RESULT(root.Connect(node, pin, sink, "Input"));
        root.m_Interface.PushBack({pin, false, root.m_Passes[sink].m_Id, "Output"});
      }
      name.Append(".Root");
      resources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    }
    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    geometry.ComputeTangents();
    xiiMeshResourceDescriptor meshDescription;
    meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    meshDescription.AddSection(12, 0, 0);
    meshDescription.ComputeBounds();
    const auto                mesh = xiiResourceManager::CreateResource<xiiMeshResource>("AnalyticGI.Box", std::move(meshDescription));
    xiiMaterialResourceHandle materials[6];
    for (xiiUInt32 index = 0; index < 6; ++index)
    {
      xiiMaterialResourceDescriptor material;
      material.m_BaseColor = index < 3 ? xiiColor(0.5f, 0.5f, 0.5f) : xiiColor::Black;
      material.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
      if (index == 1)
        material.m_fMetallic = 1;
      if (index == 2)
        material.m_fTransmission = 0.5f;
      if (index == 4)
        material.m_FeatureFlags.Remove(xiiMaterialFeatureFlags::CastsShadows);
      if (index == 5)
      {
        material.m_AlphaMode   = xiiMaterialAlphaMode::Mask;
        material.m_BaseColor.a = 0;
      }
      xiiStringBuilder name;
      name.SetFormat("AnalyticGI.Material.{}", index);
      materials[index] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(material));
    }
    xiiIESProfileResourceDescriptor profile;
    profile.m_fMaximumCandela = 1;
    profile.m_NormalizedCandela.SetCount(2048);
    for (auto& value : profile.m_NormalizedCandela)
      value = 0.5f;
    const auto          halfIES = xiiResourceManager::CreateResource<xiiIESProfileResource>("AnalyticGI.HalfIES", std::move(profile));
    xiiWorldDescription description("Analytic diffuse transport");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    const auto addObject = [&](xiiVec3 position, xiiVec3 scale) {
      xiiGameObjectDescription object;
      object.m_LocalPosition = position;
      object.m_LocalScaling  = scale;
      object.m_bDynamic      = true;
      xiiGameObject* created = nullptr;
      world.CreateObject(object, created);
      return created;
    };
    const auto addMesh = [&](xiiVec3 position, xiiVec3 scale, xiiUInt32 material) {
      auto*             object    = addObject(position, scale);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, materials[material]);
      return component;
    };
    auto* room    = addMesh({0, 0, 0}, {10, 10, 10}, 0);
    auto* blocker = addMesh({-2, 0, 0}, {0.5f, 0.5f, 0.5f}, 3);
    blocker->SetActiveFlag(false);
    xiiPointLightComponent* points[9];
    for (auto& point : points)
    {
      auto* object = addObject({-2, 0, 0}, {1, 1, 1});
      xiiPointLightComponent::CreateComponent(object, point);
      point->SetLightColor(xiiColor::Red);
      point->SetTemperature(6500);
      point->SetIntensity(100);
      point->SetIntensityUnit(xiiPhotometricUnit::Candela);
      point->SetRange(4);
      point->SetRadius(0);
      point->SetActiveFlag(false);
    }
    const auto& excluded   = xiiTagRegistry::GetGlobalRegistry().RegisterTag("AnalyticGI.Excluded");
    auto*       blueObject = addObject({-2, 0, 0}, {1, 1, 1});
    // Tag changes use the authoritative object API rather than modifying cached packets.
    xiiTagSet blueTags;
    blueTags.Set(excluded);
    blueObject->SetTags(blueTags);
    xiiPointLightComponent* blue = nullptr;
    xiiPointLightComponent::CreateComponent(blueObject, blue);
    blue->SetLightColor(xiiColor::Blue);
    blue->SetTemperature(6500);
    blue->SetIntensity(100);
    blue->SetIntensityUnit(xiiPhotometricUnit::Candela);
    blue->SetRange(4);
    blue->SetActiveFlag(false);
    xiiDirectionalLightComponent*   sun       = nullptr;
    xiiSpotLightComponent*          spot      = nullptr;
    xiiRectangleAreaLightComponent* rectangle = nullptr;
    xiiDiscAreaLightComponent*      disc      = nullptr;
    xiiSphereAreaLightComponent*    sphere    = nullptr;
    xiiTubeAreaLightComponent*      tube      = nullptr;
    xiiDirectionalLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), sun);
    xiiSpotLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), spot);
    xiiRectangleAreaLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), rectangle);
    xiiDiscAreaLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), disc);
    xiiSphereAreaLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), sphere);
    xiiTubeAreaLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), tube);
    const auto flipped = xiiQuat::MakeShortestRotation(xiiVec3(1, 0, 0), xiiVec3(-1, 0, 0));
    sun->GetOwner()->SetLocalRotation(flipped);
    spot->GetOwner()->SetLocalRotation(flipped);
    sun->SetIntensityUnit(xiiPhotometricUnit::Lux);
    spot->SetIntensityUnit(xiiPhotometricUnit::Candela);
    spot->SetRange(4);
    spot->SetRadius(0);
    spot->SetInnerSpotAngle(xiiAngle::MakeFromDegree(90));
    spot->SetOuterSpotAngle(xiiAngle::MakeFromDegree(90));
    rectangle->SetIntensityUnit(xiiPhotometricUnit::Nit);
    rectangle->SetExtents({1, 1});
    disc->SetIntensityUnit(xiiPhotometricUnit::Nit);
    disc->SetRadius(0.3f);
    disc->SetRange(4);
    sphere->SetIntensityUnit(xiiPhotometricUnit::Nit);
    sphere->SetRadius(0.25f);
    sphere->SetRange(4);
    tube->SetIntensityUnit(xiiPhotometricUnit::Nit);
    tube->SetRadius(0.1f);
    tube->SetLength(1);
    tube->SetRange(4);
    xiiLightComponent* additional[] = {sun, spot, rectangle, disc, sphere, tube};
    for (auto* light : additional)
    {
      light->SetActiveFlag(false);
      light->SetTemperature(6500);
      light->SetLightColor(xiiColor::Red);
    }
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
    xiiView*                    views[count];
    xiiSharedPtr<xiiGALTexture> targets[count];
    for (xiiUInt32 variant = 0; variant < count; ++variant)
    {
      world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Analytic ray GI", views[variant]);
      views[variant]->SetCamera(&camera);
      views[variant]->SetCameraUsageHint(editor ? xiiCameraUsageHint::EditorView : xiiCameraUsageHint::MainView);
      views[variant]->SetPipelineParameter("Editor.SelectionOverlay", false);
      views[variant]->SetPipelineParameter("Editor.ShapeIcons", false);
      views[variant]->SetRenderPipelineResource(resources[variant]);
      views[variant]->SetViewport({0, 0, 21, 15});
      if (variant < 3 || variant == 5)
        views[variant]->m_ExcludeTags.Set(excluded);
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {21, 15};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[variant]   = environment.GetDevice()->CreateTexture(target);
      views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(views[variant]->SetDisplayOutputSettings(display));
    }
    xiiDynamicArray<xiiVec4> original, unshadowedBlocked;
    xiiDynamicArray<xiiVec4> emitterReferences[6];
    xiiMutex                 timingMutex;
    double                   traceMilliseconds   = 0;
    xiiUInt32                traceSamples        = 0;
    const xiiStringView      scanNames[]         = {"GI candidate power scan", "GI block power scan"};
    double                   scanMilliseconds[2] = {};
    xiiUInt32                scanSamples[2]      = {};
    xiiHashSet<xiiString>    traceNames;
    for (const auto& resource : resources)
    {
      xiiResourceLock<xiiRenderPipelineResource> pipeline(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      for (const auto& pass : pipeline->GetPipeline()->GetDescriptor().m_Passes)
        if (pass.m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
          traceNames.Insert(pass.m_sName);
    }
    xiiEvent<const xiiRenderGraphTimestampProfiler::TimingSample&, xiiMutex>::Unsubscriber timingSubscription;
    xiiRenderGraphTimestampProfiler::s_TimingEvents.AddEventHandler([&](const xiiRenderGraphTimestampProfiler::TimingSample& sample) {
      if (sample.m_bAvailable && !sample.m_bSubmission)
        for (xiiUInt32 scan = 0; scan < 2; ++scan)
          if (sample.m_sName.GetView() == scanNames[scan])
          {
            XII_LOCK(timingMutex);
            scanMilliseconds[scan] += sample.m_fDurationMs;
            ++scanSamples[scan];
          }
      if (sample.m_bAvailable && !sample.m_bSubmission && traceNames.Contains(xiiString(sample.m_sName.GetView())))
      {
        XII_LOCK(timingMutex);
        traceMilliseconds += sample.m_fDurationMs;
        ++traceSamples;
      }
    },
                                                                    timingSubscription);
    xiiDynamicArray<xiiPointLightComponent*> boundaryLights;
    for (xiiUInt32 phase = 0; phase < 33; ++phase)
    {
      for (xiiUInt32 i = 0; i < 9; ++i)
      {
        points[i]->SetActiveFlag(phase == 3 || phase == 4 || phase == 5 ? true : i == 0);
        points[i]->SetIntensity(phase == 1 ? 200.0f : 100.0f);
        points[i]->SetCastShadows(phase >= 6);
        points[i]->SetIESProfile(phase == 2 ? halfIES : xiiIESProfileResourceHandle());
        points[i]->SetLightColor(phase == 32 ? xiiColor::Black : xiiColor::Red);
      }
      if (phase == 11 || phase >= 13)
        for (auto* point : points)
          point->SetActiveFlag(false);
      if (phase >= 27)
        for (xiiUInt32 i = 0; i < 9; ++i)
        {
          points[i]->SetActiveFlag(phase != 30);
          points[i]->SetCastShadows(false);
          points[i]->SetIntensity(phase == 31 ? 1000.0f : phase == 29 ? 100.0f :
                                    i == (phase == 27 ? 0U : 8U)      ? 10000.0f :
                                                                        1.0f);
        }
      if (phase == 29)
        for (xiiUInt32 i = 0; i < 512; ++i)
        {
          xiiPointLightComponent* point = nullptr;
          xiiPointLightComponent::CreateComponent(addObject({-2, 0, 0}, {1, 1, 1}), point);
          point->SetLightColor(xiiColor::Red);
          point->SetTemperature(6500);
          point->SetIntensityUnit(xiiPhotometricUnit::Candela);
          point->SetIntensity(100);
          point->SetRange(4);
          boundaryLights.PushBack(point);
        }
      for (auto* point : boundaryLights)
        point->SetActiveFlag(phase == 29);
      blue->SetActiveFlag(phase == 4 || phase == 13 || phase == 31);
      blue->SetIntensity(phase == 31 ? 10.0f : 100.0f);
      blue->SetRange(phase == 31 ? 10.0f : 4.0f);
      blue->SetLightColor(phase == 31 ? xiiColorGammaUB(0, 0, 13) : xiiColorGammaUB(xiiColor::Blue));
      for (xiiUInt32 emitter = 0; emitter < 6; ++emitter)
      {
        additional[emitter]->SetActiveFlag(phase == 14 + emitter || phase == 20 + emitter || (phase == 26 && emitter == 0));
        additional[emitter]->SetIntensity(phase >= 20 && phase < 26 ? 200.0f : 100.0f);
        additional[emitter]->SetCastShadows(phase == 26);
      }
      blocker->SetActiveFlag(phase >= 6 && phase <= 9);
      blocker->SetMaterialOverride(0, materials[phase == 8 ? 4 : phase == 9 ? 5 :
                                                                              3]);
      if (phase == 7)
        points[0]->SetCastShadows(false);
      room->SetMaterialOverride(0, materials[phase == 10 ? 1 : phase == 12 ? 2 :
                                                                             0]);
      world.Update();
      environment.GetDevice()->WaitIdle();
      xiiDynamicArray<xiiVec4>   irradiance[count], hdr[count];
      xiiDynamicArray<xiiUInt32> albedo[count], presented[count];
      xiiDynamicArray<float>     depth;
      for (xiiUInt32 variant = 0; variant < count; ++variant)
      {
        if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
        xiiUInt32 scanPasses = 0;
        for (const auto& pass : views[variant]->GetRenderGraph()->GetCompiledPasses())
          if (!pass.m_bIsCulled && (pass.m_sName.GetView() == scanNames[0] || pass.m_sName.GetView() == scanNames[1]))
            ++scanPasses;
        auto&       pipelineInstance = views[variant]->GetRenderPipelineInstance();
        const auto& definition       = pipelineInstance.GetPipeline()->GetDescriptor();
        for (xiiUInt32 pass = 0; pass < definition.m_Passes.GetCount(); ++pass)
          if (definition.m_Passes[pass].m_pType->GetTypeName() == "xiiWorldLightsUploadPass")
          {
            const auto* binding     = pipelineInstance.GetOutputBinding(pass, "Candidates");
            const auto* description = binding ? views[variant]->GetRenderGraph()->GetBufferDescription(binding->m_hBuffer) : nullptr;
            if (!XII_TEST_BOOL(description != nullptr))
              return;
            const bool expected = budgets[variant] != 0 && variant != 1 && variant != 5 && description->m_uiSize > sizeof(xiiUInt32);
            XII_TEST_BOOL((scanPasses != 0) == expected);
            break;
          }
        const auto image = [&](xiiStringView pin) {
          auto& instance = views[variant]->GetRenderPipelineInstance();
          return views[variant]->GetRenderGraph()->GetExportedTexture(instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(pin, false))->m_hTexture);
        };
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ScreenIrradiance"), irradiance[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("Albedo"), albedo[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], presented[variant]))
          return;
        if (variant == 0 && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("SceneDepth"), depth))
          return;
      }
      double    energy = 0, uniformError = 0, powerError = 0, weakBlueReference = 0, weakBlueEstimate = 0;
      xiiUInt32 changed = 0;
      for (xiiUInt32 pixel = 0; pixel < irradiance[2].GetCount(); ++pixel)
      {
        XII_TEST_BOOL(irradiance[2][pixel].IsValid());
        energy += irradiance[2][pixel].x;
        changed += presented[2][pixel] != presented[0][pixel];
        for (xiiUInt32 variant = 0; variant < count; ++variant)
        {
          if (!budgets[variant])
            XII_TEST_FLOAT(irradiance[variant][pixel].GetAsVec3().GetLength(), 0, 1e-6f);
          else
          {
            const auto baseline = variant == 4 ? 3 : 0;
            for (xiiUInt32 channel = 0; channel < 3; ++channel)
            {
              const float reflectance = float((albedo[variant][pixel] >> (8 * channel)) & 255) / 255;
              XII_TEST_FLOAT(hdr[variant][pixel].GetData()[channel] - hdr[baseline][pixel].GetData()[channel], irradiance[variant][pixel].GetData()[channel] * reflectance * (phase == 12 ? 0.5f : 1.0f) / xiiMath::Pi<float>(), 5e-5f);
            }
          }
        }
        if (phase != 4 && phase < 27)
          XII_TEST_FLOAT(irradiance[1][pixel].x, irradiance[2][pixel].x, 1e-5f);
        if (phase == 1 || phase == 2 || phase == 3 || phase == 5 || phase == 9 || phase == 12)
        {
          const float factor = phase == 1 ? 2.0f : phase == 2 || phase == 12 ? 0.5f :
            phase == 3 || phase == 5                                         ? 9.0f :
                                                                               1.0f;
          XII_TEST_FLOAT(irradiance[2][pixel].x, original[pixel].x * factor, 1e-4f);
        }
        if (phase == 6 || phase == 10 || phase == 11 || phase == 13 || phase == 26)
          XII_TEST_FLOAT(irradiance[2][pixel].GetAsVec3().GetLength(), 0, 1e-6f);
        if (phase == 27 || phase == 28)
        {
          const float reference = original[pixel].x * 100.08f;
          powerError += xiiMath::Square(double(irradiance[2][pixel].x - reference));
          uniformError += xiiMath::Square(double(irradiance[5][pixel].x - reference));
        }
        if (phase == 29)
          XII_TEST_FLOAT(irradiance[2][pixel].x, original[pixel].x * 521.0f, 2e-3f);
        if (phase == 30 || phase == 32)
          XII_TEST_FLOAT(irradiance[2][pixel].GetAsVec3().GetLength(), 0, 1e-6f);
        if (phase == 31)
        {
          const auto* lights = world.GetOrCreateModule<xiiRenderWorldModule>()->GetWorldLightData();
          const auto  index  = lights->GetIndex(blue->GetHandle());
          if (!XII_TEST_BOOL(index < lights->GetLights().GetCount()))
            return;
          const auto& record    = lights->GetLights()[index];
          const auto  clip      = xiiVec4((float(pixel % 21) + 0.5f) / 21 * 2 - 1, 1 - (float(pixel / 21) + 0.5f) / 15 * 2, depth[pixel], 1);
          const auto  projected = xiiSnapshotCameraConstants(*views[4]).m_InverseViewProjectionMatrix * clip;
          weakBlueReference += PointOracle(projected.GetAsVec3() / projected.w, pixel, record) * record.m_ColorAndIntensity.z;
          weakBlueEstimate += irradiance[4][pixel].z;
          XII_TEST_FLOAT(irradiance[2][pixel].z, 0, 1e-8f);
        }
        if (phase == 13)
          XII_TEST_FLOAT(irradiance[4][pixel].z, original[pixel].x * xiiEvaluateLightColor(xiiColorLinearUB(xiiColor::Blue), 6500).b / xiiEvaluateLightColor(xiiColorLinearUB(xiiColor::Red), 6500).r, 1e-4f);
        if (phase >= 20 && phase < 26 && phase != 22)
          XII_TEST_FLOAT(irradiance[2][pixel].x, emitterReferences[phase - 20][pixel].x * 2, 1e-4f);
        if (phase == 8)
          XII_TEST_FLOAT(irradiance[2][pixel].x, unshadowedBlocked[pixel].x, 1e-5f);
        if (phase == 4)
          XII_TEST_FLOAT(irradiance[2][pixel].z, 0, 1e-6f);
        if (phase == 0)
        {
          const auto clip      = xiiVec4((float(pixel % 21) + 0.5f) / 21 * 2 - 1, 1 - (float(pixel / 21) + 0.5f) / 15 * 2, depth[pixel], 1);
          const auto projected = xiiSnapshotCameraConstants(*views[2]).m_InverseViewProjectionMatrix * clip;
          const auto position  = projected.GetAsVec3() / projected.w;
          const auto lights    = world.GetOrCreateModule<xiiRenderWorldModule>()->GetWorldLightData()->GetLights();
          if (!XII_TEST_INT(lights.GetCount(), 1))
            return;
          XII_TEST_BOOL((position - lights[0].m_PositionAndInvRange.GetAsVec3()).GetLength() > lights[0].m_AttenuationAndSize.x);
          XII_TEST_FLOAT(irradiance[2][pixel].x, PointOracle(position, pixel, lights[0]) * lights[0].m_ColorAndIntensity.x, 2e-4f);
        }
      }
      if (phase == 0)
      {
        original = irradiance[2];
      }
      if (phase == 7)
        unshadowedBlocked = irradiance[2];
      if (phase >= 14 && phase < 20)
        emitterReferences[phase - 14] = irradiance[2];
      if (phase == 0 || phase == 1 || phase == 2 || phase == 3 || phase == 4 || phase == 5 || phase == 7 || phase == 8 || phase == 9 || phase == 12)
        XII_TEST_BOOL(energy > 0.001 && changed > 0);
      if (phase >= 14 && phase < 26)
        XII_TEST_BOOL(energy > 0.001 && changed > 0);
      if (phase == 27 || phase == 28)
      {
        XII_TEST_BOOL(powerError < uniformError * 0.1);
        xiiTestFramework::Output(xiiTestOutput::Details, "Unequal-light squared error: power %.6f, uniform %.6f at equal four-light sample budget.", powerError, uniformError);
      }
      if (phase == 31)
      {
        XII_TEST_BOOL(weakBlueReference > 0 && weakBlueEstimate > weakBlueReference * 0.6 && weakBlueEstimate < weakBlueReference * 1.4);
        xiiTestFramework::Output(xiiTestOutput::Details, "Dim-blue transport: estimate %.9f, independent reference %.9f with strong red lights.", weakBlueEstimate, weakBlueReference);
      }
      if (phase == 4)
      {
        double blueEnergy = 0;
        for (const auto& value : irradiance[4])
          blueEnergy += value.z;
        XII_TEST_BOOL(blueEnergy > 0.001);
        xiiTestFramework::Output(xiiTestOutput::Details, "Included blue energy %.6f; excluded blue contribution remains zero.", blueEnergy);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "Analytic GI phase %u: energy %.6f, %u presented pixels changed.", phase, energy, changed);
    }
    for (const xiiStringView input : {"Lights", "LightCounts", "LightCandidates", "IESProfiles"})
    {
      xiiRenderPipelineResourceDescriptor missing;
      if (!XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(missing, resources[2])))
        return;
      xiiUInt32 removed = 0;
      for (xiiUInt32 i = missing.m_Connections.GetCount(); i-- > 0;)
      {
        const auto& edge = missing.m_Connections[i];
        if (edge.m_sInput == input && missing.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
        {
          XII_TEST_RESULT(missing.Disconnect(i));
          ++removed;
        }
      }
      XII_TEST_INT(removed, 1);
      xiiStringBuilder name;
      name.SetFormat("AnalyticGI.Missing.{}", input);
      auto active = missing;
      views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(active)));
      {
        xiiTestLogInterface   expected;
        xiiTestLogSystemScope scope(&expected);
        expected.ExpectMessage("Ray irradiance requires canonical light, count, view-candidate and IES records", xiiLogMsgType::ErrorMsg, 1);
        expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
        world.Update();
        XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
        XII_TEST_RESULT(views[2]->GetLastRenderResult());
      }
      for (const auto& edge : missing.m_Connections)
        if (edge.m_sInput == "Depth" && missing.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
          missing.m_Passes[edge.m_uiSource].m_Properties.Insert("Enabled", false);
      name.Append(".Disabled");
      views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(missing)));
      world.Update();
      XII_TEST_RESULT(views[0]->GetLastRenderResult());
    }
    {
      XII_LOCK(timingMutex);
      if (traceSamples)
        xiiTestFramework::Output(xiiTestOutput::Details, "21x15 analytic GI trace mean %.6f ms over %u retired GPU samples; correctness fixture only.", traceMilliseconds / traceSamples, traceSamples);
      else
        xiiTestFramework::Output(xiiTestOutput::Details, "Analytic GI timing unavailable on this configuration.");
      for (xiiUInt32 scan = 0; scan < 2; ++scan)
        if (scanSamples[scan])
          xiiTestFramework::Output(xiiTestOutput::Details, "%s mean %.6f ms over %u retired GPU samples.", scanNames[scan].GetStartPointer(), scanMilliseconds[scan] / scanSamples[scan], scanSamples[scan]);
    }
  }
} // namespace xiiRayAnalyticLightingTest
