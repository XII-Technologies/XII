/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
class xiiIndirectTestCameraLut : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIndirectTestCameraLut, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin  m_Camera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineTextureOutputPin m_Lut{xiiGALResourceFormat::RG32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto* parameter = context.m_Parameters.GetValue("IndirectCameraZ");
    const float cameraZ   = parameter ? parameter->Get<float>() : 2.0f;
    struct Data
    {
      xiiRenderGraphBufferHandle  m_Camera;
      xiiRenderGraphTextureHandle m_Lut;
      xiiFrameCameraConstants     m_Snapshot;
      xiiVec2                     m_Pixels[16];
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Snapshot=xiiFrameCameraConstants(); data.m_Snapshot.m_CameraPositionAndNearPlane=xiiVec4(0,0,cameraZ,0.1f);
      for(xiiUInt32 y=0;y<4;++y) for(xiiUInt32 x=0;x<4;++x) data.m_Pixels[y*4+x]=xiiVec2(0.1f+0.2f*(x+0.5f)/4,0.05f+0.1f*(y+0.5f)/4);
      xiiGALBufferCreationDescription b; b.m_uiSize=sizeof(xiiFrameCameraConstants); b.m_BindFlags=xiiGALBindFlags::UniformBuffer; b.m_Usage=xiiGALResourceUsage::Dynamic; b.m_CPUAccessFlags=xiiGALCPUAccessFlag::Write; data.m_Camera=builder.WriteBuffer("Analytic camera",b,xiiGALResourceStateFlags::ConstantBuffer);
      xiiGALTextureCreationDescription t; t.m_Type=xiiGALResourceDimension::Texture2D; t.m_Size={4,4}; t.m_Format=xiiGALResourceFormat::RG32Float; t.m_BindFlags=xiiGALBindFlags::ShaderResource; data.m_Lut=builder.WriteTexture("Analytic BRDF lookup",t,xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList(); {xiiGALMapHelper<xiiFrameCameraConstants> mapped(commands,execute.GetBuffer(data.m_Camera),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot;}
      xiiGALStateTransitionDescription t; t.m_pResource=execute.GetBuffer(data.m_Camera); t.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; t.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&t,1));
      xiiGALTextureSubResourceData upload; upload.m_pData=xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Pixels),sizeof(data.m_Pixels)); upload.m_uiStride=4*sizeof(xiiVec2);
      commands.UpdateTexture(execute.GetTexture(data.m_Lut),{},xiiBoundingBoxU32::MakeFromMinMax({0,0,0},{4,4,1}),upload); });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Camera, pass.first->m_Camera));
    return context.SetOutput(m_Lut, pass.first->m_Lut);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIndirectTestCameraLut,1,xiiRTTIDefaultAllocator<xiiIndirectTestCameraLut>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("BRDFLut",m_Lut)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES; } XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
class xiiIndirectTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIndirectTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA32Float, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto pass = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {data=builder.ReadTexture(context.GetInputTexture(m_Input),xiiGALResourceStateFlags::ShaderResource);builder.ExportTexture(data,xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *pass.first);
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIndirectTestExport,1,xiiRTTIDefaultAllocator<xiiIndirectTestExport>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Input",m_Input)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("Output",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES; } XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
namespace
{
  template <typename F> void CheckIndirectImage(xiiGALDevice& device, const xiiSharedPtr<xiiGALTexture>& texture, F&& expected, float absoluteTolerance = 0.0001f)
  {
    if (!XII_TEST_BOOL(texture && texture->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float))
      return;
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture.Borrow()});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(bool(capture)))
      return;
    commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
        {
          const auto* pixel     = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride) + x * 4;
          const auto  reference = expected(x, y);
          for (xiiUInt32 channel = 0; channel < 4; ++channel)
            XII_TEST_FLOAT(pixel[channel], reference.GetData()[channel], xiiMath::Max(absoluteTolerance, xiiMath::Abs(reference.GetData()[channel]) * 0.00001f));
        }
      XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    }
    commands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeIndirectLighting);
XII_CREATE_SIMPLE_TEST(NativeIndirectLighting, LayeredIrradianceMaterialBrdfAndRetiredRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 surfaceAsset = xiiLoadPipelineLibraryResource("SurfaceIndirectLighting"), layeredAsset = xiiLoadPipelineLibraryResource("LayeredDiffuseLighting"), specularAsset = xiiLoadPipelineLibraryResource("EnvironmentSpecular"), layerAsset = xiiLoadPipelineLibraryResource("IrradianceLayer"), diffuseAsset = xiiLoadPipelineLibraryResource("DiffuseSurfaceLighting");
  xiiResourceLock<xiiRenderPipelineResource> surface(surfaceAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), layered(layeredAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), specular(specularAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), layer(layerAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), diffuse(diffuseAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(surface.IsValid() && layered.IsValid() && specular.IsValid() && layer.IsValid() && diffuse.IsValid() && surface->GetAuthoring() && layered->GetAuthoring() && specular->GetAuthoring() && layer->GetAuthoring() && diffuse->GetAuthoring()))
    return;
  const auto sa = surface->GetAuthoring(), la = layered->GetAuthoring(), ea = specular->GetAuthoring();
  xiiUuid    diffuseInstance, specularInstance, diffuseNode;
  for (const auto& node : sa->m_Descriptor.m_Passes)
  {
    if (node.m_Definition == la->m_Descriptor.m_Id)
      diffuseInstance = node.m_Id;
    if (node.m_Definition == ea->m_Descriptor.m_Id)
      specularInstance = node.m_Id;
  }
  for (const auto& node : la->m_Descriptor.m_Passes)
    if (node.m_sType == "xiiDiffuseSurfaceLightingPass")
      diffuseNode = node.m_Id;
  if (!XII_TEST_BOOL(diffuseInstance.IsValid() && specularInstance.IsValid() && diffuseNode.IsValid()))
    return;
  xiiRenderGraphTestContext contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (auto& context : contexts)
    if (!XII_TEST_RESULT(context.Initialize("Explicit indirect surface view")))
      return;
  for (xiiUInt32 phase = 0; phase < 9; ++phase)
  {
    xiiVec4 coarse(1, 2, 3, 0.25f), detail(4, 2, 1, 0.5f), albedo(0.5f, 0.25f, 1, 0.8f), material(0.5f, 0, 0.5f, 0), emission(0, 0, 0, 0), radiance(2, 3, 4, 1);
    float   occlusion = phase == 1 ? 1.0f : 0.5f;
    if (phase == 1)
    {
      coarse.w = 0;
      detail.w = 0;
    }
    if (phase == 2)
    {
      coarse.w   = 2;
      detail.w   = -1;
      material.y = 1;
      material.x = 0.2f;
    }
    if (phase == 3)
    {
      detail.w   = 2;
      material.w = 64.0f / 255;
      emission.w = 0.6f;
      albedo.w   = 0.6f;
    }
    if (phase == 6)
    {
      coarse = xiiVec4(1e8f, 1e8f, 1e8f, 1);
      detail = xiiVec4(2e8f, 2e8f, 2e8f, 1);
    }
    if (phase == 7)
      occlusion = 0;
    if (phase == 8)
    {
      coarse   = xiiVec4(-1, 2, 3, 0.3f);
      detail   = xiiVec4(4, -2, 1, 0.7f);
      radiance = xiiVec4(-1, -2, -3, 1);
    }
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({sa->m_Descriptor.m_Id, surfaceAsset.GetResourceID()});
    root.m_References.PushBack({layer->GetAuthoring()->m_Descriptor.m_Id, layerAsset.GetResourceID()});
    root.m_References.PushBack({diffuse->GetAuthoring()->m_Descriptor.m_Id, diffuseAsset.GetResourceID()});
    const auto clear = [&](xiiStringView name, xiiVec4 color, xiiGALResourceFormat::Enum format = xiiGALResourceFormat::RGBA32Float, bool mismatch = false) {
      xiiClearTexturePass pass;
      pass.m_uiWidth  = mismatch ? 8 : 9;
      pass.m_uiHeight = 11;
      pass.m_Format   = format;
      pass.m_Color    = xiiColor(color.x, color.y, color.z, color.w);
      xiiVariantDictionary p;
      XII_TEST_RESULT(pass.GetProperties(p));
      return root.AddPass(name, "xiiClearTexturePass", p);
    };
    const auto depth = clear("Depth", xiiVec4(phase == 4 ? 0.0f : 0.5f, 0, 0, 0), xiiGALResourceFormat::R32Float), normal = clear("Normal", xiiVec4(0.5f, 0.5f, 0, 0), xiiGALResourceFormat::RG32Float), a = clear("Albedo", albedo), m = clear("Material", material), e = clear("Emissive", emission), c = clear("Coarse irradiance", coarse, xiiGALResourceFormat::RGBA32Float, phase == 5), d = clear("Detail irradiance", detail), r = clear("Filtered environment radiance", radiance);
    const auto camera = root.AddPass("Analytic camera and lookup", "xiiIndirectTestCameraLut"), env = root.AddPass("Ambient coefficient", "xiiEnvironmentLightConstantsPass");
    root.m_Passes[env].m_Properties.Insert("AmbientRadiance", xiiVec3(0.2f, 0.4f, 0.6f));
    const auto first = root.AddSubgraph("First indirect", sa->m_Descriptor.m_Id), second = root.AddSubgraph("Second indirect", sa->m_Descriptor.m_Id);
    auto&      diffuseWeight = root.m_Passes[second].m_Overrides.ExpandAndGetRef();
    diffuseWeight.m_Path.PushBack(diffuseInstance);
    diffuseWeight.m_Path.PushBack(diffuseNode);
    diffuseWeight.m_Properties.Insert("Scale", 2.0f);
    auto& specularWeight = root.m_Passes[second].m_Overrides.ExpandAndGetRef();
    specularWeight.m_Path.PushBack(specularInstance);
    specularWeight.m_Path.PushBack(ea->m_Descriptor.m_Passes[0].m_Id);
    specularWeight.m_Properties.Insert("Scale", 0.5f);
    const auto connect = [&](xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input) {
      XII_TEST_RESULT(root.Connect(source, output, target, input));
    };
    const auto exportImage = [&](xiiUInt32 source, xiiStringView pin, xiiStringView name) {
      const auto output = root.AddPass(name, "xiiIndirectTestExport");
      connect(source, pin, output, "Input");
      root.m_Interface.PushBack({name, false, root.m_Passes[output].m_Id, "Output"});
    };
    const auto ao = phase == 1 ? xiiInvalidIndex : clear("Dynamic occlusion", xiiVec4(occlusion, 0, 0, 0), xiiGALResourceFormat::R32Float);
    for (const auto target : {first, second})
    {
      connect(depth, "Texture", target, "Depth");
      connect(normal, "Texture", target, "Normal");
      connect(a, "Texture", target, "Albedo");
      connect(m, "Texture", target, "Material");
      connect(e, "Texture", target, "Emissive");
      connect(c, "Texture", target, "CoarseIrradiance");
      connect(d, "Texture", target, "DetailIrradiance");
      connect(r, "Texture", target, "EnvironmentRadiance");
      connect(camera, "Camera", target, "Camera");
      connect(camera, "BRDFLut", target, "BRDFLut");
      connect(env, "EnvironmentLight", target, "EnvironmentLight");
      if (ao != xiiInvalidIndex)
        connect(ao, "Texture", target, "Occlusion");
      exportImage(target, "DiffuseLighting", target == first ? "Diffuse0" : "Diffuse1");
      exportImage(target, "EnvironmentSpecular", target == first ? "Specular0" : "Specular1");
      exportImage(target, "IndirectLighting", target == first ? "Indirect0" : "Indirect1");
    }
    const auto standaloneLayer = root.AddSubgraph("Inspect layered confidence", layer->GetAuthoring()->m_Descriptor.m_Id);
    connect(c, "Texture", standaloneLayer, "CoarseIrradiance");
    connect(d, "Texture", standaloneLayer, "DetailIrradiance");
    exportImage(standaloneLayer, "Irradiance", "Layer");
    const auto ambientOnly = root.AddSubgraph("Ambient without irradiance or dynamic occlusion", diffuse->GetAuthoring()->m_Descriptor.m_Id);
    connect(depth, "Texture", ambientOnly, "Depth");
    connect(a, "Texture", ambientOnly, "Albedo");
    connect(m, "Texture", ambientOnly, "Material");
    connect(e, "Texture", ambientOnly, "Emissive");
    connect(env, "EnvironmentLight", ambientOnly, "EnvironmentLight");
    exportImage(ambientOnly, "Lighting", "AmbientOnly");
    xiiStringBuilder resourceName;
    resourceName.SetFormat("Indirect surface phase {}", phase);
    const auto resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(resourceName, std::move(root));
    for (auto& instance : instances)
      if (!XII_TEST_RESULT(instance.Synchronize(resource)))
        return;
    XII_TEST_BOOL(instances[0].GetPipeline() == instances[1].GetPipeline() && instances[0].GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && instances[0].GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& context = contexts[view];
      auto& graph   = context.GetGraph();
      auto& cache   = context.GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const float          cameraZ = view ? 3.5f : 2.0f;
      xiiVariantDictionary parameters;
      parameters.Insert("IndirectCameraZ", cameraZ);
      if (phase == 5)
      {
        XII_TEST_BOOL(instances[view].Build(nullptr, graph, context.GetBlackboard(), parameters).Failed());
        XII_TEST_BOOL(instances[view].GetLastBuildError().FindSubString("IrradianceLayer requires matching") != nullptr);
        graph.EndSetup();
        continue;
      }
      if (!XII_TEST_RESULT(instances[view].Build(nullptr, graph, context.GetBlackboard(), parameters)))
        return;
      const auto binding = [&](xiiStringView name) {
        return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(name, false))->m_hTexture;
      };
      const auto diffuse0 = binding("Diffuse0"), diffuse1 = binding("Diffuse1"), specular0 = binding("Specular0"), specular1 = binding("Specular1"), indirect0 = binding("Indirect0"), indirect1 = binding("Indirect1"), layeredImage = binding("Layer"), ambientImage = binding("AmbientOnly");
      instances[view].Clear();
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
        return;
      environment.GetDevice()->WaitIdle();
      const float detailWeight = xiiMath::Saturate(detail.w), coarseWeight = xiiMath::Saturate(coarse.w) * (1 - detailWeight), confidence = detailWeight + coarseWeight;
      xiiVec3     weighted;
      for (xiiUInt32 k = 0; k < 3; ++k)
        weighted.GetData()[k] = xiiMath::Max(coarse.GetData()[k], 0.0f) * coarseWeight + xiiMath::Max(detail.GetData()[k], 0.0f) * detailWeight;
      const xiiVec3 combined     = confidence > 0 ? weighted / confidence : xiiVec3::MakeZero();
      const xiiVec3 incident     = xiiVec3(0.2f, 0.4f, 0.6f) * (1 - confidence) + weighted;
      const float   transmission = phase == 3 ? 0.6f : 0.0f, visibility = xiiMath::Saturate(albedo.w * occlusion);
      const auto    diffuseValue = [&](bool ambient) {
        xiiVec3 value;
        for (xiiUInt32 k = 0; k < 3; ++k)
          value.GetData()[k] = (ambient ? xiiVec3(0.2f, 0.4f, 0.6f).GetData()[k] : incident.GetData()[k]) * albedo.GetData()[k] * (1 - material.y) * (1 - transmission) * (ambient ? albedo.w : visibility) / xiiMath::Pi<float>();
        return phase == 4 ? xiiVec3::MakeZero() : value;
      };
      const auto specularValue = [&](xiiUInt32 x, xiiUInt32 y) {
        if (phase == 4)
          return xiiVec3::MakeZero();
        const xiiVec3 world(2 * (x + 0.5f) / 9 - 1, 1 - 2 * (y + 0.5f) / 11, 0.5f);
        const float   NoV = (cameraZ - 0.5f) / (xiiVec3(0, 0, cameraZ) - world).GetLength(), roughness = xiiMath::Max(material.x, 0.02f);
        const float   brdfX = 0.1f + 0.2f * xiiMath::Clamp(NoV, 0.125f, 0.875f), brdfY = 0.05f + 0.1f * xiiMath::Clamp(roughness, 0.125f, 0.875f), fresnelPower = xiiMath::Pow(1 - NoV, 5.0f);
        xiiVec3       value;
        for (xiiUInt32 k = 0; k < 3; ++k)
        {
          const float f0     = xiiMath::Lerp(0.08f * material.z, albedo.GetData()[k], material.y);
          const float f      = f0 + (xiiMath::Max(1 - roughness, f0) - f0) * fresnelPower;
          value.GetData()[k] = xiiMath::Max(radiance.GetData()[k], 0.0f) * (f * brdfX + brdfY) * visibility;
        }
        return value;
      };
      const auto rgba = [](xiiVec3 value, float alpha = 1.0f) {
        return xiiVec4(value, alpha);
      };
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(layeredImage), [&](xiiUInt32, xiiUInt32) { return rgba(combined, confidence); });
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(ambientImage), [&](xiiUInt32, xiiUInt32) { return rgba(diffuseValue(true)); });
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(diffuse0), [&](xiiUInt32, xiiUInt32) { return rgba(diffuseValue(false)); });
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(diffuse1), [&](xiiUInt32, xiiUInt32) { return rgba(diffuseValue(false) * 2.0f); });
      // Hardware bilinear interpolation has finite subtexel precision.
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(specular0), [&](xiiUInt32 x, xiiUInt32 y) { return rgba(specularValue(x, y)); }, 0.0005f);
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(specular1), [&](xiiUInt32 x, xiiUInt32 y) { return rgba(specularValue(x, y) * 0.5f); }, 0.0005f);
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(indirect0), [&](xiiUInt32 x, xiiUInt32 y) { return rgba(diffuseValue(false) + specularValue(x, y)); }, 0.0005f);
      CheckIndirectImage(*environment.GetDevice(), graph.GetExportedTexture(indirect1), [&](xiiUInt32 x, xiiUInt32 y) { return rgba(diffuseValue(false) * 2.0f + specularValue(x, y) * 0.5f); }, 0.0005f);
      XII_TEST_BOOL(graph.GetExportedTexture(diffuse0) != graph.GetExportedTexture(diffuse1));
      if (phase == 0)
      {
        XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
        xiiTestFramework::Output(xiiTestOutput::Details, "Indirect surface view %u: %u operations, %u submissions, %u barriers.", view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
      }
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    }
    frame.End();
  }
}
