/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
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
}
