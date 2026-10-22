/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>

#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
namespace
{
  xiiVec4 TransportRadiance(xiiUInt32 phase, float u, float v)
  {
    if (phase >= 10)
      return xiiVec4(0.2f + 1.2f * u, 0.4f + v, 0.1f + 0.5f * u + 0.3f * v, 0.3f);
    if (phase == 5)
      return xiiVec4(1e8f, 2e8f, 3e8f, 0.3f);
    return phase == 6 ? xiiVec4(-1, 2, -3, 0.3f) : xiiVec4(4, 8, 12, 0.3f);
  }
  float TransportDepth(xiiUInt32 phase, xiiUInt32 x) { return phase == 2 ? 0.0f : phase == 11 && x >= 4 ? 0.8f :
                                                                                                          0.5f; }
} // namespace
class xiiScreenTransportTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenTransportTestSource, xiiRenderPipelinePass);

public:
  xiiUInt32                         m_uiPhase = 0;
  xiiRenderPipelineTextureOutputPin m_Radiance{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::R32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferOutputPin  m_Camera{false, xiiGALBindFlags::UniformBuffer}, m_Light{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    xiiRenderPassResourceScope scope(context);
    struct Data
    {
      xiiRenderGraphTextureHandle  m_Radiance, m_Depth;
      xiiRenderGraphBufferHandle   m_Camera, m_Light;
      xiiFrameCameraConstants      m_CameraData;
      xiiEnvironmentLightConstants m_LightData;
      xiiVec4                      m_Pixels[99];
      float                        m_Depths[132] = {};
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_CameraData=xiiFrameCameraConstants();data.m_CameraData.m_CameraPositionAndNearPlane=xiiVec4(0,0,m_uiPhase==9?2000.0f:2.0f,0.1f);
      if(m_uiPhase==9){data.m_CameraData.m_InverseViewProjectionMatrix=xiiMat4::MakeScaling(xiiVec3(1,1,1000));data.m_CameraData.m_ViewProjectionMatrix=xiiMat4::MakeScaling(xiiVec3(1,1,0.001f));}
      data.m_LightData={};data.m_LightData.EnvironmentDirectionAndIlluminance=xiiVec4(0,0,-1,m_uiPhase==9?2.0f:0.0f);data.m_LightData.EnvironmentColorAndAngularRadius=xiiVec4(1,1,1,0);
      for(xiiUInt32 y=0;y<11;++y)for(xiiUInt32 x=0;x<9;++x){data.m_Pixels[y*9+x]=TransportRadiance(m_uiPhase,(x+0.5f)/9,(y+0.5f)/11);data.m_Depths[y*12+x]=TransportDepth(m_uiPhase,x);}
      xiiGALTextureCreationDescription texture;texture.m_Type=xiiGALResourceDimension::Texture2D;texture.m_Size={9,11};texture.m_Format=xiiGALResourceFormat::RGBA32Float;texture.m_BindFlags=xiiGALBindFlags::ShaderResource;
      data.m_Radiance=builder.WriteTexture("Transport source radiance",texture,xiiGALResourceStateFlags::CopyDestination);texture.m_Format=xiiGALResourceFormat::R32Float;data.m_Depth=builder.WriteTexture("Transport source depth",texture,xiiGALResourceStateFlags::CopyDestination);
      xiiGALBufferCreationDescription buffer;buffer.m_uiSize=m_uiPhase==8?144:sizeof(xiiFrameCameraConstants);buffer.m_BindFlags=xiiGALBindFlags::UniformBuffer;buffer.m_Usage=xiiGALResourceUsage::Dynamic;buffer.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write;data.m_Camera=builder.WriteBuffer("Transport camera",buffer,xiiGALResourceStateFlags::ConstantBuffer);buffer.m_uiSize=sizeof(xiiEnvironmentLightConstants);data.m_Light=builder.WriteBuffer("Transport light",buffer,xiiGALResourceStateFlags::ConstantBuffer); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList();{xiiGALMapHelper<xiiFrameCameraConstants> mapped(commands,execute.GetBuffer(data.m_Camera),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_CameraData;}{xiiGALMapHelper<xiiEnvironmentLightConstants> mapped(commands,execute.GetBuffer(data.m_Light),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_LightData;}
      xiiGALStateTransitionDescription transitions[2];transitions[0].m_pResource=execute.GetBuffer(data.m_Camera);transitions[1].m_pResource=execute.GetBuffer(data.m_Light);for(auto& t:transitions){t.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;t.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;}commands.TransitionResourceStates(xiiMakeArrayPtr(transitions));
      xiiGALTextureSubResourceData upload;upload.m_pData=xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Pixels),sizeof(data.m_Pixels));upload.m_uiStride=9*sizeof(xiiVec4);commands.UpdateTexture(execute.GetTexture(data.m_Radiance),{},xiiBoundingBoxU32::MakeFromMinMax({0,0,0},{9,11,1}),upload);
      upload.m_pData=xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Depths),sizeof(data.m_Depths));upload.m_uiStride=12*sizeof(float);commands.UpdateTexture(execute.GetTexture(data.m_Depth),{},xiiBoundingBoxU32::MakeFromMinMax({0,0,0},{9,11,1}),upload); });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Radiance, pass.first->m_Radiance));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Depth, pass.first->m_Depth));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Camera, pass.first->m_Camera));
    return context.SetOutput(m_Light, pass.first->m_Light);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenTransportTestSource,1,xiiRTTIDefaultAllocator<xiiScreenTransportTestSource>)
{ XII_BEGIN_PROPERTIES {
  XII_MEMBER_PROPERTY("Radiance",m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),XII_MEMBER_PROPERTY("EnvironmentLight",m_Light)->AddFlags(xiiPropertyFlags::ReadOnly),XII_MEMBER_PROPERTY("Phase",m_uiPhase),
} XII_END_PROPERTIES; } XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
XII_CREATE_SIMPLE_TEST_GROUP(NativeScreenTransport);
XII_CREATE_SIMPLE_TEST(NativeScreenTransport, AuthoredTransportMaterialResponseAndImmutableRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 resource = xiiLoadPipelineLibraryResource("ScreenEffects");
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (auto& context : contexts)
    if (!XII_TEST_RESULT(context.Initialize("Authored screen transport")))
      return;
  for (xiiUInt32 phase = 0; phase < 15; ++phase)
  {
    const bool                          transmissionEnabled = phase != 4 && phase != 9 && phase != 13;
    const float                         transmission        = transmissionEnabled ? (phase == 3 ? 0.5f : 0.25f) : 0.0f;
    const float                         metallic            = phase == 3 ? 1.0f : 0.0f;
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, resource.GetResourceID()});
    const auto source = root.AddPass("Analytic transport inputs", xiiScreenTransportTestSource().GetDynamicRTTI());
    root.m_Passes[source].m_Properties.Insert("Phase", phase);
    const auto clear = [&](xiiStringView name, xiiVec4 value, bool mismatch = false) {
      xiiClearTexturePass p;
      p.m_uiWidth  = mismatch && phase == 7 ? 8 : 9;
      p.m_uiHeight = 11;
      p.m_Format   = mismatch && phase == 14 ? xiiGALResourceFormat::RGBA32UInt : xiiGALResourceFormat::RGBA32Float;
      p.m_Color    = xiiColor(value.x, value.y, value.z, value.w);
      xiiVariantDictionary properties;
      XII_TEST_RESULT(p.GetProperties(properties));
      return root.AddPass(name, xiiClearTexturePass().GetDynamicRTTI(), properties);
    };
    const auto normal = clear("Transport normals", phase >= 10 ? xiiVec4(1, 0.5f, 0, 0) : phase == 1 ? xiiVec4(0.5f, 0.5f, 0, 0) :
                                                                                                       xiiVec4(1, 1, 0, 0),
                              true),
               albedo = clear("Transport albedo", xiiVec4(0.5f, 0.25f, 0.1f, 1)), material = clear("Transport material", xiiVec4(0.5f, metallic, 0, transmissionEnabled ? 64.0f / 255 : 0)), emissive = clear("Transport transmission", xiiVec4(0, 0, 0, phase == 4 ? 0.75f : transmission)), transmittance = clear("Unit solar transmittance", xiiVec4(1)), multiScatter = clear("No multiple scattering", xiiVec4::MakeZero());
    const auto background       = root.AddPass("Independent refraction background", xiiScreenTransportTestSource().GetDynamicRTTI());
    const auto suppliedLighting = clear("Supplied screen lighting", xiiVec4(0.2f, 0.4f, 0.6f, 1));
    const auto connect          = [&](xiiUInt32 a, xiiStringView out, xiiUInt32 b, xiiStringView in) {
      XII_TEST_RESULT(root.Connect(a, out, b, in));
    };
    const auto capture = [&](xiiUInt32 node, xiiStringView pin, xiiStringView name) {
      const auto sink = root.AddPass(name, xiiIndirectTestExport().GetDynamicRTTI());
      connect(node, pin, sink, "Input");
      root.m_Interface.PushBack({name, false, root.m_Passes[sink].m_Id, "Output"});
    };
    for (xiiUInt32 branch = 0; branch < 2; ++branch)
    {
      const auto node   = root.AddSubgraph(branch ? "Second transport" : "First transport", loaded->GetAuthoring()->m_Descriptor.m_Id);
      auto&      gather = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      gather.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenEffects/Screen Diffuse Lighting"));
      gather.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenDiffuseLighting/Gather Incident Irradiance"));
      gather.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenIrradiance/Operation"));
      gather.m_Properties.Insert("RayLength", phase >= 9 ? 0.0f : 0.01f);
      gather.m_Properties.Insert("Thickness", 10.0f);
      gather.m_Properties.Insert("StepCount", 1U);
      auto& response = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      response.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenEffects/Screen Diffuse Lighting"));
      response.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenDiffuseLighting/Evaluate Diffuse Material"));
      response.m_Properties.Insert("Scale", branch ? 0.5f : 0.25f);
      auto& refraction = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      refraction.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenEffects/Screen Refraction"));
      refraction.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenRefraction/Sample Background Refraction"));
      refraction.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenRefractionTrace/Operation"));
      refraction.m_Properties.Insert("PerturbScale", 0.5f);
      refraction.m_Properties.Insert("MaximumDistance", 0.2f);
      refraction.m_Properties.Insert("Chromatic", 0.1f);
      xiiAtmosphereConstantsPass atmosphere;
      atmosphere.m_Settings.m_vRayleighScattering    = phase == 9 ? xiiVec3(0.1f, 0.2f, 0.3f) : xiiVec3::MakeZero();
      atmosphere.m_Settings.m_fRayleighScaleHeightKm = 1e9f;
      atmosphere.m_Settings.m_vMieScattering.SetZero();
      atmosphere.m_Settings.m_vMieAbsorption.SetZero();
      atmosphere.m_Settings.m_vOzoneAbsorption.SetZero();
      auto& parameters = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      parameters.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.ScreenEffects/World Atmosphere Parameters"));
      XII_TEST_RESULT(atmosphere.GetProperties(parameters.m_Properties));
      for (const xiiStringView pin : {"Depth", "Radiance", "Camera", "EnvironmentLight"})
        connect(source, pin, node, pin);
      if (phase == 12)
        connect(background, "Radiance", node, "RefractionBackground");
      if (phase == 13)
        connect(suppliedLighting, "Texture", node, "ScreenLighting");
      connect(normal, "Texture", node, "Normal");
      connect(albedo, "Texture", node, "Albedo");
      connect(material, "Texture", node, "Material");
      connect(emissive, "Texture", node, "Emissive");
      connect(transmittance, "Texture", node, "Transmittance");
      connect(multiScatter, "Texture", node, "MultiScatter");
      for (const xiiStringView pin : {"Radiance", "ScreenIrradiance", "ScreenLighting", "RefractionSignal", "TransmittedRadiance"})
        capture(node, pin, xiiStringBuilder(branch ? "B/" : "A/", pin));
    }
    capture(source, "Radiance", "Original");
    xiiStringBuilder name;
    name.SetFormat("Screen transport phase {}", phase);
    const auto fixture = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    for (auto& instance : instances)
      if (!XII_TEST_RESULT(instance.Synchronize(fixture)))
        return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALTexture> outputs[2][2];
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto built = instances[view].Build(nullptr, graph, contexts[view].GetBlackboard());
      if (phase == 7 || phase == 8 || phase == 14)
      {
        XII_TEST_BOOL(built.Failed());
        XII_TEST_BOOL(instances[view].GetLastBuildError().FindSubString("ScreenIrradianceTrace requires matching") != nullptr);
        graph.EndSetup();
        continue;
      }
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[view].GetLastBuildError().GetStartPointer()))
        return;
      XII_TEST_BOOL(instances[view].GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && instances[view].GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
      xiiRenderGraphTextureHandle captured[2][5], original;
      const auto                  binding = [&](xiiStringView pin) {
        return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(pin, false))->m_hTexture;
      };
      original = binding("Original");
      for (xiiUInt32 branch = 0; branch < 2; ++branch)
      {
        xiiUInt32 i = 0;
        for (const xiiStringView pin : {"Radiance", "ScreenIrradiance", "ScreenLighting", "RefractionSignal", "TransmittedRadiance"})
          captured[branch][i++] = binding(xiiStringBuilder(branch ? "B/" : "A/", pin));
      }
      instances[view].Clear();
      graph.EndSetup();
      xiiStringBuilder compileError;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = false}, &compileError), "Phase %u view %u: %s", phase, view, compileError.GetData()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
        return;
      environment.GetDevice()->WaitIdle();
      const auto positive = [](xiiVec3 v) {
        return xiiVec3(xiiMath::Max(v.x, 0.0f), xiiMath::Max(v.y, 0.0f), xiiMath::Max(v.z, 0.0f));
      };
      const auto check = [&](xiiRenderGraphTextureHandle image, xiiVec2U32 pixel, xiiVec4 expected) {
        xiiVec4 actual;
        if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(image).Borrow(), pixel.x, pixel.y, actual))
          return;
        for (xiiUInt32 c = 0; c < 4; ++c)
          XII_TEST_FLOAT(actual.GetData()[c], expected.GetData()[c], 0.0005f + xiiMath::Abs(expected.GetData()[c]) * 0.00001f);
      };
      for (xiiUInt32 branch = 0; branch < 2; ++branch)
      {
        outputs[view][branch] = graph.GetExportedTexture(captured[branch][0]);
        XII_TEST_BOOL(outputs[view][branch] != graph.GetExportedTexture(original));
        for (const xiiVec2U32 pixel : {xiiVec2U32(0, 0), xiiVec2U32(3, 5), xiiVec2U32(4, 5), xiiVec2U32(8, 10)})
        {
          const float u = (pixel.x + 0.5f) / 9, v = (pixel.y + 0.5f) / 11, depth = TransportDepth(phase, pixel.x);
          const auto  sourceColor = TransportRadiance(phase, u, v);
          const auto  sourceValue = positive(sourceColor.GetAsVec3());
          const bool  hit         = phase != 1 && phase != 2 && phase < 9;
          const auto  irradiance  = hit ? sourceValue * xiiMath::Pi<float>() : xiiVec3::MakeZero();
          const auto  diffuse     = phase == 13 ? xiiVec3(0.2f, 0.4f, 0.6f) : hit ? sourceValue.CompMul(xiiVec3(0.5f, 0.25f, 0.1f)) * ((1 - metallic) * (1 - transmission) * (branch ? 0.5f : 0.25f)) :
                                                                                    xiiVec3::MakeZero();
          const auto  base        = sourceValue + diffuse;
          xiiVec3     refracted   = phase == 12 ? xiiVec3(4, 8, 12) : base;
          if (phase >= 10 && phase != 12)
          {
            const float offset      = transmission * 0.5f / (1 + 0.1f / depth * 0.5f);
            float       refractU    = u + offset;
            const auto  sampleDepth = [&](float uv) {
              const float texel = xiiMath::Clamp(uv * 9 - 0.5f, 0.0f, 8.0f);
              const auto  first = static_cast<xiiUInt32>(xiiMath::Floor(texel));
              return xiiMath::Lerp(TransportDepth(phase, first), TransportDepth(phase, xiiMath::Min(first + 1, 8U)), texel - first);
            };
            if (sampleDepth(refractU) > depth)
              refractU = u;
            const float r = xiiMath::Clamp(refractU + offset * 0.1f, 0.5f / 9, 8.5f / 9), g = xiiMath::Clamp(refractU, 0.5f / 9, 8.5f / 9), b = xiiMath::Clamp(refractU - offset * 0.1f, 0.5f / 9, 8.5f / 9);
            refracted = xiiVec3(TransportRadiance(phase, r, v).x, TransportRadiance(phase, g, v).y, TransportRadiance(phase, b, v).z);
          }
          const float weight      = depth > 0 ? transmission : 0;
          const auto  transmitted = refracted * weight;
          auto        result      = base + transmitted;
          if (phase == 9)
          {
            const xiiVec3 toSurface(2 * u - 1, 1 - 2 * v, -1500);
            const float   length = toSurface.GetLength(), cosine = -1500 / length, phaseFunction = 3.0f / (16 * xiiMath::Pi<float>()) * (1 + cosine * cosine);
            for (xiiUInt32 c = 0; c < 3; ++c)
            {
              const float t       = xiiMath::Exp(-(float(c) + 1) * 0.1f * length * 0.001f);
              result.GetData()[c] = base.GetData()[c] * t + 2 * phaseFunction * (1 - t);
            }
          }
          check(original, pixel, sourceColor);
          check(captured[branch][0], pixel, xiiVec4(result, 1));
          check(captured[branch][1], pixel, xiiVec4(irradiance, depth > 0 ? 1.0f : 0.0f));
          check(captured[branch][2], pixel, xiiVec4(diffuse, 1));
          check(captured[branch][3], pixel, weight > 0 ? xiiVec4(refracted, weight) : xiiVec4::MakeZero());
          check(captured[branch][4], pixel, xiiVec4(transmitted, 1));
        }
      }
      XII_TEST_BOOL(outputs[view][0] != outputs[view][1]);
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      if (phase == 0)
        xiiTestFramework::Output(xiiTestOutput::Details, "Screen transport view %u: %u operations, %u submissions, %u barriers.", view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    }
    if (phase != 7 && phase != 8 && phase != 14)
      XII_TEST_BOOL(outputs[0][0] != outputs[1][0] && outputs[0][1] != outputs[1][1]);
    frame.End();
  }
}
