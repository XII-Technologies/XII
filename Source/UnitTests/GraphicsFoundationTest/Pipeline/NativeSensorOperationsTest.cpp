/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

namespace
{
  template <typename T> xiiSharedPtr<xiiGALTexture> SensorImage(xiiGALDevice& device, xiiGALResourceFormat::Enum format, const T (&pixels)[16])
  {
    xiiGALTextureCreationDescription desc;
    desc.m_Type      = xiiGALResourceDimension::Texture2D;
    desc.m_Size      = {4, 4};
    desc.m_Format    = format;
    desc.m_BindFlags = xiiGALBindFlags::ShaderResource;
    desc.m_Usage     = xiiGALResourceUsage::Immutable;
    xiiGALTextureSubResourceData subresource(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(pixels), sizeof(pixels)), 4 * sizeof(T), sizeof(pixels));
    xiiGALTextureData            data(xiiMakeArrayPtr(&subresource, 1));
    return device.CreateTexture(desc, &data);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeSensors);
XII_CREATE_SIMPLE_TEST(NativeSensors, SpecializedCalibratedOutputsAndRollingShutter)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  float          depthPixels[16];
  xiiFloat16Vec4 radiancePixels[16], albedoPixels[16];
  xiiFloat16Vec2 normalPixels[16], velocityPixels[16];
  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    depthPixels[i]    = 0.2f + 0.05f * (i / 4);
    radiancePixels[i] = xiiVec4(0.25f, 0.5f, 0.75f, 1);
    albedoPixels[i]   = xiiVec4(0.03125f);
    normalPixels[i]   = xiiVec2(0.5f); // Octahedral +Z in the engine's normalized encoding.
    velocityPixels[i] = xiiVec2(0.5f);
  }
  const auto depth    = SensorImage(*environment.GetDevice(), xiiGALResourceFormat::R32Float, depthPixels);
  const auto radiance = SensorImage(*environment.GetDevice(), xiiGALResourceFormat::RGBA16Float, radiancePixels);
  const auto albedo   = SensorImage(*environment.GetDevice(), xiiGALResourceFormat::RGBA16Float, albedoPixels);
  const auto normal   = SensorImage(*environment.GetDevice(), xiiGALResourceFormat::RG16Float, normalPixels);
  const auto velocity = SensorImage(*environment.GetDevice(), xiiGALResourceFormat::RG16Float, velocityPixels);
  if (!XII_TEST_BOOL(depth && radiance && albedo && normal && velocity))
    return;
  environment.GetDevice()->WaitIdle();
  xiiWorldDescription description("Specialized sensor input consumer");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Calibrated sensor", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 53.130102f, 0.1f, 100.0f);
  camera.LookAt(xiiVec3::MakeZero(), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0));
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 4, 4});
  xiiRenderGraphContext context;
  if (!XII_TEST_RESULT(context.Initialize("Specialized sensors")))
    return;
  for (xiiUInt32 rolling = 0; rolling < 2; ++rolling)
  {
    xiiRenderPipelineResourceDescriptor root;
    xiiRenderPipelineResourceHandle     assets[4];
    xiiUInt32                           branch = 0;
    for (xiiStringView name : {"DepthSensor", "RgbSensor", "InfraredSensor", "LidarSensor"})
    {
      assets[branch] = xiiLoadPipelineLibraryResource(name);
      xiiResourceLock<xiiRenderPipelineResource> source(assets[branch], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(source.IsValid() && source->GetPipeline() && source->GetAuthoring()))
        return;
      const auto definition = source->GetAuthoring();
      root.m_References.PushBack({definition->m_Descriptor.m_Id, assets[branch].GetResourceID()});
      const auto node     = root.AddSubgraph(name, definition->m_Descriptor.m_Id);
      auto&      override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(definition->m_Descriptor.m_Passes[0].m_Id);
      xiiDepthSensorPass defaults;
      auto&              c = defaults.m_Calibration;
      c.m_uiResolutionX = c.m_uiResolutionY = 4;
      c.m_fFocalLengthXPixels = c.m_fFocalLengthYPixels = 4;
      c.m_fPrincipalPointXPixels = c.m_fPrincipalPointYPixels = 2;
      c.m_Shutter                                             = rolling ? xiiSensorShutterType::Rolling : xiiSensorShutterType::Global;
      c.m_fRollingShutterSeconds                              = rolling ? 0.016f : 0.0f;
      c.m_NoiseModel                                          = xiiSensorNoiseModel::None;
      c.m_fDepthNoiseScalePerMeter = c.m_fDepthNoiseStandardDeviationMeters = 0;
      c.m_fExposureSeconds = c.m_fQuantumEfficiency = c.m_fAnalogGain = c.m_fSaturationElectrons = c.m_fRadianceToElectrons = 1;
      c.m_vSpectralSensitivity                                                                                              = xiiVec3(1, 0, 0);
      c.m_fWavelengthNanometers                                                                                             = 850;
      xiiVariantDictionary config;
      if (!XII_TEST_RESULT(defaults.GetProperties(config)))
        return;
      override.m_Properties.Insert("Calibration", *config.GetValue("Calibration"));
      override.m_Properties.Insert("UseViewProfile", false);
      for (const auto& input : definition->m_Descriptor.m_Interface)
        if (input.m_bInput)
          root.m_Interface.PushBack({input.m_sName, true, root.m_Passes[node].m_Id, input.m_sName});
      root.m_Interface.PushBack({name, false, root.m_Passes[node].m_Id, "SensorOutput"});
      ++branch;
    }
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(rolling ? "Rolling native sensors" : "Global native sensors", std::move(root));
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    XII_TEST_BOOL(instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && instance.GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin(3)))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    struct Source
    {
      xiiRenderGraphTextureHandle m_Depth, m_Radiance, m_Normal, m_Albedo, m_Velocity;
      xiiRenderGraphBufferHandle  m_Camera, m_Global;
      xiiFrameCameraConstants     m_CameraData;
      xiiFrameGlobalConstants     m_GlobalData;
    };
    const auto                                     source = graph.AddPass<Source>("Explicit test sensor inputs", xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
      data.m_Depth=builder.ImportTexture("Depth",depth,depth->GetResourceState()); data.m_Radiance=builder.ImportTexture("Radiance",radiance,radiance->GetResourceState());
      data.m_Normal=builder.ImportTexture("Normal",normal,normal->GetResourceState()); data.m_Albedo=builder.ImportTexture("Albedo",albedo,albedo->GetResourceState()); data.m_Velocity=builder.ImportTexture("Velocity",velocity,velocity->GetResourceState());
      xiiGALBufferCreationDescription uniform; uniform.m_BindFlags=xiiGALBindFlags::UniformBuffer; uniform.m_Usage=xiiGALResourceUsage::Dynamic; uniform.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write;
      uniform.m_uiSize=sizeof(data.m_CameraData); data.m_Camera=builder.WriteBuffer("Camera",uniform,xiiGALResourceStateFlags::ConstantBuffer);
      uniform.m_uiSize=sizeof(data.m_GlobalData); data.m_Global=builder.WriteBuffer("Frame timing",uniform,xiiGALResourceStateFlags::ConstantBuffer);
      data.m_CameraData=xiiSnapshotCameraConstants(*view); data.m_GlobalData=xiiSnapshotGlobalConstants(*view,xiiUInt32(frame.GetFrameIndex())); data.m_GlobalData.m_fDeltaTimeMs=16; }, [](const Source& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList();
      { xiiGALMapHelper<xiiFrameCameraConstants> mapped(commands,execute.GetBuffer(data.m_Camera),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_CameraData; }
      { xiiGALMapHelper<xiiFrameGlobalConstants> mapped(commands,execute.GetBuffer(data.m_Global),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_GlobalData; } });
    xiiDynamicArray<xiiRenderPipelineInputBinding> inputs;
    const auto*                                    compiled = instance.GetPipeline();
    for (xiiUInt32 i = 0; i < compiled->GetInterface().GetCount(); ++i)
    {
      const auto& pin = compiled->GetInterface()[i].m_Pin;
      if (!pin.m_bInput)
        continue;
      xiiRenderPipelineBinding binding;
      if (pin.m_sName == "SceneDepth")
        binding.m_hTexture = source.first->m_Depth;
      else if (pin.m_sName == "PerFrameCamera")
        binding.m_hBuffer = source.first->m_Camera;
      else if (pin.m_sName == "PerFrameGlobal")
      {
        if (rolling)
          binding.m_hBuffer = source.first->m_Global;
      }
      else if (pin.m_sName == "Velocity")
      {
        if (rolling)
          binding.m_hTexture = source.first->m_Velocity;
      }
      else if (pin.m_sName == "Radiance")
        binding.m_hTexture = source.first->m_Radiance;
      else if (pin.m_sName == "Normal")
        binding.m_hTexture = source.first->m_Normal;
      else if (pin.m_sName == "Albedo")
        binding.m_hTexture = source.first->m_Albedo;
      inputs.PushBack(xiiRenderPipelineInputBinding(graph, i, binding));
    }
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(instance.Build(nullptr, graph, context.GetBlackboard(), {}, inputs), "%s", instance.GetLastBuildError().GetStartPointer()))
      return;
    xiiRenderGraphTextureHandle handles[4];
    branch = 0;
    for (xiiStringView name : {"DepthSensor", "RgbSensor", "InfraredSensor", "LidarSensor"})
      handles[branch++] = instance.GetInterfaceBinding(compiled->FindInterface(name, false))->m_hTexture;
    const auto cameraSnapshot = source.first->m_CameraData;
    instance.Clear(); // Command recording owns the selected pipelines and calibration snapshots.
    graph.EndSetup();
    if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = false}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    environment.GetDevice()->WaitIdle();
    xiiVec4 values[4];
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(handles[i]).Borrow(), 1, 1, values[i]))
        return;
      for (xiiUInt32 j = 0; j < i; ++j)
        XII_TEST_BOOL(graph.GetExportedTexture(handles[i]) != graph.GetExportedTexture(handles[j]));
      XII_TEST_FLOAT(values[i].w, 1, 0.00001f);
    }
    const xiiVec2 uv            = rolling ? xiiVec2(0.21875f, 0.53125f) : xiiVec2(0.375f);
    const float   sampleDepth   = depthPixels[xiiUInt32(uv.y * 4) * 4 + xiiUInt32(uv.x * 4)];
    const xiiVec4 homogeneous   = cameraSnapshot.m_InverseViewProjectionMatrix.Transform(xiiVec4(uv.x * 2 - 1, 1 - uv.y * 2, sampleDepth, 1));
    const xiiVec3 worldPosition = homogeneous.GetAsVec3() / homogeneous.w;
    const float   axial         = (worldPosition - cameraSnapshot.m_CameraPositionAndNearPlane.GetAsVec3()).Dot(cameraSnapshot.m_CameraForwardAndFarPlane.GetAsVec3().GetNormalized());
    const float   range         = axial * xiiVec3(-0.125f, -0.125f, 1).GetLength();
    const float   incidence     = xiiMath::Max(0.0f, (-worldPosition).GetNormalized().z);
    XII_TEST_FLOAT(values[0].x, axial, 0.0001f);
    XII_TEST_FLOAT(values[0].y, range, 0.0001f);
    XII_TEST_FLOAT(values[0].z, 1, 0.00001f);
    XII_TEST_FLOAT(values[1].x, 0.25f, 0.0001f);
    XII_TEST_FLOAT(values[1].y, 0.5f, 0.0001f);
    XII_TEST_FLOAT(values[1].z, 0.75f, 0.0001f);
    XII_TEST_FLOAT(values[2].x, 0.25f, 0.0001f);
    XII_TEST_FLOAT(values[2].y, 850, 0.0001f);
    XII_TEST_FLOAT(values[2].z, range, 0.0001f);
    XII_TEST_FLOAT(values[3].x, range, 0.0001f);
    XII_TEST_FLOAT(values[3].y, xiiMath::Min(1.0f, 0.03125f * incidence / (range * range)), 0.0001f);
    XII_TEST_FLOAT(values[3].z, incidence, 0.0001f);
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
}
