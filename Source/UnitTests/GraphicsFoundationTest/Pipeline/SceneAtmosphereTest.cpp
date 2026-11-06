/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/SceneAtmosphereTestUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneAtmosphere);
XII_CREATE_SIMPLE_TEST(SceneAtmosphere, CanonicalSkyAerialPerspectiveOptionalWorldsAndReload) { xiiSceneAtmosphereTest::Run(false); }

XII_CREATE_SIMPLE_TEST(SceneAtmosphere, MissingLookupFailsAndAbsentAtmospherePreservesInputFormat)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Atmosphere failure boundary");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view   = nullptr;
  module->CreateView("Atmosphere failure view", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 8, 8});
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Atmosphere failure graph")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  for (xiiUInt32 disabled = 0; disabled < 2; ++disabled)
  {
    xiiRenderPipelineResourceDescriptor definition;
    const auto                          source         = definition.AddPass("Surface", xiiGetStaticRTTI<xiiScreenEffectTestSource>());
    const auto                          frameConstants = definition.AddPass("Camera", xiiGetStaticRTTI<xiiViewConstantsPass>());
    const auto                          light          = definition.AddPass("Light", xiiGetStaticRTTI<xiiEnvironmentLightConstantsPass>());
    const auto                          constants      = definition.AddPass("Physical parameters", xiiGetStaticRTTI<xiiAtmosphereConstantsPass>());
    definition.m_Passes[constants].m_Properties.Insert("Enabled", disabled == 0);
    const auto apply   = definition.AddPass("Atmospheric application", xiiGetStaticRTTI<xiiAtmosphereApplyPass>());
    const auto capture = definition.AddPass("Capture", xiiGetStaticRTTI<xiiOpaqueTestExport>());
    XII_TEST_RESULT(definition.Connect(source, "Depth", apply, "Depth"));
    XII_TEST_RESULT(definition.Connect(source, "Color", apply, "Radiance"));
    XII_TEST_RESULT(definition.Connect(frameConstants, "PerFrameCamera", apply, "Camera"));
    XII_TEST_RESULT(definition.Connect(light, "EnvironmentLight", apply, "EnvironmentLight"));
    XII_TEST_RESULT(definition.Connect(constants, "AtmosphereConstants", apply, "Atmosphere"));
    XII_TEST_RESULT(definition.Connect(apply, "AtmosphereRadiance", capture, "Input"));
    definition.m_Interface.PushBack({"Result", false, definition.m_Passes[capture].m_Id, "Output"});
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(disabled ? "SceneAtmosphere.Absent" : "SceneAtmosphere.MissingLookup", std::move(definition));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    graph.BeginSetup(frame.GetFrameIndex());
    const auto built = instance.Build(view, graph, context.GetBlackboard());
    if (!disabled)
    {
      XII_TEST_BOOL(built.Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("LUTs") != nullptr);
      graph.EndSetup();
      continue;
    }
    if (!XII_TEST_RESULT_MSG(built, "%s", instance.GetLastBuildError().GetStartPointer()))
      return;
    const auto output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Result", false))->m_hTexture;
    graph.EndSetup();
    instance.Clear();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    environment.GetDevice()->WaitIdle();
    const auto texture = graph.GetExportedTexture(output);
    if (!XII_TEST_BOOL(texture && texture->GetDescription().m_Format == xiiGALResourceFormat::RGBA16Float))
      return;
    struct Pixel
    {
      XII_DECLARE_POD_TYPE();
      xiiFloat16 r, g, b, a;
    } pixel;
    if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture.Borrow(), 4, 4, pixel))
      return;
    XII_TEST_FLOAT(float(pixel.r), 4, 0);
    XII_TEST_FLOAT(float(pixel.g), 4, 0);
    XII_TEST_FLOAT(float(pixel.b), 4, 0);
    XII_TEST_FLOAT(float(pixel.a), 1, 0);
  }
}
