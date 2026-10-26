/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeTransparency);
XII_CREATE_SIMPLE_TEST(NativeTransparency, ReadOnlyHDRPrecisionAndConditionalInputs)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  for (xiiUInt32 phase = 0; phase < 9; ++phase)
  {
    const bool                          fullPrecision = phase != 0 && phase != 2;
    const bool                          absent        = phase == 0 || phase == 6 || phase == 7;
    const xiiColor                      background    = fullPrecision ? xiiColor(100000, 200000, 300000, 0.5f) : xiiColor(2, 4, 6, 0.5f);
    xiiRenderPipelineResourceDescriptor definition;
    const auto                          clear = [&](xiiStringView name, xiiGALResourceFormat::Enum format, xiiColor color, bool mismatch = false) {
      xiiClearTexturePass pass;
      pass.m_OutputTexture.SetFormat(format);
      pass.m_OutputTexture.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, mismatch ? 5U : 9U, 11});
      pass.m_Color = color;
      xiiVariantDictionary properties;
      XII_TEST_RESULT(pass.GetProperties(properties));
      return definition.AddPass(name, xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
    };
    const auto           hdr          = clear("Read-only opaque HDR", fullPrecision ? xiiGALResourceFormat::RGBA32Float : xiiGALResourceFormat::RGBA16Float, background);
    const auto           accumulation = clear("Weighted transparent radiance", xiiGALResourceFormat::RGBA32Float, phase == 3 ? xiiColor::MakeZero() : xiiColor(2, 4, 6, 2), phase == 8);
    const auto           revealage    = clear("Transparent transmittance", phase == 4 ? xiiGALResourceFormat::R8UNormalized : xiiGALResourceFormat::R32Float, {0.25f, 0, 0, 0});
    xiiVariantDictionary settings;
    settings.Insert("AllowAbsentAccumulation", phase != 6);
    settings.Insert("Bypass", phase == 7);
    const auto resolve = definition.AddPass("Weighted transparency resolve", xiiGetStaticRTTI<xiiWeightedBlendedOITResolvePass>(), settings);
    XII_TEST_RESULT(definition.Connect(hdr, "Texture", resolve, "HDRSceneColor"));
    if (!absent)
      XII_TEST_RESULT(definition.Connect(accumulation, "Texture", resolve, "OITAccumulate"));
    if (!absent && phase != 5)
      XII_TEST_RESULT(definition.Connect(revealage, "Texture", resolve, "OITReveal"));
    // Root inputs permit construction of the intentionally incomplete strict case.
    if (phase == 6)
    {
      definition.m_Interface.PushBack({"Accumulation", true, definition.m_Passes[resolve].m_Id, "OITAccumulate"});
      definition.m_Interface.PushBack({"Revealage", true, definition.m_Passes[resolve].m_Id, "OITReveal"});
    }
    for (const auto source : {hdr, resolve})
    {
      const auto sink = definition.AddPass(source == hdr ? "Base readback" : "Composed readback", xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(definition.Connect(source, source == hdr ? "Texture" : "HDRSceneColor", sink, "Input"));
      definition.m_Interface.PushBack({source == hdr ? "Base" : "Composed", false, definition.m_Passes[sink].m_Id, "Output"});
    }
    xiiStringBuilder name;
    name.SetFormat("Transparency.Resolve.{}", phase);
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    xiiRenderGraphTestContext context;
    if (!XII_TEST_RESULT(context.Initialize("Weighted transparency precision")))
      return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    const auto result = instance.Build(nullptr, graph, context.GetBlackboard());
    if (phase == 5 || phase == 6 || phase == 8)
    {
      XII_TEST_BOOL(result.Failed());
      graph.EndSetup();
      continue;
    }
    if (!XII_TEST_RESULT_MSG(result, "Phase %u: %s", phase, instance.GetLastBuildError().GetStartPointer()))
      return;
    const auto composed = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Composed", false))->m_hTexture;
    const auto base     = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Base", false))->m_hTexture;
    if (absent)
      XII_TEST_BOOL(composed == base);
    graph.EndSetup();
    instance.Clear();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    for (const auto output : {base, composed})
    {
      const auto image = graph.GetExportedTexture(output);
      if (!XII_TEST_BOOL(image != nullptr))
        return;
      XII_TEST_BOOL(image->GetDescription().m_Format == (fullPrecision ? xiiGALResourceFormat::RGBA32Float : xiiGALResourceFormat::RGBA16Float));
      xiiVec4 expected(background.r, background.g, background.b, background.a);
      if (output == composed && !absent && phase != 3)
      {
        const float transmittance = phase == 4 ? 64.0f / 255 : 0.25f;
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
          expected.GetData()[channel] = background.GetData()[channel] * transmittance + float(channel + 1) * (1 - transmittance);
      }
      if (fullPrecision)
      {
        xiiDynamicArray<xiiVec4> pixels;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image, pixels))
          return;
        for (const auto& pixel : pixels)
          for (xiiUInt32 channel = 0; channel < 4; ++channel)
            XII_TEST_FLOAT(pixel.GetData()[channel], expected.GetData()[channel], xiiMath::Max(0.0001f, xiiMath::Abs(expected.GetData()[channel]) * 0.000001f));
      }
      else
      {
        struct HalfPixel
        {
          XII_DECLARE_POD_TYPE();
          xiiFloat16 rgba[4];
        };
        xiiDynamicArray<HalfPixel> pixels;
        if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image, pixels))
          return;
        for (const auto& pixel : pixels)
          for (xiiUInt32 channel = 0; channel < 4; ++channel)
            XII_TEST_FLOAT(float(pixel.rgba[channel]), float(xiiFloat16(expected.GetData()[channel])), 0);
      }
    }
  }
}
