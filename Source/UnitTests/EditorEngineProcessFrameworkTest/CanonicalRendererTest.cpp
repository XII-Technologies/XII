/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

XII_CREATE_SIMPLE_TEST_GROUP(CanonicalRenderer);
XII_CREATE_SIMPLE_TEST(CanonicalRenderer, EditorUsesScene)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto scenePipeline  = xiiLoadPipelineLibraryResource("Scene");
  const auto editorPipeline = xiiLoadPipelineLibraryResource("Editor");
  if (!XII_TEST_BOOL(scenePipeline.IsValid() && editorPipeline.IsValid()))
    return;
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("CanonicalRenderer.Mesh", std::move(meshDescription));
  xiiMaterialResourceDescriptor materialDescription;
  materialDescription.m_BaseColor = xiiColor(0.8f, 0.1f, 0.05f);
  const auto          material    = xiiResourceManager::CreateResource<xiiMaterialResource>("CanonicalRenderer.Material", std::move(materialDescription));
  xiiWorldDescription worldDescription("Canonical Editor world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectHandle owners[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGameObjectDescription description;
    description.m_LocalPosition       = xiiVec3(float(i) * 1.5f, 0, 0);
    xiiGameObject* object             = nullptr;
    owners[i]                         = world.CreateObject(description, object);
    xiiStaticMeshComponent* component = nullptr;
    xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
  }
  xiiGameObject*           lightObject = nullptr;
  xiiGameObjectDescription lightDescription;
  lightDescription.m_LocalPosition         = xiiVec3(0, 1.5f, 0.5f);
  const auto                    lightOwner = world.CreateObject(lightDescription, lightObject);
  xiiDirectionalLightComponent* light      = nullptr;
  xiiDirectionalLightComponent::CreateComponent(lightObject, light);
  light->SetIntensity(2);
  const xiiUInt8                 white[] = {255, 255, 255, 255};
  xiiGALTextureSubResourceData   iconContent(xiiConstByteBlobPtr(white, sizeof(white)), 4, 4);
  xiiTexture2DResourceDescriptor iconDescription;
  iconDescription.m_TextureDescription.m_Size   = {1, 1};
  iconDescription.m_TextureDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
  iconDescription.m_InitialContent              = xiiMakeArrayPtr(&iconContent, 1);
  const auto iconTexture                        = xiiResourceManager::CreateResource<xiiTexture2DResource>("Editor/ShapeIcons/xiiDirectionalLightComponent.dds", std::move(iconDescription));
  auto*      module                             = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera  camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-4, 0, 0}, {0, 0, 0}, {0, 0, 1});
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    module->CreateView(i ? "Canonical Scene" : "Canonical Editor", views[i]);
    views[i]->SetCamera(&camera);
    views[i]->SetCameraUsageHint(i ? xiiCameraUsageHint::MainView : xiiCameraUsageHint::EditorView);
    views[i]->SetViewport({0, 0, 64, 64});
    views[i]->SetRenderPipelineResource(i ? scenePipeline : editorPipeline);
    xiiDisplayOutputSettings display;
    display.m_Exposure.m_Mode = xiiExposureMode::Manual;
    display.m_fBloomStrength  = 0;
    XII_TEST_RESULT(views[i]->SetDisplayOutputSettings(display));
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {64, 64};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    targets[i]         = environment.GetDevice()->CreateTexture(target);
    views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  views[0]->SetPipelineParameter("Editor.SelectionOverlay", false);
  views[0]->SetPipelineParameter("Editor.ShapeIcons", false);
  const auto update = [&](xiiUInt32 count) {
    for (xiiUInt32 frame = 0; frame < count; ++frame)
    {
      world.Update();
      environment.GetDevice()->WaitIdle();
    }
  };
  const auto capture = [&]() -> const xiiSceneMeshPickingPass* {
    const auto& instance = views[0]->GetRenderPipelineInstance();
    if (const auto* pipeline = instance.GetPipeline())
      for (xiiUInt32 i = 0; i < pipeline->GetDescriptor().m_Passes.GetCount(); ++i)
        if (const auto* picking = xiiDynamicCast<const xiiSceneMeshPickingPass*>(instance.GetPass(i)))
          return picking;
    return nullptr;
  };
  update(8);
  if (!XII_TEST_RESULT(views[0]->GetLastRenderResult()) || !XII_TEST_RESULT(views[1]->GetLastRenderResult()))
    return;
  xiiUInt32 pixels[2] = {};
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[i].Borrow(), 32, 32, pixels[i]))
      return;
  XII_TEST_INT(pixels[0], pixels[1]);
  XII_TEST_BOOL((pixels[0] & 0xFFFFFF) != 0);
  const auto* picking = capture();
  if (!XII_TEST_BOOL(picking))
    return;
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_TEST_RESULT(picking->Pick(*views[0], 32, 32, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == owners[0]);
  for (xiiUInt32 selected = 0; selected < 2; ++selected)
  {
    xiiVariantArray selection;
    selection.PushBack(owners[selected].GetInternalID().m_Data);
    views[0]->SetPipelineParameter("Editor.SelectedObjects", selection);
    views[0]->SetPipelineParameter("Editor.SelectionOverlay", true);
    update(3);
    xiiUInt32 overlay = 0;
    if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[0].Borrow(), 32, 32, overlay))
      return;
    XII_TEST_BOOL(overlay != pixels[1]);
    XII_TEST_RESULT(capture()->Pick(*views[0], 32, 32, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == owners[0]);
  }
  views[0]->SetPipelineParameter("Editor.Picking", false);
  update(1);
  XII_TEST_BOOL(capture()->Pick(*views[0], 32, 32, hit).Failed());
  views[0]->SetPipelineParameter("Editor.Picking", true);
  views[0]->SetRenderPipelineResource(scenePipeline);
  update(1);
  XII_TEST_BOOL(capture() == nullptr);
  views[0]->SetRenderPipelineResource(editorPipeline);
  update(6);
  XII_TEST_RESULT(capture()->Pick(*views[0], 32, 32, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == owners[0]);

  xiiGALTextureCreationDescription resized;
  resized.m_Type      = xiiGALResourceDimension::Texture2D;
  resized.m_Size      = {96, 64};
  resized.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  resized.m_BindFlags = xiiGALBindFlags::RenderTarget;
  targets[0]          = environment.GetDevice()->CreateTexture(resized);
  views[0]->SetRenderTargetView(targets[0]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  views[0]->SetViewport({8, 12, 64, 32});
  views[0]->SetRenderScale(0.5f);
  views[0]->SetPipelineParameter("Editor.ShapeIcons", true);
  update(6);
  if (!XII_TEST_RESULT(views[0]->GetLastRenderResult()))
    return;
  XII_TEST_RESULT(capture()->Pick(*views[0], 40, 28, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == owners[0]);
  const auto&                  instance = views[0]->GetRenderPipelineInstance();
  const xiiSceneShapeIconPass* icons    = nullptr;
  for (xiiUInt32 i = 0; i < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
    if (const auto* pass = xiiDynamicCast<const xiiSceneShapeIconPass*>(instance.GetPass(i)))
      icons = pass;
  if (!XII_TEST_BOOL(icons && !icons->GetIcons().IsEmpty()))
    return;
  const auto&                          icon = icons->GetIcons()[0];
  xiiSceneShapeIconPass::PickingResult iconHit;
  XII_TEST_RESULT(icons->PickIcon(*views[0], xiiUInt32(8 + icon.m_vScreenPosition.x * 64), xiiUInt32(12 + icon.m_vScreenPosition.y * 32), iconHit));
  XII_TEST_BOOL(iconHit.m_hObject == lightOwner);

  // The native four-pane layout uses signed-depth orthographic cameras and
  // wireframe visualization. Exercise the authored Editor, including its
  // explicit camera and surface-query bindings, rather than only Scene.
  xiiCamera                   orthographic[3];
  xiiView*                    panes[3] = {};
  xiiSharedPtr<xiiGALTexture> paneTargets[3];
  views[0]->SetPipelineParameter("Editor.ShapeIcons", false);
  views[0]->SetPipelineParameter("Editor.SelectionOverlay", false);
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    orthographic[i].SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, -1000, 1000);
    orthographic[i].LookAt(i == 0 ? xiiVec3(0, 0, 5) : i == 1 ? xiiVec3(0, -5, 0) :
                                                                xiiVec3(-5, 0, 0),
                           xiiVec3::MakeZero(), i == 0 ? xiiVec3::MakeAxisY() : xiiVec3::MakeAxisZ());
    module->CreateView("Orthographic Editor pane", panes[i]);
    panes[i]->SetCamera(&orthographic[i]);
    panes[i]->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
    panes[i]->SetViewport({0, 0, 64, 64});
    panes[i]->SetViewRenderMode(xiiViewRenderMode::Wireframe);
    panes[i]->SetRenderPipelineResource(editorPipeline);
    panes[i]->SetPipelineParameter("Editor.ShapeIcons", false);
    panes[i]->SetPipelineParameter("Editor.Picking", false);
    panes[i]->SetPipelineParameter("Editor.PickSelected", false);
    panes[i]->SetPipelineParameter("Editor.SelectionOverlay", false);
    paneTargets[i] = environment.GetDevice()->CreateTexture(targets[0]->GetDescription());
    panes[i]->SetRenderTargetView(paneTargets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  update(6);
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    XII_TEST_RESULT(panes[i]->GetLastRenderResult());
    xiiUInt32 litPixels = 0;
    for (xiiUInt32 y = 30; y < 34; ++y)
      for (xiiUInt32 x = 23; x < 26; ++x)
      {
        xiiUInt32 pixel = 0;
        if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), paneTargets[i].Borrow(), x, y, pixel))
          litPixels += (pixel & 0x00FFFFFFU) != 0;
      }
    XII_TEST_BOOL(litPixels > 0);
    xiiUInt32 background = 0, grid = 0;
    XII_TEST_BOOL(xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), paneTargets[i].Borrow(), 48, 48, background));
    XII_TEST_RESULT(xiiInfiniteGridPass::ConfigureView(*panes[i], 0.25f));
    update(1);
    XII_TEST_RESULT(panes[i]->GetLastRenderResult());
    XII_TEST_BOOL(xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), paneTargets[i].Borrow(), 48, 48, grid));
    XII_TEST_BOOL(grid != background);
    module->DestroyView(panes[i]->GetHandle());
  }

  // Compare actual contact-occluded geometry through Scene and Editor, then
  // disable only Editor's nested AO occurrence through an authored override.
  world.DeleteObjectNow(owners[1]);
  xiiGameObjectDescription floorDescription;
  floorDescription.m_LocalPosition = {0, 0, -0.6f};
  floorDescription.m_LocalScaling  = {8, 8, 0.2f};
  xiiGameObject* floor             = nullptr;
  world.CreateObject(floorDescription, floor);
  xiiStaticMeshComponent* floorMesh = nullptr;
  xiiStaticMeshComponent::CreateComponent(floor, floorMesh);
  floorMesh->SetMesh(mesh);
  floorMesh->SetMaterialOverride(0, material);
  light->SetIntensity(0.1f);
  camera.LookAt({-4, -5, 2.5f}, {0, 0, -0.3f}, {0, 0, 1});
  camera.SetExposure(8);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    auto target   = targets[i]->GetDescription();
    target.m_Size = {129, 97};
    targets[i]    = environment.GetDevice()->CreateTexture(target);
    views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    views[i]->SetViewport({0, 0, 129, 97});
    views[i]->SetRenderScale(1);
  }
  update(4);
  for (auto* view : views)
    XII_TEST_RESULT(view->GetLastRenderResult());
  xiiUInt32 contact[2] = {};
  for (xiiUInt32 y = 53; y <= 55; ++y)
    for (xiiUInt32 x = 64; x <= 66; ++x)
    {
      xiiUInt32 values[2] = {};
      for (xiiUInt32 i = 0; i < 2; ++i)
        if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[i].Borrow(), x, y, values[i]))
          return;
      XII_TEST_INT(values[0], values[1]);
      if (x == 65 && y == 54)
      {
        contact[0] = values[0];
        contact[1] = values[1];
      }
    }
  xiiRenderPipelineResourceDescriptor disabledEditor;
  {
    xiiResourceLock<xiiRenderPipelineResource> scene(scenePipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> editor(editorPipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(scene.IsValid() && editor.IsValid() && scene->GetAuthoring() && editor->GetAuthoring()))
      return;
    disabledEditor       = editor->GetAuthoring()->m_Descriptor;
    xiiUInt32 overridden = 0;
    for (auto& pass : disabledEditor.m_Passes)
      if (pass.m_Definition == scene->GetAuthoring()->m_Descriptor.m_Id)
      {
        auto& override = pass.m_Overrides.ExpandAndGetRef();
        override.m_Path.PushBack(xiiFindPipelineTestPass(scene->GetAuthoring()->m_Descriptor, "xiiAmbientOcclusionPass"));
        override.m_Properties.Insert("Enabled", false);
        ++overridden;
      }
    XII_TEST_INT(overridden, 1);
  }
  views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("CanonicalRenderer.EditorWithoutAO", std::move(disabledEditor)));
  update(3);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    XII_TEST_RESULT(views[i]->GetLastRenderResult());
    xiiUInt32 value = 0;
    if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[i].Borrow(), 65, 54, value))
      return;
    if (i == 0)
      XII_TEST_BOOL((value & 255) > (contact[i] & 255) + 3);
    else
      XII_TEST_INT(value, contact[i]);
  }

  // A world-selected atmosphere reaches Editor through the same Scene HDR path.
  // Removing its component restores the original geometry and background pixels.
  views[0]->SetRenderPipelineResource(editorPipeline);
  light->SetActiveFlag(false);
  xiiGameObjectDescription skySunDescription;
  skySunDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -xiiVec3::MakeAxisZ());
  xiiGameObject* skySunObject       = nullptr;
  world.CreateObject(skySunDescription, skySunObject);
  xiiDirectionalLightComponent* skySun = nullptr;
  xiiDirectionalLightComponent::CreateComponent(skySunObject, skySun);
  skySun->SetIntensityUnit(xiiPhotometricUnit::Lux);
  skySun->SetIntensity(4);
  update(3);
  xiiUInt32 beforeAtmosphere[2][2] = {};
  for (xiiUInt32 view = 0; view < 2; ++view)
    for (xiiUInt32 sample = 0; sample < 2; ++sample)
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[view].Borrow(), sample ? 65 : 0, sample ? 54 : 0, beforeAtmosphere[view][sample]))
        return;
  xiiGameObject*             atmosphereObject = nullptr;
  const auto                 atmosphereOwner  = world.CreateObject({}, atmosphereObject);
  xiiSkyAtmosphereComponent* atmosphere       = nullptr;
  xiiSkyAtmosphereComponent::CreateComponent(atmosphereObject, atmosphere);
  update(4);
  xiiUInt32 atmospherePixels[2][2] = {};
  for (xiiUInt32 view = 0; view < 2; ++view)
  {
    XII_TEST_RESULT(views[view]->GetLastRenderResult());
    for (xiiUInt32 sample = 0; sample < 2; ++sample)
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[view].Borrow(), sample ? 65 : 0, sample ? 54 : 0, atmospherePixels[view][sample]))
        return;
    XII_TEST_BOOL(atmospherePixels[view][0] != beforeAtmosphere[view][0]);
  }
  for (xiiUInt32 sample = 0; sample < 2; ++sample)
    XII_TEST_INT(atmospherePixels[0][sample], atmospherePixels[1][sample]);
  world.DeleteObjectNow(atmosphereOwner);
  update(3);
  for (xiiUInt32 view = 0; view < 2; ++view)
    for (xiiUInt32 sample = 0; sample < 2; ++sample)
    {
      xiiUInt32 value = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), targets[view].Borrow(), sample ? 65 : 0, sample ? 54 : 0, value))
        return;
      XII_TEST_INT(value, beforeAtmosphere[view][sample]);
    }

  // World shadow intent reaches both roots before Editor overlays. Compare full
  // output images, including geometry pixels shared by Scene and Editor.
  skySun->SetActiveFlag(false);
  xiiGameObjectDescription shadowSunDescription;
  shadowSunDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.8f, -0.6f, -1).GetNormalized());
  xiiGameObject* shadowSunObject       = nullptr;
  world.CreateObject(shadowSunDescription, shadowSunObject);
  xiiDirectionalLightComponent* shadowSun = nullptr;
  xiiDirectionalLightComponent::CreateComponent(shadowSunObject, shadowSun);
  shadowSun->SetIntensityUnit(xiiPhotometricUnit::Lux);
  shadowSun->SetIntensity(0.1f);
  xiiDynamicArray<xiiUInt32> shadowImages[2][3];
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    shadowSun->SetCastShadows(phase == 1);
    update(3);
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      if (!XII_TEST_RESULT(views[view]->GetLastRenderResult()))
        return;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[view], shadowImages[view][phase]))
        return;
    }
  }
  xiiUInt32 changed[2] = {}, matchingChanges = 0;
  for (xiiUInt32 pixel = 0; pixel < shadowImages[0][0].GetCount(); ++pixel)
  {
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      XII_TEST_INT(shadowImages[view][0][pixel], shadowImages[view][2][pixel]);
      changed[view] += shadowImages[view][0][pixel] != shadowImages[view][1][pixel];
    }
    matchingChanges += shadowImages[1][0][pixel] != shadowImages[1][1][pixel] && shadowImages[0][0][pixel] == shadowImages[1][0][pixel] && shadowImages[0][1][pixel] == shadowImages[1][1][pixel];
  }
  XII_TEST_BOOL(changed[0] > 8 && changed[1] > 8 && matchingChanges > 8);
}
