/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsTemporalPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

class xiiTemporalReflectionTestSource final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalReflectionTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Radiance{xiiGALResourceFormat::Unknown};
  xiiRenderPipelineTextureOutputPin m_Normal{xiiGALResourceFormat::RGBA32Float}, m_Material{xiiGALResourceFormat::RGBA32Float};
  xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::R32Float}, m_Ids{xiiGALResourceFormat::R32UInt}, m_Motion{xiiGALResourceFormat::RG32Float};
  bool                              m_bHalfPrecision = false;
  static xiiUInt32                  s_uiPhase;
  static bool                       s_bMoving;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto       phase = s_uiPhase;
    const xiiSizeU32 size(context.m_pView->GetRenderResolutionWidth(), context.m_pView->GetRenderResolutionHeight());
    struct Data
    {
      xiiRenderGraphTextureHandle        m_Images[6];
      xiiSizeU32                         m_Size;
      xiiDynamicArray<xiiVec4>           m_Radiance, m_Normal, m_Material;
      xiiDynamicArray<xiiColorLinear16f> m_Half;
      xiiDynamicArray<float>             m_Depth;
      xiiDynamicArray<xiiUInt32>         m_Ids;
      xiiDynamicArray<xiiVec2>           m_Motion;
      bool                               m_bHalf;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Size = size; data.m_bHalf = m_bHalfPrecision;
      const xiiGALResourceFormat::Enum formats[] = {m_bHalfPrecision ? xiiGALResourceFormat::RGBA16Float : xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::R32Float, xiiGALResourceFormat::R32UInt, xiiGALResourceFormat::RG32Float};
      for (xiiUInt32 i = 0; i < 6; ++i)
      {
        xiiGALTextureCreationDescription image;
        image.m_Type = xiiGALResourceDimension::Texture2D; image.m_Size = size; image.m_Format = formats[i]; image.m_BindFlags = xiiGALBindFlags::ShaderResource;
        xiiStringBuilder name; name.SetFormat("Known SSR input {}", i);
        data.m_Images[i] = builder.WriteTexture(name, image, xiiGALResourceStateFlags::CopyDestination);
      }
      for (xiiUInt32 y = 0; y < size.height; ++y)
        for (xiiUInt32 x = 0; x < size.width; ++x)
        {
          const float value = ((x + y + phase) & 1U) != 0U ? 3.0f : 1.0f;
          const xiiColor color(value, value * 0.5f, value * 0.25f, 0.5f);
          data.m_Radiance.PushBack(xiiVec4(color.r, color.g, color.b, color.a));
          data.m_Half.PushBack(xiiColorLinear16f(color));
          data.m_Normal.PushBack(phase >= 4 ? xiiVec4(1, 0.5f, 0, 0) : xiiVec4(0.5f, 0.5f, 0, 0));
          data.m_Material.PushBack(xiiVec4(0.3f, 0, phase >= 3 ? 0.6f : 0.5f, 0));
          data.m_Depth.PushBack(phase >= 5 ? 0.8f : 0.5f);
          data.m_Ids.PushBack(phase >= 2 ? 8U : 7U);
          data.m_Motion.PushBack(phase == 6 ? xiiVec2(10, 10) : s_bMoving && phase == 1 ? xiiVec2(4.0f / size.width, 0) : xiiVec2::MakeZero());
        } }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      const auto upload = [&](xiiUInt32 index, const void* pixels, xiiUInt32 bytesPerPixel) {
        const auto stride = data.m_Size.width * bytesPerPixel, aligned = (stride + 15U) & ~15U;
        xiiDynamicArray<xiiUInt8> padded; padded.SetCount(aligned * data.m_Size.height);
        for (xiiUInt32 row = 0; row < data.m_Size.height; ++row)
          xiiMemoryUtils::Copy(padded.GetData() + row * aligned, static_cast<const xiiUInt8*>(pixels) + row * stride, stride);
        xiiGALTextureSubResourceData source;
        source.m_pData = xiiConstByteBlobPtr(padded.GetData(), padded.GetCount()); source.m_uiStride = aligned;
        execute.GetCommandList().UpdateTexture(execute.GetTexture(data.m_Images[index]), {}, xiiBoundingBoxU32::MakeFromMinMax({0, 0, 0}, {data.m_Size.width, data.m_Size.height, 1}), source);
      };
      upload(0, data.m_bHalf ? static_cast<const void*>(data.m_Half.GetData()) : data.m_Radiance.GetData(), data.m_bHalf ? sizeof(xiiColorLinear16f) : sizeof(xiiVec4));
      upload(1, data.m_Normal.GetData(), sizeof(xiiVec4)); upload(2, data.m_Material.GetData(), sizeof(xiiVec4));
      upload(3, data.m_Depth.GetData(), sizeof(float)); upload(4, data.m_Ids.GetData(), sizeof(xiiUInt32)); upload(5, data.m_Motion.GetData(), sizeof(xiiVec2)); });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Radiance, emitted.first->m_Images[0]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Normal, emitted.first->m_Images[1]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Material, emitted.first->m_Images[2]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Depth, emitted.first->m_Images[3]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Ids, emitted.first->m_Images[4]));
    return context.SetOutput(m_Motion, emitted.first->m_Images[5]);
  }
};
xiiUInt32 xiiTemporalReflectionTestSource::s_uiPhase = 0;
bool      xiiTemporalReflectionTestSource::s_bMoving = false;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalReflectionTestSource, 1, xiiRTTIDefaultAllocator<xiiTemporalReflectionTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Radiance", m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Material", m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Depth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Ids", m_Ids)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Motion", m_Motion)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("HalfPrecision", m_bHalfPrecision),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  xiiRenderPipelineResourceHandle MakeTemporalReflectionPipeline(bool half, bool enabled, bool jitter = false)
  {
    xiiRenderPipelineResourceDescriptor definition;
    const auto                          source = definition.AddPass("Known reflection surfaces", xiiGetStaticRTTI<xiiTemporalReflectionTestSource>());
    definition.m_Passes[source].m_Properties.Insert("HalfPrecision", half);
    const auto camera = definition.AddPass("Camera history", xiiGetStaticRTTI<xiiTemporalCameraPass>());
    definition.m_Passes[camera].m_Properties.Insert("Enabled", jitter);
    const auto temporal = definition.AddPass("Temporal reflection filter", xiiGetStaticRTTI<xiiScreenSpaceReflectionsTemporalPass>());
    definition.m_Passes[temporal].m_Properties.Insert("Enabled", enabled);
    XII_TEST_RESULT(definition.Connect(camera, "TemporalCamera", temporal, "TemporalCamera"));
    const xiiStringView from[] = {"Radiance", "Normal", "Material", "Depth", "Ids", "Motion"};
    const xiiStringView to[]   = {"SSRRawTerm", "GBufferNormal", "GBufferMaterial", "SceneDepth", "GBufferObjectId", "DilatedVelocityBuffer"};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(from); ++i)
      XII_TEST_RESULT(definition.Connect(source, from[i], temporal, to[i]));
    for (const xiiStringView name : {"Resolved", "Raw", "Previous"})
    {
      const auto capture = definition.AddPass(name, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(definition.Connect(name == "Raw" ? source : temporal, name == "Raw" ? "Radiance" : name == "Resolved" ? "SSRTerm" :
                                                                                                                              "PreviousReflection",
                                         capture, "Input"));
      definition.m_Interface.PushBack({name, false, definition.m_Passes[capture].m_Id, "Output"});
    }
    xiiStringBuilder name;
    name.SetFormat("Temporal reflection fixture {} {} {}", half, enabled, jitter);
    return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(name, std::move(definition));
  }

  bool ReadTemporalPixels(xiiGALDevice& device, const xiiSharedPtr<xiiGALTexture>& image, xiiDynamicArray<xiiVec4>& pixels)
  {
    if (image->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float)
      return xiiGpuTestReadback::ReadTexturePixels(device, image, pixels);
    xiiDynamicArray<xiiColorLinear16f> half;
    if (!xiiGpuTestReadback::ReadTexturePixels(device, image, half))
      return false;
    pixels.SetCount(half.GetCount());
    for (xiiUInt32 i = 0; i < pixels.GetCount(); ++i)
    {
      const auto color = half[i].ToLinearFloat();
      pixels[i]        = xiiVec4(color.r, color.g, color.b, color.a);
    }
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(TemporalReflections);
static void TestTemporalReflectionValidation(bool half, bool jitter = false, bool moving = false)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiTemporalReflectionTestSource::s_bMoving = moving;
  XII_SCOPE_EXIT(xiiTemporalReflectionTestSource::s_bMoving = false);
  const auto          enabled = MakeTemporalReflectionPipeline(half, true, jitter), disabled = MakeTemporalReflectionPipeline(half, false, jitter);
  xiiWorldDescription description("Temporal reflection validation");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 7, 0.1f, 10);
  xiiView* views[2];
  for (auto*& view : views)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Private reflection history", view);
    view->SetCamera(&camera);
    view->SetViewport({0, 0, 9, 7});
    view->SetRenderPipelineResource(enabled);
  }
  const auto image = [&](xiiUInt32 v, xiiStringView name) {
    const auto& instance = views[v]->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
    return binding && binding->m_hTexture.IsValid() ? views[v]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
  };
  xiiDynamicArray<xiiVec4> first;
  for (xiiUInt32 phase = 0; phase < 11; ++phase)
  {
    if (phase == 7)
      views[0]->InvalidateTemporalHistory();
    if (phase == 8)
      views[0]->SetViewport({0, 0, 13, 11});
    if (phase == 9 || phase == 10)
      views[0]->SetRenderPipelineResource(phase == 9 ? disabled : enabled);
    xiiTemporalReflectionTestSource::s_uiPhase = phase;
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiSharedPtr<xiiGALTexture> histories[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      if (!XII_TEST_RESULT_MSG(views[v]->GetLastRenderResult(), "%s", views[v]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
      const auto output = image(v, "Resolved");
      histories[v]      = output;
      xiiDynamicArray<xiiVec4> raw, resolved;
      if (!ReadTemporalPixels(*environment.GetDevice(), image(v, "Raw"), raw) || !ReadTemporalPixels(*environment.GetDevice(), output, resolved))
        return;
      xiiUInt32 differences = 0;
      for (xiiUInt32 i = 0; i < raw.GetCount(); ++i)
      {
        XII_TEST_BOOL(resolved[i].IsValid());
        XII_TEST_FLOAT(resolved[i].w, raw[i].w, 0);
        differences += xiiMath::Abs(resolved[i].x - raw[i].x) > 0.01f;
        for (xiiUInt32 c = 0; c < 3; ++c)
          XII_TEST_BOOL(resolved[i].GetData()[c] >= 0 && resolved[i].GetData()[c] <= 3);
        if (phase == 1)
        {
          const float oldWeight     = 0.5f * 0.92f * 0.96f;
          const bool  outside       = moving && i % output->GetDescription().m_Size.width < 2;
          const auto  previousIndex = moving && !outside ? i - 2 : i;
          const float expected      = outside ? raw[i].x : (raw[i].x * 0.5f + first[previousIndex].x * oldWeight) / (0.5f + oldWeight);
          XII_TEST_FLOAT(resolved[i].x, expected, 0.005f);
        }
      }
      if (phase == 0 && v == 0)
        first = raw;
      if (phase == 0 || (phase >= 2 && phase <= 6) || (v == 0 && phase >= 7))
        XII_TEST_INT(differences, 0);
      else if (!jitter || phase == 1)
        XII_TEST_BOOL(differences > 8);
      // Later Halton offsets can reproject this alternating pattern onto an
      // equal-valued neighbor; a valid reuse need not change its radiance.
      if (phase == 0 || (v == 0 && (phase == 8 || phase == 9 || phase == 10)))
        XII_TEST_BOOL(!image(v, "Previous"));
      xiiTestFramework::Output(xiiTestOutput::Details, "Temporal SSR %s phase %u view %u: %u filtered pixels.", half ? "16f" : "32f", phase, v, differences);
    }
    XII_TEST_BOOL(histories[0] != histories[1]);
  }
}
XII_CREATE_SIMPLE_TEST(TemporalReflections, Float32SurfaceAndHistoryValidation) { TestTemporalReflectionValidation(false); }
XII_CREATE_SIMPLE_TEST(TemporalReflections, Float16SurfaceAndHistoryValidation) { TestTemporalReflectionValidation(true); }

XII_CREATE_SIMPLE_TEST(TemporalReflections, JitteredCameraHistoryValidation) { TestTemporalReflectionValidation(false, true); }
XII_CREATE_SIMPLE_TEST(TemporalReflections, MotionReprojectsPreviousPixels) { TestTemporalReflectionValidation(false, false, true); }

XII_CREATE_SIMPLE_TEST(TemporalReflections, RetainedRecordingsOrderCapacityAndOwnerDestruction)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto             resource = MakeTemporalReflectionPipeline(false, true);
  xiiWorldDescription    description("Retained reflection recordings");
  xiiUniquePtr<xiiWorld> world = XII_DEFAULT_NEW(xiiWorld, description);
  xiiCamera              camera;
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 7, 0.1f, 10);
  xiiView* view = nullptr;
  {
    XII_LOCK(world->GetWriteMarker());
    world->SetWorldSimulationEnabled(false);
    world->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Retained reflection view", view);
    view->SetCamera(&camera);
    view->SetViewport({0, 0, 9, 7});
  }
  xiiRenderPipelineInstance recording;
  if (!XII_TEST_RESULT(recording.Synchronize(resource)))
    return;
  xiiRenderGraphTestContext   contexts[9];
  xiiRenderGraphTextureHandle outputs[9], previous[9];
  xiiUInt64                   frames[9] = {};
  const auto                  execute   = [&](xiiUInt32 index) {
    auto& context = contexts[index];
    auto& graph   = context.GetGraph();
    auto& cache   = context.GetResourceCache();
    cache.BeginFrame(frames[index], frames[index]);
    XII_SCOPE_EXIT(cache.EndFrame());
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return false;
    environment.GetDevice()->WaitIdle();
    return true;
  };
  for (xiiUInt32 index = 0; index < 9; ++index)
  {
    if (!XII_TEST_RESULT(contexts[index].Initialize("Deferred temporal reflection graph")))
      return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    frames[index] = frame.GetFrameIndex();
    auto& context = contexts[index];
    auto& graph   = context.GetGraph();
    auto& cache   = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiTemporalReflectionTestSource::s_uiPhase = index;
    xiiResult built                            = XII_FAILURE;
    {
      XII_LOCK(world->GetWriteMarker());
      built = recording.Build(view, graph, context.GetBlackboard());
    }
    if (index < 8)
    {
      if (!XII_TEST_RESULT_MSG(built, "%s", recording.GetLastBuildError().GetStartPointer()))
        return;
      outputs[index]  = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("Resolved", false))->m_hTexture;
      previous[index] = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("Previous", false))->m_hTexture;
    }
    else
    {
      XII_TEST_BOOL(built.Failed());
      XII_TEST_BOOL(recording.GetLastBuildError().FindSubString("history capacity") != nullptr);
    }
    graph.EndSetup();
    cache.EndFrame();
    frame.End();
    if (index == 2)
    {
      // A newer recording becomes authoritative even when the earlier recording
      // executes afterwards. Both retain their own output and source payloads.
      if (!execute(1) || !execute(0))
        return;
    }
    if (index == 3)
    {
      if (!execute(3))
        return;
      xiiDynamicArray<xiiVec4> history;
      if (!ReadTemporalPixels(*environment.GetDevice(), graph.GetExportedTexture(previous[index]), history))
        return;
      XII_TEST_FLOAT(history[0].x, 3, 0); // Phase 1, not the later-executed phase 0.
    }
  }
  for (const xiiUInt32 index : {0U, 1U})
  {
    xiiDynamicArray<xiiVec4> retained;
    if (!ReadTemporalPixels(*environment.GetDevice(), contexts[index].GetGraph().GetExportedTexture(outputs[index]), retained))
      return;
    XII_TEST_FLOAT(retained[0].x, index ? 3 : 1, 0);
  }
  // Release completed and abandoned recordings, then reuse the same bounded
  // instance. GPU completion alone did not permit their slots to be overwritten.
  for (auto& context : contexts)
    context.Shutdown();
  if (!XII_TEST_RESULT(contexts[8].Initialize("Reflection recording after owner destruction")))
    return;
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    frames[8]     = frame.GetFrameIndex();
    auto& context = contexts[8];
    auto& graph   = context.GetGraph();
    auto& cache   = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiTemporalReflectionTestSource::s_uiPhase = 0;
    {
      XII_LOCK(world->GetWriteMarker());
      if (!XII_TEST_RESULT_MSG(recording.Build(view, graph, context.GetBlackboard()), "%s", recording.GetLastBuildError().GetStartPointer()))
        return;
    }
    outputs[8] = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("Resolved", false))->m_hTexture;
    graph.EndSetup();
    cache.EndFrame();
    frame.End();
  }
  recording.Clear();
  world.Clear();
  if (!execute(8))
    return;
  xiiDynamicArray<xiiVec4> result;
  if (!ReadTemporalPixels(*environment.GetDevice(), contexts[8].GetGraph().GetExportedTexture(outputs[8]), result))
    return;
  XII_TEST_FLOAT(result[0].x, 1, 0);
}

XII_CREATE_SIMPLE_TEST(TemporalReflections, UnsupportedStorageRequiresExplicitBypass)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Restricted temporal reflection device");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 7, 0.1f, 10);
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Restricted reflection view", view);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 9, 7});
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Restricted temporal reflection graph")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  XII_SCOPE_EXIT(frame.End());
  auto& graph = context.GetGraph();
  graph.BeginSetup(frame.GetFrameIndex());
  xiiRenderPipelineInstance pipeline;
  if (!XII_TEST_RESULT(pipeline.Synchronize(MakeTemporalReflectionPipeline(false, true))))
    return;
  XII_TEST_BOOL(pipeline.Build(view, graph, context.GetBlackboard()).Failed());
  XII_TEST_BOOL(pipeline.GetLastBuildError().FindSubString("TextureUAVExtendedFormats") != nullptr);
  graph.EndSetup();
  graph.BeginSetup(frame.GetFrameIndex());
  if (!XII_TEST_RESULT(pipeline.Synchronize(MakeTemporalReflectionPipeline(false, false))))
    return;
  XII_TEST_RESULT(pipeline.Build(view, graph, context.GetBlackboard()));
  graph.EndSetup();
}
