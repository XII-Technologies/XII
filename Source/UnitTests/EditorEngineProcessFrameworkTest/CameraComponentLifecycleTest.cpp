/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

XII_CREATE_SIMPLE_TEST(EditorViews, CameraComponentLifecycle)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  for (bool renderModuleFirst : {false, true})
  {
    xiiWorldDescription description("Camera manager lifecycle test");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    if (renderModuleFirst)
      world.GetOrCreateModule<xiiRenderWorldModule>();
    world.GetOrCreateComponentManager<xiiCameraComponentManager>();
    auto* renderModule = world.GetModule<xiiRenderWorldModule>();
    if (!XII_TEST_BOOL(renderModule != nullptr))
      return;
    xiiView* view   = nullptr;
    auto     handle = renderModule->CreateView("Before camera manager deletion", view);
    renderModule->DestroyView(handle);
    world.DeleteComponentManager<xiiCameraComponentManager>();
    XII_TEST_BOOL(world.GetComponentManager<xiiCameraComponentManager>() == nullptr);
    XII_TEST_BOOL(renderModule->GetViewEvents().IsEmpty());
    handle = renderModule->CreateView("After camera manager deletion", view);
    renderModule->DestroyView(handle);

    // Re-registration and whole-world teardown must also remove the converted callback exactly once.
    world.GetOrCreateComponentManager<xiiCameraComponentManager>();
    XII_TEST_BOOL(!renderModule->GetViewEvents().IsEmpty());
    handle = renderModule->CreateView("After camera manager recreation", view);
    renderModule->DestroyView(handle);
  }
}
