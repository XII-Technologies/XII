/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Composition/ConfidenceRadiancePass.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceAddPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST(NativeRadianceComposition, ExplicitAbsenceAndReadOnlyForwarding)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  struct HalfPixel
  {
    XII_DECLARE_POD_TYPE();
    xiiFloat16 rgba[4];
  };
  HalfPixel pixel;
  for (xiiUInt32 channel = 0; channel < 4; ++channel)
    pixel.rgba[channel] = float(channel + 1);
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {1, 1};
  description.m_Format    = xiiGALResourceFormat::RGBA16Float;
  description.m_BindFlags = xiiGALBindFlags::ShaderResource;
  xiiGALTextureSubResourceData subresource;
  subresource.m_pData         = xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&pixel), sizeof(pixel));
  subresource.m_uiStride      = sizeof(pixel);
  subresource.m_uiDepthStride = sizeof(pixel);
  xiiGALTextureData initial(xiiMakeArrayPtr(&subresource, 1));
  const auto        input = environment.GetDevice()->CreateTexture(description, &initial);
  if (!XII_TEST_BOOL(input != nullptr))
    return;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderPipelineResourceDescriptor definition;
    xiiVariantDictionary                settings;
    settings.Insert("AllowAbsentB", phase != 0);
    settings.Insert("ScaleA", phase == 2 ? 2.0f : 1.0f);
    const auto add = definition.AddPass("Optional reflection addition", xiiGetStaticRTTI<xiiRadianceAddPass>(), settings);
    settings.Clear();
    settings.Insert("AllowAbsentSignal", true);
    const auto confidence = definition.AddPass("Absent confidence", xiiGetStaticRTTI<xiiConfidenceRadiancePass>(), settings);
    const auto sink       = definition.AddPass("Export composed color", xiiGetStaticRTTI<xiiOpaqueTestExport>());
    XII_TEST_RESULT(definition.Connect(add, "Radiance", sink, "Input"));
    definition.m_Interface.PushBack({"A", true, definition.m_Passes[add].m_Id, "A"});
    if (phase == 0)
      definition.m_Interface.PushBack({"B", true, definition.m_Passes[add].m_Id, "B"});
    definition.m_Interface.PushBack({"Output", false, definition.m_Passes[sink].m_Id, "Output"});
    definition.m_Interface.PushBack({"Confidence", false, definition.m_Passes[confidence].m_Id, "Radiance"});
    xiiStringBuilder name;
    name.SetFormat("Radiance.Absence.{}", phase);
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    xiiRenderGraphTestContext context;
    if (!XII_TEST_RESULT(context.Initialize("Optional reflection composition")))
      return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    const auto                          imported = graph.AddPass<xiiRenderGraphTextureHandle>("Read-only HDR", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) { data = builder.ImportTexture("Read-only HDR", input, input->GetResourceState()); }, [](const auto&, xiiRenderGraphPassContext&) {}).first;
    const xiiRenderPipelineInputBinding binding(graph, instance.GetPipeline()->FindInterface("A", true), *imported);
    const auto                          result = instance.Build(nullptr, graph, context.GetBlackboard(), {}, xiiMakeArrayPtr(&binding, 1));
    if (phase == 0)
    {
      XII_TEST_BOOL(result.Failed());
      graph.EndSetup();
      continue;
    }
    if (!XII_TEST_RESULT(result))
      return;
    const auto output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Output", false))->m_hTexture;
    XII_TEST_BOOL(!instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Confidence", false))->m_hTexture.IsValid());
    if (phase == 1)
      XII_TEST_BOOL(output == *imported);
    graph.EndSetup();
    instance.Clear();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    const auto image = graph.GetExportedTexture(output);
    if (phase == 1)
    {
      XII_TEST_BOOL(image == input);
      xiiDynamicArray<HalfPixel> pixels;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image, pixels))
        return;
      for (xiiUInt32 channel = 0; channel < 4; ++channel)
        XII_TEST_FLOAT(float(pixels[0].rgba[channel]), float(pixel.rgba[channel]), 0);
    }
    else
    {
      XII_TEST_BOOL(image->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float);
      xiiDynamicArray<xiiVec4> pixels;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image, pixels))
        return;
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
        XII_TEST_FLOAT(pixels[0].GetData()[channel], float(pixel.rgba[channel]) * 2, 0);
    }
  }
}
