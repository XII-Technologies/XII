/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentLightConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>

#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
// Test-only sink preserving the uniform-buffer contract of atmosphere parameters.
class xiiAtmosphereTestUniformExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtmosphereTestUniformExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_Output{false, xiiGALBindFlags::UniformBuffer, xiiRenderPipelinePinAccess::Read};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto pass = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) { data=builder.ReadBuffer(context.GetInputBuffer(m_Input),xiiGALResourceStateFlags::ConstantBuffer);builder.ExportBuffer(data,xiiGALResourceStateFlags::ConstantBuffer); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *pass.first);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereTestUniformExport,1,xiiRTTIDefaultAllocator<xiiAtmosphereTestUniformExport>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Input",m_Input)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Output",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES; } XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
namespace
{
  xiiVec3 AtmosphereReferenceTransmission(const xiiAtmosphereSettings& s, float u, float v)
  {
    const float   height = xiiMath::Lerp(s.m_fPlanetRadiusKm, s.m_fAtmosphereRadiusKm, u), cosine = v * 2 - 1;
    const xiiVec3 origin(0, height, 0), direction(xiiMath::Sqrt(xiiMath::Max(0.0f, 1 - cosine * cosine)), cosine, 0);
    const float   b = height * cosine, c = height * height - s.m_fAtmosphereRadiusKm * s.m_fAtmosphereRadiusKm;
    const float   atmosphereDistance = -b + xiiMath::Sqrt(xiiMath::Max(b * b - c, 0.0f));
    const float   groundC = height * height - s.m_fPlanetRadiusKm * s.m_fPlanetRadiusKm, discriminant = b * b - groundC;
    if (discriminant >= 0 && -b - xiiMath::Sqrt(discriminant) >= 0 && -b - xiiMath::Sqrt(discriminant) < atmosphereDistance)
      return xiiVec3::MakeZero();
    xiiVec3     optical = xiiVec3::MakeZero();
    const float dt      = atmosphereDistance / s.m_uiTransmittanceIntegrationSteps;
    for (xiiUInt32 i = 0; i < s.m_uiTransmittanceIntegrationSteps; ++i)
    {
      const float altitude = xiiMath::Max((origin + direction * ((i + 0.5f) * dt)).GetLength() - s.m_fPlanetRadiusKm, 0.0f);
      const float rayleigh = xiiMath::Exp(-altitude / s.m_fRayleighScaleHeightKm), mie = xiiMath::Exp(-altitude / s.m_fMieScaleHeightKm), ozone = xiiMath::Max(0.0f, 1 - xiiMath::Abs(altitude - 25) / 15);
      optical += (s.m_vRayleighScattering * rayleigh + (s.m_vMieScattering + s.m_vMieAbsorption) * mie + s.m_vOzoneAbsorption * ozone) * dt;
    }
    return xiiVec3(xiiMath::Exp(-optical.x), xiiMath::Exp(-optical.y), xiiMath::Exp(-optical.z));
  }
  xiiVec3 SampleAtmosphereTransmission(const xiiAtmosphereSettings& s, xiiSizeU32 extent, float u, float v)
  {
    const float x = xiiMath::Clamp(u * extent.width - 0.5f, 0.0f, float(extent.width - 1)), y = xiiMath::Clamp(v * extent.height - 0.5f, 0.0f, float(extent.height - 1));
    const auto  ix = static_cast<xiiUInt32>(x), iy = static_cast<xiiUInt32>(y);
    const auto  sample = [&](xiiUInt32 px, xiiUInt32 py) {
      return AtmosphereReferenceTransmission(s, (px + 0.5f) / extent.width, (py + 0.5f) / extent.height);
    };
    return xiiMath::Lerp(xiiMath::Lerp(sample(ix, iy), sample(xiiMath::Min(ix + 1, extent.width - 1), iy), x - ix), xiiMath::Lerp(sample(ix, xiiMath::Min(iy + 1, extent.height - 1)), sample(xiiMath::Min(ix + 1, extent.width - 1), xiiMath::Min(iy + 1, extent.height - 1)), x - ix), y - iy);
  }
  xiiVec3 AtmosphereReferenceMultiple(const xiiAtmosphereSettings& s, xiiSizeU32 transmissionExtent, float u, float v)
  {
    const float altitude = (s.m_fAtmosphereRadiusKm - s.m_fPlanetRadiusKm) * v, rayleigh = xiiMath::Exp(-altitude / s.m_fRayleighScaleHeightKm), mie = xiiMath::Exp(-altitude / s.m_fMieScaleHeightKm), g = s.m_fMiePhaseG;
    xiiVec3     value = xiiVec3::MakeZero();
    const float pi    = xiiMath::Pi<float>();
    for (xiiUInt32 i = 0; i < s.m_uiMultiScatterSqrtSamples; ++i)
    {
      const float cosine = 1 - 2 * (i + 0.5f) / s.m_uiMultiScatterSqrtSamples, phaseRayleigh = 3 / (16 * pi) * (1 + cosine * cosine), phaseMie = 3 / (8 * pi) * ((1 - g * g) * (1 + cosine * cosine)) / ((2 + g * g) * xiiMath::Pow(1 + g * g - 2 * g * cosine, 1.5f));
      value += s.m_vRayleighScattering * (rayleigh * phaseRayleigh) + s.m_vMieScattering * (mie * phaseMie);
    }
    return (value / float(s.m_uiMultiScatterSqrtSamples)).CompMul(SampleAtmosphereTransmission(s, transmissionExtent, v, u));
  }
  void CheckAtmosphereLookups(xiiGALDevice& device, const xiiSharedPtr<xiiGALTexture>& transmission, const xiiSharedPtr<xiiGALTexture>& multiple, const xiiAtmosphereSettings& settings)
  {
    if (!XII_TEST_BOOL(transmission && multiple))
      return;
    for (xiiUInt32 type = 0; type < 2; ++type)
    {
      const auto image = type ? multiple : transmission;
      const auto size  = image->GetDescription().m_Size;
      for (const xiiVec2U32 pixel : {xiiVec2U32(0, 0), xiiVec2U32(size.width / 2, size.height / 2), xiiVec2U32(size.width - 1, size.height - 1), xiiVec2U32(size.width / 4, 3 * size.height / 4)})
      {
        const float u = (pixel.x + 0.5f) / size.width, v = (pixel.y + 0.5f) / size.height;
        const auto  expected = type ? AtmosphereReferenceMultiple(settings, transmission->GetDescription().m_Size, u, v) : AtmosphereReferenceTransmission(settings, u, v);
        xiiVec4     value;
        if (!xiiGpuTestReadback::ReadTexturePixel(device, image.Borrow(), pixel.x, pixel.y, value))
          return;
        for (xiiUInt32 c = 0; c < 3; ++c)
          XII_TEST_FLOAT(value.GetData()[c], expected.GetData()[c], 0.0003f);
        XII_TEST_FLOAT(value.w, 1.0f, 0.00001f);
      }
    }
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeAtmosphere);
XII_CREATE_SIMPLE_TEST(NativeAtmosphere, WorldComponentsAndIndependentLookupViews)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("WorldAtmosphereView");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  xiiAtmosphereConstantsPass fallback;
  fallback.m_bUseWorldAtmosphere                        = true;
  fallback.m_Settings.m_fPlanetRadiusKm                 = 10;
  fallback.m_Settings.m_fAtmosphereRadiusKm             = 11;
  fallback.m_Settings.m_vRayleighScattering             = xiiVec3(0.03f, 0.06f, 0.09f);
  fallback.m_Settings.m_uiTransmittanceIntegrationSteps = 32;
  fallback.m_Settings.m_uiMultiScatterSqrtSamples       = 4;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto atmosphere       = root.AddSubgraph("Authored world atmosphere", loaded->GetAuthoring()->m_Descriptor.m_Id);
  auto&      fallbackOverride = root.m_Passes[atmosphere].m_Overrides.ExpandAndGetRef();
  fallbackOverride.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.WorldAtmosphereView/World Atmosphere Lookup"));
  fallbackOverride.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.AtmosphereLookup/Physical Atmosphere Parameters"));
  XII_TEST_RESULT(fallback.GetProperties(fallbackOverride.m_Properties));
  xiiClearTexturePass clear;
  clear.m_OutputTexture.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, 9, 11});
  clear.m_OutputTexture.SetFormat(xiiGALResourceFormat::RGBA32Float);
  clear.m_Color = xiiColor(1, 2, 3, 0.4f);
  xiiVariantDictionary properties;
  XII_TEST_RESULT(clear.GetProperties(properties));
  const auto color = root.AddPass("Independent scene radiance", xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
  clear.m_OutputTexture.SetFormat(xiiGALResourceFormat::R32Float);
  clear.m_Color = xiiColor::MakeZero();
  XII_TEST_RESULT(clear.GetProperties(properties));
  const auto depth  = root.AddPass("Sky depth", xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
  const auto camera = root.AddPass("View camera", xiiGetStaticRTTI<xiiViewConstantsPass>()), light = root.AddPass("No direct sun", xiiGetStaticRTTI<xiiEnvironmentLightConstantsPass>());
  XII_TEST_RESULT(root.Connect(color, "Texture", atmosphere, "Radiance"));
  XII_TEST_RESULT(root.Connect(depth, "Texture", atmosphere, "Depth"));
  XII_TEST_RESULT(root.Connect(camera, "PerFrameCamera", atmosphere, "Camera"));
  XII_TEST_RESULT(root.Connect(light, "EnvironmentLight", atmosphere, "EnvironmentLight"));
  for (const xiiStringView name : {"Radiance", "Transmittance", "MultiScatter"})
  {
    const auto sink = root.AddPass(xiiStringBuilder("Read ", name), xiiGetStaticRTTI<xiiIndirectTestExport>());
    XII_TEST_RESULT(root.Connect(atmosphere, name, sink, "Input"));
    root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
  }
  const auto parameters = root.AddPass("Read atmosphere parameters", xiiGetStaticRTTI<xiiAtmosphereTestUniformExport>());
  XII_TEST_RESULT(root.Connect(atmosphere, "Atmosphere", parameters, "Input"));
  root.m_Interface.PushBack({"Atmosphere", false, root.m_Passes[parameters].m_Id, "Output"});
  for (const xiiStringView name : {"Solar Transmittance", "Multiple Scattering"})
  {
    auto& override = root.m_Passes[atmosphere].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.WorldAtmosphereView/World Atmosphere Lookup"));
    override.m_Path.PushBack(xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.AtmosphereLookup/", name)));
    override.m_Path.PushBack(xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name == "Solar Transmittance" ? "AtmosphereTransmittance" : "AtmosphereMultiScatter", "/Operation")));
    override.m_Properties.Insert("Width", name == "Solar Transmittance" ? 33U : 13U);
    override.m_Properties.Insert("Height", name == "Solar Transmittance" ? 17U : 9U);
  }
  const auto fixture = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Authored world atmosphere lookups", std::move(root));
  struct Scene
  {
    xiiUniquePtr<xiiWorld> m_World;
    xiiRenderWorldModule*  m_Module;
    xiiGameObjectHandle    m_Object;
    xiiComponentHandle     m_Component;
    xiiCamera              m_Camera;
    xiiView*               m_Views[2];
    xiiAtmosphereSettings  m_Settings;
  } scenes[2];
  for (xiiUInt32 w = 0; w < 2; ++w)
  {
    auto&               s = scenes[w];
    xiiWorldDescription description("Component-authored atmosphere");
    s.m_World = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(s.m_World->GetWriteMarker());
    s.m_World->SetWorldSimulationEnabled(false);
    s.m_Module                         = s.m_World->GetOrCreateModule<xiiRenderWorldModule>();
    s.m_Settings.m_fPlanetRadiusKm     = 10;
    s.m_Settings.m_fAtmosphereRadiusKm = w ? 13.0f : 12.0f;
    s.m_Settings.m_vRayleighScattering = xiiVec3(0.02f, 0.04f, 0.08f);
    s.m_Settings.m_vMieScattering      = xiiVec3(0.01f);
    s.m_Settings.m_vMieAbsorption      = xiiVec3(0.02f);
    s.m_Settings.m_vOzoneAbsorption.SetZero();
    s.m_Settings.m_uiTransmittanceIntegrationSteps = 32;
    s.m_Settings.m_uiMultiScatterSqrtSamples       = 4;
    xiiGameObject* object                          = nullptr;
    s.m_Object                                     = s.m_World->CreateObject({}, object);
    xiiSkyAtmosphereComponent* component           = nullptr;
    s.m_Component                                  = xiiSkyAtmosphereComponent::CreateComponent(object, component);
    component->SetAtmosphereSettings(s.m_Settings);
    s.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    s.m_Camera.LookAt({0, 0, 0}, {1, 0, 1}, {0, 0, 1});
    for (auto& view : s.m_Views)
    {
      s.m_Module->CreateView("Explicit atmosphere view", view);
      view->SetCamera(&s.m_Camera);
      view->SetViewport({0, 0, 9, 11});
      view->SetRenderPipelineResource(fixture);
    }
  }
  for (xiiUInt32 phase = 0; phase < 5; ++phase)
  {
    if (phase == 3)
      scenes[0].m_World.Clear();
    xiiSharedPtr<xiiGALTexture> textures[2][2];
    for (xiiUInt32 w = 0; w < 2; ++w)
    {
      auto& s = scenes[w];
      if (!s.m_World)
        continue;
      XII_LOCK(s.m_World->GetWriteMarker());
      if (phase == 1 && w == 0)
      {
        s.m_Settings.m_vRayleighScattering *= 2;
        xiiSkyAtmosphereComponent* component = nullptr;
        if (!XII_TEST_BOOL(s.m_World->TryGetComponent(s.m_Component, component)))
          return;
        component->SetAtmosphereSettings(s.m_Settings);
      }
      if (phase == 2 && w == 0)
      {
        s.m_World->DeleteObjectNow(s.m_Object);
        s.m_Settings = fallback.m_Settings;
      }
      s.m_World->Update();
      environment.GetDevice()->WaitIdle();
      XII_TEST_BOOL(s.m_Module->GetExtractionRequirements() == xiiRenderExtractionCategory::Atmosphere && s.m_Module->GetGpuInstanceData() == nullptr);
      for (xiiUInt32 v = 0; v < 2; ++v)
      {
        const auto* view     = s.m_Views[v];
        const auto& instance = view->GetRenderPipelineInstance();
        const auto* graph    = view->GetRenderGraph();
        if (!XII_TEST_BOOL(instance.GetPipeline() != nullptr))
          return;
        const auto binding = [&](xiiStringView name) {
          return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
        };
        if (!XII_TEST_BOOL(binding("Transmittance") && binding("MultiScatter") && binding("Atmosphere") && binding("Radiance")))
          return;
        const auto transmission = graph->GetExportedTexture(binding("Transmittance")->m_hTexture), multiple = graph->GetExportedTexture(binding("MultiScatter")->m_hTexture);
        CheckAtmosphereLookups(*environment.GetDevice(), transmission, multiple, s.m_Settings);
        textures[w][v] = transmission;
        xiiAtmosphereConstants constants;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph->GetExportedBuffer(binding("Atmosphere")->m_hBuffer).Borrow(), 0, constants))
          return;
        XII_TEST_FLOAT(constants.PlanetAtmosphereRadiiScaleHeights.x, s.m_Settings.m_fPlanetRadiusKm, 0);
        XII_TEST_FLOAT(constants.RayleighScattering.z, s.m_Settings.m_vRayleighScattering.z, 0);
        xiiVec4 pixel;
        if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph->GetExportedTexture(binding("Radiance")->m_hTexture).Borrow(), 4, 5, pixel))
          return;
        XII_TEST_FLOAT(pixel.x, 1, 0.0001f);
        XII_TEST_FLOAT(pixel.y, 2, 0.0001f);
        XII_TEST_FLOAT(pixel.z, 3, 0.0001f);
        XII_TEST_FLOAT(pixel.w, 1, 0.0001f);
        if (phase == 0)
          xiiTestFramework::Output(xiiTestOutput::Details, "World atmosphere %u view %u: %u operations, %u submissions, %u barriers.", w, v, graph->GetStatistics().m_uiCompiledPassCount, graph->GetStatistics().m_uiQueueSubmissionCount, graph->GetStatistics().m_uiTotalBarrierCount);
      }
      XII_TEST_BOOL(textures[w][0] != textures[w][1]);
    }
    if (phase < 3)
      XII_TEST_BOOL(textures[0][0] != textures[1][0]);
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
  // Independent authored lookup recordings retain no world, pass object or global cache.
  const auto                                 lookupAsset = xiiLoadPipelineLibraryResource("AtmosphereLookup");
  xiiResourceLock<xiiRenderPipelineResource> lookup(lookupAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(lookup.IsValid() && lookup->GetAuthoring()))
    return;
  xiiRenderPipelineResourceDescriptor standalone;
  standalone.m_References.PushBack({lookup->GetAuthoring()->m_Descriptor.m_Id, lookupAsset.GetResourceID()});
  const auto occurrence          = standalone.AddSubgraph("Retired lookup", lookup->GetAuthoring()->m_Descriptor.m_Id);
  fallback.m_bUseWorldAtmosphere = false;
  auto& authored                 = standalone.m_Passes[occurrence].m_Overrides.ExpandAndGetRef();
  authored.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.AtmosphereLookup/Physical Atmosphere Parameters"));
  XII_TEST_RESULT(fallback.GetProperties(authored.m_Properties));
  for (const xiiStringView pin : {"Transmittance", "MultiScatter"})
    standalone.m_Interface.PushBack({pin, false, standalone.m_Passes[occurrence].m_Id, pin});
  const auto                standaloneResource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Retired authored atmosphere lookup", std::move(standalone));
  xiiRenderGraphTestContext context;
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(context.Initialize("Retired atmosphere lookup")) || !XII_TEST_RESULT(instance.Synchronize(standaloneResource)))
    return;
  XII_TEST_BOOL(instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard())))
    return;
  const auto transmission = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Transmittance", false))->m_hTexture, multiple = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("MultiScatter", false))->m_hTexture;
  graph.AddPass<xiiUInt32>("Retained atmosphere exports", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {for(const auto image:{transmission,multiple})builder.ExportTexture(builder.ReadTexture(image,xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
  instance.Clear();
  scenes[1].m_World.Clear();
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile({.m_bEnableAsyncQueues = false})) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
    return;
  frame.End();
  environment.GetDevice()->WaitIdle();
  CheckAtmosphereLookups(*environment.GetDevice(), graph.GetExportedTexture(transmission), graph.GetExportedTexture(multiple), fallback.m_Settings);
}

class xiiResolutionHistoryTestPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiResolutionHistoryTestPass, xiiRenderPipelinePass);

public:
  xiiUInt32 m_uiResets          = 0;
  xiiUInt32 m_uiPublishedResets = 0;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    xiiRenderPipelinePin pin;
    pin.m_sName                  = "History";
    pin.m_bInput                 = false;
    pin.m_bOptional              = true;
    pin.m_bTemporalHistoryOutput = true;
    pins.PushBack(pin);
  }
  void      InvalidateTemporalHistory() override { ++m_uiResets; }
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext&) override
  {
    m_uiPublishedResets = m_uiResets;
    return XII_SUCCESS;
  }
  xiiResult Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiResolutionHistoryTestPass, 1, xiiRTTIDefaultAllocator<xiiResolutionHistoryTestPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(TextureResolution);
XII_CREATE_SIMPLE_TEST(TextureResolution, ViewLocalTextureResolution)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  using Mode = xiiRenderPipelineTextureResolutionMode;
  xiiWorldDescription description("Pipeline resolution");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*    module     = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* firstView  = nullptr;
  xiiView* secondView = nullptr;
  module->CreateView("First", firstView);
  module->CreateView("Second", secondView);
  firstView->SetViewport({0, 0, 1920, 1080});
  firstView->SetRenderScale(0.5f);
  secondView->SetViewport({0, 0, 1280, 720});
  secondView->SetRenderScale(0.75f);
  xiiRenderPipelineResourceDescriptor definition;
  for (auto mode : {Mode::RenderResolution, Mode::Viewport, Mode::Fixed})
  {
    xiiCreateTexturePass               pass;
    xiiRenderPipelineTextureResolution resolution;
    if (mode != Mode::Viewport)
      resolution.m_Mode = mode; // Viewport must also work through the default policy.
    resolution.m_fScale = 0.5f;
    if (mode == Mode::Fixed)
    {
      resolution.m_uiWidth  = 19;
      resolution.m_uiHeight = 11;
    }
    pass.m_OutputTexture.SetResolution(resolution);
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    definition.AddPass(mode == Mode::Fixed ? "Fixed texture" : mode == Mode::Viewport ? "Viewport texture" :
                                                                                        "Internal texture",
                       xiiGetStaticRTTI<xiiCreateTexturePass>(), properties);
  }
  definition.AddPass("History invalidation", xiiGetStaticRTTI<xiiResolutionHistoryTestPass>());
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled)))
    return;
  xiiRenderPipelineInstance first, second;
  XII_TEST_RESULT(first.Initialize(compiled));
  XII_TEST_RESULT(second.Initialize(compiled));
  xiiRenderGraph           a, b;
  xiiRenderGraphBlackboard blackboardA, blackboardB;
  const auto               check = [&](xiiView* view, xiiRenderPipelineInstance& instance, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiUInt32 frame) {
    graph.BeginSetup(frame);
    XII_TEST_RESULT(instance.Build(view, graph, blackboard));
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      const auto* binding = instance.GetOutputBinding(i, "Texture");
      const auto* texture = binding ? graph.GetTextureDescription(binding->m_hTexture) : nullptr;
      if (!XII_TEST_BOOL(texture != nullptr))
        continue;
      const xiiSizeU32 expected = i == 0 ? xiiSizeU32((view->GetRenderResolutionWidth() + 1) / 2, (view->GetRenderResolutionHeight() + 1) / 2) :
                      i == 1                           ? xiiSizeU32(static_cast<xiiUInt32>(view->GetViewport().width) / 2, static_cast<xiiUInt32>(view->GetViewport().height) / 2) :
                                                         xiiSizeU32(10, 6);
      XII_TEST_BOOL(texture->m_Size == expected);
    }
    graph.EndSetup();
  };
  check(firstView, first, a, blackboardA, 0);
  check(secondView, second, b, blackboardB, 0);
  const auto* history = static_cast<const xiiResolutionHistoryTestPass*>(first.GetPass(3));
  XII_TEST_INT(history->m_uiResets, 1);
  check(firstView, first, a, blackboardA, 1);
  XII_TEST_INT(history->m_uiResets, 1);
  firstView->SetRenderScale(1.0f);
  check(firstView, first, a, blackboardA, 2);
  XII_TEST_INT(history->m_uiResets, 2);
  XII_TEST_INT(history->m_uiPublishedResets, 2);
  firstView->SetViewport({0, 0, 800, 600});
  check(firstView, first, a, blackboardA, 3);
  XII_TEST_INT(history->m_uiResets, 3);
  XII_TEST_INT(static_cast<const xiiResolutionHistoryTestPass*>(second.GetPass(3))->m_uiResets, 1);
  XII_TEST_BOOL(first.GetPipeline() == second.GetPipeline() && first.GetPipeline() == compiled.Borrow());
  XII_TEST_BOOL(compiled->GetPins(0)[0].m_Resolution.m_Mode == Mode::RenderResolution);
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_TEST_RESULT(definition.Serialize(writer));
  xiiMemoryStreamReader               reader(&storage);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> roundtrip;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, roundtrip));
  XII_TEST_BOOL(roundtrip->GetPins(0)[0].m_Resolution == compiled->GetPins(0)[0].m_Resolution);
  firstView->SetViewport({0, 0, xiiMath::NaN<float>(), 10});
  a.BeginSetup(4);
  XII_TEST_BOOL(first.Build(firstView, a, blackboardA).Failed());
  a.EndSetup();
  xiiGALTextureCreationDescription importedDescription;
  importedDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  importedDescription.m_Size      = {37, 23};
  importedDescription.m_Format    = xiiGALResourceFormat::RGBA16Float;
  importedDescription.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
  auto imported                   = environment.GetDevice()->CreateTexture(importedDescription);
  if (!XII_TEST_BOOL(imported != nullptr))
    return;
  a.BeginSetup(5);
  const auto                   handle = a.AddPass<xiiRenderGraphTextureHandle>("Imported resolution", xiiGALCommandQueueFlags::Graphics, [&](auto& resource, xiiRenderGraphBuilder& builder) { resource = builder.ImportTexture("Offscreen texture", imported, imported->GetResourceState()); }, [](const auto&, xiiRenderGraphPassContext&) {});
  xiiVariantDictionary         parameters;
  xiiRenderPipelinePassContext context{nullptr, a, blackboardA, parameters, {}, {}, "Imported extent"};
  xiiRenderPipelinePin         pin;
  pin.m_Resolution.m_Mode   = Mode::Input;
  pin.m_Resolution.m_fScale = 0.5f;
  xiiSizeU32 extent;
  XII_TEST_RESULT(context.ResolveTextureSize(pin, *handle.first, extent));
  XII_TEST_BOOL(extent == xiiSizeU32(19, 12));
  XII_TEST_BOOL(imported->GetDescription().m_Size == xiiSizeU32(37, 23));
  a.EndSetup();
}
