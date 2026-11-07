/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

namespace xiiRayEmissiveLightingTest
{
  // Independent midpoint integration over an off-camera 0.5 by 0.5 metre panel.
  inline float PanelOracle(const xiiVec3& receiver, bool stripe = false)
  {
    const auto   origin   = receiver - xiiVec3(0.02f, 0, 0);
    const double distance = origin.x + 4.0;
    const double halfZ    = stripe ? 0.03125 : 0.25;
    double       result   = 0;
    for (xiiUInt32 y = 0; y < 64; ++y)
      for (xiiUInt32 z = 0; z < 64; ++z)
      {
        const double dy      = (double(y) + 0.5) / 128.0 - 0.25 - origin.y;
        const double dz      = (double(z) + 0.5) / 64.0 * (2 * halfZ) - halfZ - origin.z;
        const double squared = distance * distance + dy * dy + dz * dz;
        result += distance * distance / (squared * squared);
      }
    return float(result * (100.0 * halfZ / 4096.0));
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
      xiiTestFramework::Output(xiiTestOutput::Message, "Emissive ray GI requires hardware ray tracing.");
      return;
    }
    const auto settings = xiiEmissiveLightManager::GetConfiguration();
    XII_SCOPE_EXIT(xiiEmissiveLightManager::Configure(settings).AssertSuccess());
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
    constexpr xiiUInt32             count              = 8;
    const xiiUInt32                 pathSamples[count] = {4, 32, 4, 4, 32, 4, 4, 4}, emissiveSamples[count] = {0, 0, 4, 0, 0, 4, 4, 8}, hits[count] = {1, 1, 1, 1, 2, 2, 4, 1};
    xiiRenderPipelineResourceHandle resources[count];
    for (xiiUInt32 variant = 0; variant < count; ++variant)
    {
      xiiResourceLock<xiiRenderPipelineResource> original(scene, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      auto                                       definition = original->GetAuthoring()->m_Descriptor;
      if (!XII_TEST_RESULT(xiiSceneIrradianceTest::UseDeterministicTransport(definition)))
        return;
      for (auto& pass : definition.m_Passes)
      {
        const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
        if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass")
          pass.m_Properties.Insert("Enabled", false);
        if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance"))
        {
          auto& enable = pass.m_Overrides.ExpandAndGetRef();
          enable.m_Path.PushBack(gate);
          enable.m_Properties.Insert("Enabled", variant != 0);
          auto& budget = pass.m_Overrides.ExpandAndGetRef();
          budget.m_Path.PushBack(ray);
          budget.m_Properties.Insert("SampleCount", pathSamples[variant]);
          budget.m_Properties.Insert("MaximumHits", hits[variant]);
          budget.m_Properties.Insert("EmissiveLightSamples", emissiveSamples[variant]);
          budget.m_Properties.Insert("AnalyticLightSamples", 0U);
          budget.m_Properties.Insert("RayLength", 30.0f);
        }
      }
      xiiStringBuilder name;
      name.SetFormat("EmissiveMIS.Scene.{}", variant);
      auto source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
      if (editor)
      {
        xiiResourceLock<xiiRenderPipelineResource> loaded(xiiLoadPipelineLibraryResource("Editor"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        auto                                       editorDefinition = loaded->GetAuthoring()->m_Descriptor;
        const auto                                 sceneId          = original->GetAuthoring()->m_Descriptor.m_Id;
        xiiUuid                                    occurrence;
        for (const auto& pass : editorDefinition.m_Passes)
          if (pass.m_Definition == sceneId)
            occurrence = pass.m_Id;
        if (!XII_TEST_BOOL(occurrence.IsValid()))
          return;
        for (auto& reference : editorDefinition.m_References)
          if (reference.m_Id == sceneId)
            reference.m_sResource = source.GetResourceID();
        for (const xiiStringView pin : {"HDRSceneColor", "Albedo", "ScreenIrradiance"})
          editorDefinition.m_Interface.PushBack({pin, false, occurrence, pin});
        name.Append(".Editor");
        source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(editorDefinition));
      }
      xiiResourceLock<xiiRenderPipelineResource> loaded(source, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      xiiRenderPipelineResourceDescriptor        root;
      root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
      const auto node = root.AddSubgraph("Authored GI", loaded->GetAuthoring()->m_Descriptor.m_Id);
      for (const xiiStringView pin : {"HDRSceneColor", "Albedo", "SceneDepth", "ScreenIrradiance"})
      {
        const auto sink = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
        XII_TEST_RESULT(root.Connect(node, pin, sink, "Input"));
        root.m_Interface.PushBack({pin, false, root.m_Passes[sink].m_Id, "Output"});
      }
      name.Append(".Root");
      resources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    }
    const auto mesh = [](xiiStringView name, bool panel) {
      xiiGeometry geometry;
      if (panel)
      {
        xiiGeometry::GeoOptions options;
        options.m_MainAxis = xiiBasisAxis::PositiveX;
        geometry.AddRect(xiiVec2(0.5f), 1, 1, options);
      }
      else
        geometry.AddBox(xiiVec3(10), true);
      geometry.ComputeTangents();
      xiiMeshResourceDescriptor description;
      description.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
      description.AddSection(panel ? 2 : 12, 0, 0);
      description.ComputeBounds();
      return xiiResourceManager::CreateResource<xiiMeshResource>(name, std::move(description));
    };
    const auto roomMesh = mesh("EmissiveMIS.Room", false), panelMesh = mesh("EmissiveMIS.Panel", true);
    const auto material = [](xiiStringView name, float base, float emission, bool masked = false, bool castsShadows = true) {
      xiiMaterialResourceDescriptor description;
      description.m_BaseColor     = xiiColor(base, base, base, masked ? 0.0f : 1.0f);
      description.m_EmissiveColor = xiiColor(emission, emission, emission);
      description.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
      if (masked)
        description.m_AlphaMode = xiiMaterialAlphaMode::Mask;
      if (!castsShadows)
        description.m_FeatureFlags.Remove(xiiMaterialFeatureFlags::CastsShadows);
      return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(description));
    };
    const auto glowing = material("EmissiveMIS.ConstantEnclosure", 0, 1), diffuseGlowing = material("EmissiveMIS.DiffuseEnclosure", 0.5f, 1), black = material("EmissiveMIS.BlackEnclosure", 0.5f, 0);
    const auto bright = material("EmissiveMIS.BrightPanel", 0, 100), doubled = material("EmissiveMIS.DoublePanel", 0, 200), cutout = material("EmissiveMIS.CutoutPanel", 0, 100, true);
    const auto texturedMaterial = [](xiiStringView name, bool stripe) {
      xiiColor pixels[16];
      for (xiiUInt32 index = 0; index < 16; ++index)
        pixels[index] = stripe ? (index == 7 || index == 8 ? xiiColor::White : xiiColor::Black) : xiiColor(0.25f, 0.5f, 0.75f);
      xiiGALTextureSubResourceData content;
      content.m_pData    = xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(pixels), sizeof(pixels));
      content.m_uiStride = content.m_uiDepthStride = sizeof(pixels);
      xiiTexture2DResourceDescriptor texture;
      texture.m_TextureDescription.m_Size        = {16, 1};
      texture.m_TextureDescription.m_uiMipLevels = 1;
      texture.m_TextureDescription.m_Format      = xiiGALResourceFormat::RGBA32Float;
      texture.m_InitialContent                   = xiiMakeArrayPtr(&content, 1);
      texture.m_SamplerDescription.m_MinFilter   = xiiGALFilterType::Point;
      texture.m_SamplerDescription.m_MagFilter   = xiiGALFilterType::Point;
      texture.m_SamplerDescription.m_MipFilter   = xiiGALFilterType::Point;
      texture.m_SamplerDescription.m_AddressU    = xiiGALTextureAddressMode::Clamp;
      texture.m_SamplerDescription.m_AddressV    = xiiGALTextureAddressMode::Clamp;
      xiiStringBuilder textureName(name);
      textureName.Append(".Texture");
      xiiMaterialResourceDescriptor description;
      description.m_BaseColor     = xiiColor::Black;
      description.m_EmissiveColor = xiiColor(100, 100, 100);
      description.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
      description.m_hEmissiveTexture = xiiResourceManager::CreateResource<xiiTexture2DResource>(textureName, std::move(texture));
      return xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(description));
    };
    const auto          colored = texturedMaterial("EmissiveMIS.ColoredPanel", false), striped = texturedMaterial("EmissiveMIS.StripedPanel", true);
    const auto          blockerMaterial = material("EmissiveMIS.NoncastingBlocker", 0, 0, false, false);
    xiiWorldDescription description("Emissive MIS acceptance");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    const auto add = [&](xiiVec3 position, xiiVec3 scale, const xiiMeshResourceHandle& shape, const xiiMaterialResourceHandle& surface) {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_bDynamic      = true;
      objectDescription.m_LocalPosition = position;
      objectDescription.m_LocalScaling  = scale;
      xiiGameObject* object             = nullptr;
      world.CreateObject(objectDescription, object);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(object, component);
      component->SetMesh(shape);
      component->SetMaterialOverride(0, surface);
      return component;
    };
    auto*     room    = add(xiiVec3::MakeZero(), xiiVec3(1), roomMesh, glowing);
    auto*     panel   = add(xiiVec3(-4, 0, 0), xiiVec3(1), panelMesh, bright);
    auto*     blocker = add(xiiVec3(-2, 0, 0), xiiVec3(20), panelMesh, blockerMaterial);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
    xiiView*                    views[count];
    xiiSharedPtr<xiiGALTexture> targets[count];
    for (xiiUInt32 variant = 0; variant < count; ++variant)
    {
      world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Emissive MIS comparison", views[variant]);
      views[variant]->SetCamera(&camera);
      views[variant]->SetRenderPipelineResource(resources[variant]);
      views[variant]->SetViewport({0, 0, 17, 13});
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {17, 13};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[variant]   = environment.GetDevice()->CreateTexture(target);
      views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
    xiiDynamicArray<xiiVec4> initial[count], panelReference[count];
    for (xiiUInt32 phase = 0; phase < 13; ++phase)
    {
      auto configuration = settings;
      if (phase == 10)
        configuration.m_uiMaxTriangles = 1;
      XII_TEST_RESULT(xiiEmissiveLightManager::Configure(configuration));
      room->SetActiveFlag(phase != 3);
      room->SetMaterialOverride(0, phase == 1 ? diffuseGlowing : phase <= 4 && phase != 2 ? glowing :
                                                                                            black);
      panel->SetActiveFlag(phase >= 5);
      panel->SetMaterialOverride(0, phase == 6 ? doubled : phase == 7 ? cutout :
                                   phase == 11                        ? colored :
                                   phase == 12                        ? striped :
                                                                        bright);
      blocker->SetActiveFlag(phase == 8);
      world.Update();
      environment.GetDevice()->WaitIdle();
      xiiDynamicArray<xiiVec4>   irradiance[count], hdr[count];
      xiiDynamicArray<xiiUInt32> albedo[count];
      xiiDynamicArray<float>     depth;
      double                     means[count] = {};
      for (xiiUInt32 variant = 0; variant < count; ++variant)
      {
        if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
          return;
        xiiUInt32 emissivePasses = 0;
        for (const auto& compiled : views[variant]->GetRenderGraph()->GetCompiledPasses())
          if (!compiled.m_bIsCulled && (compiled.m_sName.GetView() == "GPU Emissive Triangle Build" || compiled.m_sName.GetView() == "GPU Emissive Alias Block Build" || compiled.m_sName.GetView() == "GPU Emissive Alias Global Build"))
            ++emissivePasses;
        XII_TEST_INT(emissivePasses, emissiveSamples[variant] != 0 && phase != 3 ? 3 : 0);
        const auto image = [&](xiiStringView name) {
          auto& instance = views[variant]->GetRenderPipelineInstance();
          return views[variant]->GetRenderGraph()->GetExportedTexture(instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false))->m_hTexture);
        };
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("HDRSceneColor"), hdr[variant]) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("Albedo"), albedo[variant]))
          return;
        if (variant == 0)
        {
          XII_TEST_BOOL(image("ScreenIrradiance") == nullptr);
          continue;
        }
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("ScreenIrradiance"), irradiance[variant]))
          return;
        if (variant == 2 && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image("SceneDepth"), depth))
          return;
        if (phase == 0)
          initial[variant] = irradiance[variant];
        if (phase == 5)
          panelReference[variant] = irradiance[variant];
        if (phase == 4)
        {
          float maximumDifference = 0;
          for (xiiUInt32 pixel = 0; pixel < irradiance[variant].GetCount(); ++pixel)
            for (xiiUInt32 channel = 0; channel < 4; ++channel)
              maximumDifference = xiiMath::Max(maximumDifference, xiiMath::Abs(irradiance[variant][pixel].GetData()[channel] - initial[variant][pixel].GetData()[channel]));
          XII_TEST_BOOL_MSG(irradiance[variant] == initial[variant], "Restored enclosure variant %u differs by at most %.9g", variant, maximumDifference);
        }
        if (phase == 9)
          XII_TEST_BOOL(irradiance[variant] == panelReference[variant]);
        for (xiiUInt32 pixel = 0; pixel < irradiance[variant].GetCount(); ++pixel)
        {
          const auto value = irradiance[variant][pixel];
          XII_TEST_BOOL(value.IsValid());
          means[variant] += value.x / double(irradiance[variant].GetCount());
          if (phase == 2 || phase == 3 || phase == 7 || phase == 8)
            XII_TEST_FLOAT(value.GetAsVec3().GetLength(), 0, 1e-6f);
          if (phase == 6)
            XII_TEST_VEC3(value.GetAsVec3(), panelReference[variant][pixel].GetAsVec3() * 2.0f, 1e-4f);
          if (phase == 11)
            XII_TEST_VEC3(value.GetAsVec3(), panelReference[variant][pixel].GetAsVec3().CompMul(xiiVec3(0.25f, 0.5f, 0.75f)), 1e-4f);
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
          {
            const float reflectance = float((albedo[variant][pixel] >> (channel * 8)) & 255) / 255;
            XII_TEST_FLOAT(hdr[variant][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], value.GetData()[channel] * reflectance / xiiMath::Pi<float>(), 2e-4f);
          }
        }
      }
      if (phase == 0 || phase == 4)
        for (xiiUInt32 variant = 1; variant < count; ++variant)
          XII_TEST_FLOAT(means[variant], xiiMath::Pi<double>(), 0.18);
      if (phase == 1)
      {
        XII_TEST_FLOAT(means[2], means[1], 0.18);
        XII_TEST_FLOAT(means[5], means[4], 0.25);
        XII_TEST_BOOL(means[5] > means[2] * 1.25);
        XII_TEST_BOOL(means[6] > means[5] * 1.10 && means[6] < means[5] * 1.35);
      }
      if (phase == 5 || phase == 12)
      {
        double     squared[2] = {}, referenceMean = 0;
        const auto inverse = xiiSnapshotCameraConstants(*views[2]).m_InverseViewProjectionMatrix;
        for (xiiUInt32 pixel = 0; pixel < depth.GetCount(); ++pixel)
        {
          const auto clip      = xiiVec4((float(pixel % 17) + 0.5f) / 17 * 2 - 1, 1 - (float(pixel / 17) + 0.5f) / 13 * 2, depth[pixel], 1);
          const auto projected = inverse * clip;
          const auto receiver  = projected.GetAsVec3() / projected.w;
          XII_TEST_FLOAT(receiver.x, 5.0f, 0.001f);
          const float reference = PanelOracle(receiver, phase == 12);
          referenceMean += reference / depth.GetCount();
          squared[0] += xiiMath::Square(double(irradiance[1][pixel].x - reference));
          squared[1] += xiiMath::Square(double(irradiance[2][pixel].x - reference));
        }
        XII_TEST_FLOAT(means[2], referenceMean, referenceMean * (phase == 12 ? 0.10 : 0.01));
        XII_TEST_BOOL(squared[1] < squared[0] * 0.1);
        xiiTestFramework::Output(xiiTestOutput::Details, "Emissive panel: reference %.8f, MIS %.8f, BSDF/MIS squared error %.8f / %.8f", referenceMean, means[2], squared[0], squared[1]);
      }
      if (phase == 10)
      {
        XII_TEST_BOOL(irradiance[2] == irradiance[3]);
        XII_TEST_BOOL(irradiance[7] == irradiance[3]);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "Emissive phase %u: BSDF/MIS one-hit %.6f / %.6f, two-hit %.6f / %.6f, four-hit %.6f", phase, means[1], means[2], means[4], means[5], means[6]);
    }
    for (const xiiStringView input : {"EmissiveTriangles", "EmissiveHeader", "TriangleAliases", "BlockAliases", "AliasHeader"})
    {
      xiiRenderPipelineResourceDescriptor missing;
      if (!XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(missing, resources[2])))
        return;
      xiiUInt32 removed = 0;
      for (xiiUInt32 index = missing.m_Connections.GetCount(); index-- > 0;)
      {
        const auto& edge = missing.m_Connections[index];
        if (edge.m_sInput == input && missing.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
        {
          XII_TEST_RESULT(missing.Disconnect(index));
          ++removed;
        }
      }
      XII_TEST_INT(removed, 1);
      xiiStringBuilder name;
      name.SetFormat("EmissiveMIS.Missing.{}", input);
      auto active = missing;
      views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(active)));
      {
        xiiTestLogInterface   expected;
        xiiTestLogSystemScope scope(&expected);
        expected.ExpectMessage("Ray irradiance requires", xiiLogMsgType::ErrorMsg, 1);
        expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
        world.Update();
        XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
        XII_TEST_RESULT(views[2]->GetLastRenderResult());
      }
      auto disabledSampling = missing;
      for (auto& pass : disabledSampling.m_Passes)
        if (pass.m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
          pass.m_Properties.Insert("EmissiveLightSamples", 0U);
      name.Append(".Unsampled");
      views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(disabledSampling)));
      world.Update();
      XII_TEST_RESULT(views[0]->GetLastRenderResult());
      for (const auto& edge : missing.m_Connections)
        if (edge.m_sInput == "Depth" && missing.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
          missing.m_Passes[edge.m_uiSource].m_Properties.Insert("Enabled", false);
      name.Append(".Gated");
      views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(missing)));
      world.Update();
      XII_TEST_RESULT(views[0]->GetLastRenderResult());
    }
  }
} // namespace xiiRayEmissiveLightingTest
