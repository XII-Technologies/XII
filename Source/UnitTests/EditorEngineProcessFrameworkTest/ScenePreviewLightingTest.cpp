/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EditorEngineProcessFramework/Rendering/ScenePreviewLighting.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  template <typename Component>
  Component* FindPreviewComponent(xiiWorld& world)
  {
    auto* pManager = world.GetComponentManager<typename Component::ComponentManagerType>();
    if (pManager != nullptr)
    {
      for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
      {
        if (!it->GetHandle().IsInvalidated() && it->GetOwner()->GetName().StartsWith("Editor Preview"))
          return it;
      }
    }
    return nullptr;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, ScenePreviewLighting)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Scene preview lighting test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiScenePreviewLighting lighting(world);
  const auto&             editorTag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty worlds defer creation, thumbnails can force it")
  {
    XII_TEST_RESULT(lighting.EnsureCreated(true));
    XII_TEST_INT(world.GetObjectCount(), 0);
    XII_TEST_RESULT(lighting.EnsureCreated(false, true));
    XII_TEST_INT(world.GetObjectCount(), 2);
    auto* sky   = FindPreviewComponent<xiiSkyAtmosphereComponent>(world);
    auto* light = FindPreviewComponent<xiiDirectionalLightComponent>(world);
    if (!XII_TEST_BOOL(sky != nullptr && light != nullptr))
      return;
    XII_TEST_INT(sky->GetPriority(), xiiMath::MinValue<xiiInt32>());
    XII_TEST_FLOAT(light->GetIntensity(), 10.0f, 0.0f);
    XII_TEST_BOOL(light->GetIntensityUnit() == xiiPhotometricUnit::Lux);
    XII_TEST_BOOL(!sky->GetOwner()->GetTags().IsSet(editorTag));
    XII_TEST_BOOL(!light->GetOwner()->GetTags().IsSet(editorTag));
    const auto hSky   = sky->GetOwner()->GetHandle();
    const auto hLight = light->GetOwner()->GetHandle();
    XII_TEST_RESULT(lighting.EnsureCreated(true));
    XII_TEST_INT(world.GetObjectCount(), 2);
    XII_TEST_BOOL(sky->GetOwner()->GetHandle() == hSky);
    XII_TEST_BOOL(light->GetOwner()->GetHandle() == hLight);
    XII_TEST_BOOL(sky->GetOwner()->GetTags().IsSet(editorTag));
    XII_TEST_BOOL(light->GetOwner()->GetTags().IsSet(editorTag));

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         stream(&storage);
    xiiWorldWriter                writer;
    xiiTagSet                     excluded;
    excluded.Set(editorTag);
    writer.WriteWorld(stream, world, &excluded);
    XII_TEST_INT(writer.GetAllWrittenRootObjects().GetCount(), 0);

    XII_TEST_RESULT(lighting.EnsureCreated(false));
    lighting.Clear();
    XII_TEST_INT(world.GetObjectCount(), 0);
    storage.Clear();
    stream.SetWritePosition(0);
    writer.WriteWorld(stream, world);
    XII_TEST_INT(writer.GetAllWrittenRootObjects().GetCount(), 0);
    lighting.Clear();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Physical presets, isolated view state and transactional validation")
  {
    xiiView*   first        = nullptr;
    xiiView*   second       = nullptr;
    auto*      module       = world.GetOrCreateModule<xiiRenderWorldModule>();
    const auto firstHandle  = module->CreateView("Material inspection", first);
    const auto secondHandle = module->CreateView("Unchanged view", second);
    xiiCamera  camera;
    const auto originalDisplay = second->GetDisplayOutputSettings();
    for (const auto preset : {xiiPreviewLightingSettings::Preset::NeutralStudio, xiiPreviewLightingSettings::Preset::OutdoorDaylight, xiiPreviewLightingSettings::Preset::LowLightStudio, xiiPreviewLightingSettings::Preset::Diagnostic})
    {
      auto settings = xiiPreviewLightingSettings::MakePreset(preset);
      XII_TEST_BOOL(settings.IsValid());
      xiiPreviewLightingSettings restored;
      XII_TEST_RESULT(xiiPreviewLightingSettings::FromVariant(settings.ToVariant(), restored));
      XII_TEST_BOOL(restored.ToVariant() == settings.ToVariant());
      XII_TEST_RESULT(lighting.EnsureCreated(true, true));
      XII_TEST_RESULT(lighting.SetSettings(settings));
      auto* light = FindPreviewComponent<xiiDirectionalLightComponent>(world);
      XII_TEST_FLOAT(light->GetIntensity(), settings.m_fIlluminance, 0);
      XII_TEST_BOOL(light->GetIntensityUnit() == xiiPhotometricUnit::Lux);
      XII_TEST_BOOL(light->GetCastShadows() == settings.m_bShadows);
      XII_TEST_RESULT(settings.ApplyToView(*first, camera));
      XII_TEST_FLOAT(camera.GetExposure(), settings.m_fExposure, 1e-6f);
      XII_TEST_FLOAT(second->GetDisplayOutputSettings().m_Exposure.m_fExposureCompensation, originalDisplay.m_Exposure.m_fExposureCompensation, 0);
      settings.m_fIlluminance = xiiMath::NaN<float>();
      XII_TEST_BOOL(lighting.SetSettings(settings).Failed());
      XII_TEST_BOOL(settings.ApplyToView(*first, camera).Failed());
    }
    xiiPreviewLightingSettings destination;
    const auto                 unchanged = destination.ToVariant();
    XII_TEST_BOOL(xiiPreviewLightingSettings::FromVariant(xiiVariantDictionary(), destination).Failed());
    XII_TEST_BOOL(destination.ToVariant() == unchanged);
    lighting.Clear();
    module->DestroyView(firstHandle);
    module->DestroyView(secondHandle);
  }

  xiiGameObject*             authoredObject  = nullptr;
  const auto                 hAuthoredObject = world.CreateObject(xiiGameObjectDescription(), authoredObject);
  xiiSkyAtmosphereComponent* authoredSky     = nullptr;
  const auto                 hAuthoredSky    = xiiSkyAtmosphereComponent::CreateComponent(authoredObject, authoredSky);
  authoredSky->SetPriority(-100);
  XII_TEST_RESULT(lighting.EnsureCreated(true));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "External component and object deletion are repaired without duplicates")
  {
    auto*      sky             = FindPreviewComponent<xiiSkyAtmosphereComponent>(world);
    auto*      light           = FindPreviewComponent<xiiDirectionalLightComponent>(world);
    const auto hOldSkyObject   = sky->GetOwner()->GetHandle();
    const auto hOldLightObject = light->GetOwner()->GetHandle();
    sky->GetOwningManager()->DeleteComponent(sky->GetHandle());
    world.DeleteObjectNow(hOldLightObject);
    XII_TEST_RESULT(lighting.EnsureCreated(true));
    XII_TEST_INT(world.GetObjectCount(), 3);
    XII_TEST_BOOL(FindPreviewComponent<xiiSkyAtmosphereComponent>(world)->GetOwner()->GetHandle() != hOldSkyObject);
    XII_TEST_BOOL(FindPreviewComponent<xiiDirectionalLightComponent>(world)->GetOwner()->GetHandle() != hOldLightObject);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native extraction, authored priority and per-view export tags")
  {
    auto*                            module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiGALTextureCreationDescription targetDescription;
    targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    targetDescription.m_Size      = {16, 16};
    targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
    if (!XII_TEST_BOOL(target != nullptr))
      return;
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    xiiView *  editorView = nullptr, *gameView = nullptr;
    const auto hEditorView = module->CreateView("Editor preview lighting", editorView);
    const auto hGameView   = module->CreateView("No editor helpers", gameView);
    struct ClearPassData
    {
      xiiRenderGraphTextureHandle m_Target;
    };
    for (auto* view : {editorView, gameView})
    {
      view->SetCamera(&camera);
      view->SetViewport(xiiRectFloat(0, 0, 16, 16));
      view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      view->SetRenderEnabled(true);
      // Exercise world extraction and graph execution independently of scene shading.
      view->SetRenderGraphBuilder([&target](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
        graph.AddPass<ClearPassData>("Preview lighting test output", xiiGALCommandQueueFlags::Graphics, [&target](ClearPassData& data, xiiRenderGraphBuilder& builder) { data.m_Target = builder.WriteTexture(builder.ImportTexture("Preview lighting target", target, target->GetResourceState()), xiiGALResourceStateFlags::RenderTarget); }, [](const ClearPassData& data, xiiRenderGraphPassContext& context) { context.GetCommandList().ClearRenderTargetView(context.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); }, true);
      });
    }
    gameView->m_ExcludeTags.Set(editorTag);
    world.Update();
    XII_TEST_INT(editorView->GetExtractedRenderData()->GetAllRenderData().GetCount(), 3);
    XII_TEST_INT(gameView->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    const xiiSkyAtmosphereRenderData* bestSky = nullptr;
    xiiUInt32                         lights  = 0;
    for (auto* data : editorView->GetExtractedRenderData()->GetAllRenderData())
    {
      if (auto* sky = xiiDynamicCast<xiiSkyAtmosphereRenderData*>(data))
      {
        if (bestSky == nullptr || sky->m_iPriority > bestSky->m_iPriority)
          bestSky = sky;
      }
      if (auto* light = xiiDynamicCast<xiiDirectionalLightRenderData*>(data))
      {
        ++lights;
        XII_TEST_FLOAT(light->m_fPhotometricIntensity, 10.0f, 0.001f);
      }
    }
    XII_TEST_INT(lights, 1);
    if (XII_TEST_BOOL(bestSky != nullptr))
      XII_TEST_BOOL(bestSky->m_hOwnerComponent == hAuthoredSky);
    module->DestroyView(hGameView);
    module->DestroyView(hEditorView);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cleanup preserves authored objects, world clears reject stale handles")
  {
    lighting.Clear();
    XII_TEST_INT(world.GetObjectCount(), 1);
    XII_TEST_BOOL(world.TryGetObject(hAuthoredObject, authoredObject));
    XII_TEST_BOOL(world.TryGetComponent(hAuthoredSky, authoredSky));
    XII_TEST_RESULT(lighting.EnsureCreated(true));
    world.Clear();
    XII_TEST_INT(world.GetObjectCount(), 0);
    XII_TEST_RESULT(lighting.EnsureCreated(true, true));
    XII_TEST_INT(world.GetObjectCount(), 2);
    lighting.Clear();
    {
      xiiScenePreviewLighting temporary(world);
      XII_TEST_RESULT(temporary.EnsureCreated(false, true));
    }
    XII_TEST_INT(world.GetObjectCount(), 0);
  }
}
