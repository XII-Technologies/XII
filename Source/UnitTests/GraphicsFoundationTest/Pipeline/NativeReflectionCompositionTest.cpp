/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeReflectionComposition);
XII_CREATE_SIMPLE_TEST(NativeReflectionComposition, PriorityReplacementReadOnlyInputsAndRetiredRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 resource = xiiLoadPipelineLibraryResource("ReflectionComposition"), selectionResource = xiiLoadPipelineLibraryResource("ReflectionSelection");
  xiiResourceLock<xiiRenderPipelineResource> reflection(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), selection(selectionResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(reflection.IsValid() && selection.IsValid() && reflection->GetAuthoring() && selection->GetAuthoring()))
    return;
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (auto& context : contexts)
    if (!XII_TEST_RESULT(context.Initialize("Independent reflection replacement")))
      return;
  for (xiiUInt32 phase = 0; phase < 10; ++phase)
  {
    xiiVec4 scene(3, 4, 5, 0.35f), environmentTerm(0.2f, 0.4f, 0.6f, 0.15f), traced(0.4f, 0.8f, 1.2f, 0.25f), screen(0.8f, 0.4f, 0.2f, 0.5f);
    if (phase == 1)
    {
      traced.w = 0;
      screen.w = 0;
    }
    if (phase == 2)
    {
      traced.w = 2;
      screen.w = 3;
    }
    if (phase == 3)
    {
      environmentTerm = xiiVec4(-1, 2, -3, 7);
      traced          = xiiVec4(4, -2, 1, -0.5f);
      screen          = xiiVec4(-2, 3, 1, 0.7f);
      scene           = xiiVec4(-1, 0.5f, 2, 0.6f);
    }
    if (phase == 4)
    {
      scene  = xiiVec4(1e8f, 2e8f, 3e8f, 0.35f);
      traced = xiiVec4(2e8f, 1e8f, 4e8f, 0.25f);
      screen = xiiVec4(3e8f, 2e8f, 1e8f, 0.5f);
    }
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({reflection->GetAuthoring()->m_Descriptor.m_Id, resource.GetResourceID()});
    const auto clear = [&](xiiStringView name, xiiVec4 color, bool invalid = false) {
      xiiClearTexturePass pass;
      pass.m_uiWidth  = invalid && phase == 5 ? 8 : 9;
      pass.m_uiHeight = 11;
      pass.m_Format   = invalid && phase == 6 ? xiiGALResourceFormat::R32UInt : xiiGALResourceFormat::RGBA32Float;
      pass.m_Color    = xiiColor(color.x, color.y, color.z, color.w);
      xiiVariantDictionary properties;
      XII_TEST_RESULT(pass.GetProperties(properties));
      return root.AddPass(name, "xiiClearTexturePass", properties);
    };
    const auto base = clear("Original scene", scene), env = clear("Environment contribution", environmentTerm), ray = clear("Traced detail", traced);
    const auto firstScreen = clear("Screen detail A", screen, true);
    auto       alternate   = screen;
    alternate.w *= 0.5f;
    const auto secondScreen = clear("Screen detail B", alternate);
    const auto connect      = [&](xiiUInt32 from, xiiStringView output, xiiUInt32 to, xiiStringView input) {
      XII_TEST_RESULT(root.Connect(from, output, to, input));
    };
    const auto capture = [&](xiiUInt32 from, xiiStringView output, xiiStringView name) {
      const auto sink = root.AddPass(name, "xiiOpaqueTestExport");
      connect(from, output, sink, "Input");
      root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
    };
    for (xiiUInt32 branch = 0; branch < 2; ++branch)
    {
      const auto node = root.AddSubgraph(branch ? "Second reflection" : "First reflection", reflection->GetAuthoring()->m_Descriptor.m_Id);
      if (branch)
      {
        auto& override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
        for (const auto& pass : reflection->GetAuthoring()->m_Descriptor.m_Passes)
          if (pass.m_Definition.IsValid())
            override.m_Path.PushBack(pass.m_Id);
        const auto screenLayer = xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ReflectionSelection/Screen Detail Over Traced Result");
        bool       found       = false;
        for (const auto& pass : selection->GetAuthoring()->m_Descriptor.m_Passes)
          found |= pass.m_Id == screenLayer && pass.m_sType == "xiiConfidenceBlendPass";
        if (!XII_TEST_BOOL(found && override.m_Path.GetCount() == 1))
          return;
        override.m_Path.PushBack(screenLayer);
        override.m_Properties.Insert("Opacity", 0.25f);
        override.m_Properties.Insert("AsyncCompute", true);
      }
      connect(base, "Texture", node, "HDRSceneColor");
      connect(env, "Texture", node, "EnvironmentSpecular");
      if (phase != 7 && phase != 9)
        connect(ray, "Texture", node, "RTFinalReflections");
      if (phase != 8 && phase != 9)
        connect(branch ? secondScreen : firstScreen, "Texture", node, "SSRTerm");
      capture(node, "HDRSceneColor", branch ? "SceneB" : "SceneA");
      capture(node, "SelectedReflection", branch ? "SelectedB" : "SelectedA");
    }
    capture(base, "Texture", "OriginalScene");
    xiiStringBuilder name;
    name.SetFormat("Authored reflection phase {}", phase);
    const auto fixture = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    for (auto& instance : instances)
      if (!XII_TEST_RESULT(instance.Synchronize(fixture)))
        return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALTexture> results[2][2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto built = instances[view].Build(nullptr, graph, contexts[view].GetBlackboard());
      if (phase == 5 || phase == 6)
      {
        XII_TEST_BOOL(built.Failed());
        XII_TEST_BOOL(instances[view].GetLastBuildError().FindSubString("ConfidenceBlend requires matching") != nullptr);
        graph.EndSetup();
        continue;
      }
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[view].GetLastBuildError().GetStartPointer()))
        return;
      const auto binding = [&](xiiStringView pin) {
        return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(pin, false))->m_hTexture;
      };
      const auto original = binding("OriginalScene"), a = binding("SceneA"), b = binding("SceneB"), selectedA = binding("SelectedA"), selectedB = binding("SelectedB");
      instances[view].Clear();
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile({.m_bEnableAsyncQueues = false})) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
        return;
      environment.GetDevice()->WaitIdle();
      const auto nonnegative = [](xiiVec3 value) {
        return xiiVec3(xiiMath::Max(value.x, 0.0f), xiiMath::Max(value.y, 0.0f), xiiMath::Max(value.z, 0.0f));
      };
      const auto check = [&](xiiRenderGraphTextureHandle handle, xiiVec4 expected) {
        const auto texture = graph.GetExportedTexture(handle);
        if (!XII_TEST_BOOL(texture && texture->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float))
          return;
        for (const xiiVec2U32 pixel : {xiiVec2U32(0, 0), xiiVec2U32(4, 5), xiiVec2U32(8, 10)})
        {
          xiiVec4 actual;
          if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture.Borrow(), pixel.x, pixel.y, actual))
            return;
          for (xiiUInt32 c = 0; c < 4; ++c)
            XII_TEST_FLOAT(actual.GetData()[c], expected.GetData()[c], 0.0001f + xiiMath::Abs(expected.GetData()[c]) * 0.00001f);
        }
      };
      check(original, scene);
      for (xiiUInt32 branch = 0; branch < 2; ++branch)
      {
        const auto selected = xiiMath::Lerp(xiiMath::Lerp(nonnegative(environmentTerm.GetAsVec3()), nonnegative(traced.GetAsVec3()), (phase == 7 || phase == 9) ? 0.0f : xiiMath::Saturate(traced.w)), nonnegative(screen.GetAsVec3()), (phase == 8 || phase == 9) ? 0.0f : xiiMath::Saturate(branch ? alternate.w : screen.w) * (branch ? 0.25f : 1.0f));
        check(branch ? selectedB : selectedA, xiiVec4(selected, environmentTerm.w));
        check(branch ? b : a, xiiVec4(nonnegative(scene.GetAsVec3() - nonnegative(environmentTerm.GetAsVec3()) + selected), scene.w));
        results[view][branch] = graph.GetExportedTexture(branch ? b : a);
        XII_TEST_BOOL(results[view][branch] != graph.GetExportedTexture(original));
      }
      XII_TEST_BOOL(results[view][0] != results[view][1]);
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      if (phase == 0)
        xiiTestFramework::Output(xiiTestOutput::Details, "Reflection composition view %u: %u operations, %u submissions, %u barriers.", view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    }
    if (phase != 5 && phase != 6)
      XII_TEST_BOOL(results[0][0] != results[1][0] && results[0][1] != results[1][1]);
    frame.End();
  }
}
