/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <Shaders/Pipeline/Passes/DDGI/DDGIConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
#include <Shaders/Pipeline/Passes/SparseVoxelRadiance/SparseVoxelRadianceConstants.h>

class xiiOpaqueTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto input = context.GetInputTexture(m_Input);
    if (!input.IsValid())
      return XII_SUCCESS;
    const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& data, xiiRenderGraphBuilder& builder) {
      data = builder.ReadTexture(input, xiiGALResourceStateFlags::CopySource);
      builder.ExportTexture(data, xiiGALResourceStateFlags::CopySource); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Output, *emitted.first);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpaqueTestExport, 1, xiiRTTIDefaultAllocator<xiiOpaqueTestExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  void CheckOpaquePixels(xiiGPUTestingEnvironment& environment, const xiiSharedPtr<xiiGALTexture>& texture, const xiiColor& expected)
  {
    xiiGALTextureReadback readback(environment.GetDevice());
    auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture.Borrow()});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return;
    auto* queue = environment.GetDevice()->GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return;
    commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        const bool  fullPrecision = texture->GetDescription().m_Format == xiiGALResourceFormat::RGBA32Float;
        const auto* row           = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
        const auto  value         = [&](xiiUInt32 i) {
          return fullPrecision ? reinterpret_cast<const float*>(row)[i] : float(reinterpret_cast<const xiiFloat16*>(row)[i]);
        };
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
        {
          XII_TEST_FLOAT(value(x * 4), expected.r, 0.01f);
          XII_TEST_FLOAT(value(x * 4 + 1), expected.g, 0.01f);
          XII_TEST_FLOAT(value(x * 4 + 2), expected.b, 0.01f);
          XII_TEST_FLOAT(value(x * 4 + 3), expected.a, 0.01f);
        }
      }
      XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    }
    commands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentOpaqueAndReflectionOperations)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  const auto                          clear = [&](xiiStringView name, xiiColor color, xiiGALResourceFormat::Enum format = xiiGALResourceFormat::RGBA16Float) {
    xiiClearTexturePass pass;
    pass.m_Color  = color;
    pass.m_Format = format;
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    return leaf.AddPass(name, "xiiClearTexturePass", properties);
  };
  const auto                                 direct              = clear("Direct", {1, 2, 3, 1});
  const auto                                 indirect            = clear("Indirect", {0.5f, 0.25f, 1, 1});
  const auto                                 emissive            = clear("Emissive", {0.1f, 0.2f, 0.3f, 1});
  const auto                                 gi                  = clear("Near GI", {0.4f, 0.8f, 1.2f, 0.5f});
  const auto                                 volume              = clear("Volume", {0.05f, 0.1f, 0.15f, 0.25f});
  const auto                                 environmentMap      = clear("Environment", {0.2f, 0.4f, 0.6f, 1});
  const auto                                 traced              = clear("Traced reflections", {0.4f, 0.8f, 1.2f, 0.25f});
  const auto                                 radiance            = clear("Screen reflection signal", {0.8f, 0.4f, 0.2f, 0.5f});
  const auto                                 depth               = clear("Depth", xiiColor::MakeZero(), xiiGALResourceFormat::R32Float);
  const auto                                 attributes          = clear("Material", {1, 0, 0, 0}); // Roughness rejects screen traversal deterministically.
  const auto                                 surface             = clear("Previous surface", xiiColor::MakeZero());
  const auto                                 objects             = clear("Object identity", xiiColor::MakeZero(), xiiGALResourceFormat::R32UInt);
  const auto                                 camera              = leaf.AddPass("Camera", "xiiViewConstantsPass");
  const auto                                 compositionResource = xiiLoadPipelineLibraryResource("OpaqueComposition");
  xiiResourceLock<xiiRenderPipelineResource> composition(compositionResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(composition.IsValid() && composition->GetAuthoring()))
    return;
  const auto compositionId = composition->GetAuthoring()->m_Descriptor.m_Id;
  leaf.m_References.PushBack({compositionId, compositionResource.GetResourceID()});
  const auto opaque  = leaf.AddSubgraph("Authored opaque and reflection composition", compositionId);
  const auto connect = [&](xiiUInt32 source, xiiStringView pin) {
    XII_TEST_RESULT(leaf.Connect(source, "Texture", opaque, pin));
  };
  connect(direct, "DirectLighting");
  connect(indirect, "IndirectLighting");
  connect(emissive, "GBufferEmissive");
  connect(gi, "RTFinalGI");
  connect(volume, "VolumetricScattering");
  connect(environmentMap, "EnvironmentSpecular");
  connect(traced, "RTFinalReflections");
  connect(depth, "SceneDepth");
  connect(attributes, "GBufferNormal");
  connect(attributes, "GBufferMaterial");
  connect(attributes, "GBufferAlbedo");
  connect(attributes, "BRDFLut");
  connect(depth, "HiZClosestPyramid");
  connect(depth, "HiZPyramid");
  connect(radiance, "SSRRawTerm");
  connect(attributes, "DilatedVelocityBuffer");
  connect(objects, "GBufferObjectId");
  connect(surface, "ReSTIRDISurfacePrevious");
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", opaque, "PerFrameCamera"));
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameGlobal", opaque, "PerFrameGlobal"));
  const auto exportPin = [&](xiiStringView name, xiiUInt32 source, xiiStringView pin) {
    const auto output = leaf.AddPass(name, "xiiOpaqueTestExport");
    XII_TEST_RESULT(leaf.Connect(source, pin, output, "Input"));
    return leaf.m_Passes[output].m_Id;
  };
  const auto         outputId   = exportPin("Composite readback", opaque, "HDRSceneColor");
  const auto         traceId    = exportPin("Selected raw readback", opaque, "SSRRawTerm");
  const auto         historyId  = exportPin("Current reflection readback", opaque, "SSRTerm");
  const auto         previousId = exportPin("Previous reflection readback", opaque, "PreviousReflection");
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.m_References = leaf.m_References;
  root.AddSubgraph("First reflection instance", definition.m_Id);
  root.AddSubgraph("Second reflection instance", definition.m_Id);
  const auto secondId = root.m_Passes[1].m_Id;
  // One occurrence uses the authored screen trace, the other an explicitly supplied raw signal.
  auto& rawOverride = root.m_Passes[0].m_Overrides.ExpandAndGetRef();
  rawOverride.m_Path.PushBack(leaf.m_Passes[opaque].m_Id);
  rawOverride.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.OpaqueComposition/Select Raw Reflections"));
  rawOverride.m_Properties.Insert("PreferPrimary", false);
  auto& override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[radiance].m_Id);
  xiiClearTexturePass second;
  second.m_Color = {0.4f, 0.2f, 0.1f, 0.5f};
  XII_TEST_RESULT(second.GetProperties(override.m_Properties));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent opaque reflection operations", std::move(root));
  xiiWorldDescription description("No scene extraction for composition");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  auto*      module     = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*   view       = nullptr;
  const auto viewHandle = module->CreateView("Opaque composition only", view);
  view->SetCamera(&viewCamera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  xiiSharedPtr<xiiGALTexture> retained[2];
  for (xiiUInt32 frame = 0; frame < 4; ++frame)
  {
    if (frame == 2)
    {
      view->SetViewport({0, 0, 16, 8});
      view->InvalidateTemporalHistory();
    }
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* compiled = instance.GetPipeline();
    if (!XII_TEST_BOOL(compiled != nullptr))
      return;
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    xiiSharedPtr<xiiGALTexture> histories[2];
    xiiUInt32                   count = 0;
    for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
    {
      const auto& provenance = compiled->GetProvenance()[i];
      if (provenance.m_Element != outputId && provenance.m_Element != traceId && provenance.m_Element != historyId && provenance.m_Element != previousId)
        continue;
      const bool  other   = provenance.m_InstancePath[0] == secondId;
      const auto* binding = instance.GetOutputBinding(i, "Output");
      if (provenance.m_Element == previousId && frame % 2 == 0)
      {
        XII_TEST_BOOL(binding && !binding->m_hTexture.IsValid());
        continue;
      }
      auto texture = binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      if (!XII_TEST_BOOL(texture != nullptr))
        return;
      XII_TEST_INT(texture->GetDescription().m_Size.width, frame < 2 ? 8 : 16);
      if (provenance.m_Element == outputId)
        CheckOpaquePixels(environment, texture, other ? xiiColor(1.525f, 2.1875f, 3.65f, 1) : xiiColor(1.45f, 2.3375f, 3.975f, 1));
      else
        CheckOpaquePixels(environment, texture, other ? xiiColor(0.4f, 0.2f, 0.1f, 0.5f) : xiiColor::MakeZero());
      if (provenance.m_Element == historyId)
      {
        histories[other ? 1 : 0] = texture;
        ++count;
        if (frame == 1)
          retained[other ? 1 : 0] = texture;
      }
    }
    XII_TEST_INT(count, 2);
    XII_TEST_BOOL(histories[0] != histories[1]);
  }
  module->DestroyView(viewHandle);
  for (const auto& texture : retained)
  {
    XII_TEST_INT(texture->GetDescription().m_Size.width, 8);
    XII_TEST_INT(texture->GetRefCount(), 1);
  }
}

class xiiTransparencyTestHDR : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparencyTestHDR, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA16Float};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [](auto& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D;
      description.m_Size = {9, 11};
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data = builder.WriteTexture("HDR target", description, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data)->GetDefaultView(xiiGALTextureViewType::RenderTarget), {2, 4, 6, 0.5f}); });
    return context.SetOutput(m_Output, *emitted.first);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparencyTestHDR, 1, xiiRTTIDefaultAllocator<xiiTransparencyTestHDR>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Texture", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentWeightedTransparencyResolve)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          clear = [&](xiiStringView name, xiiColor color, xiiGALResourceFormat::Enum format) {
    xiiClearTexturePass pass;
    pass.m_uiWidth  = 9;
    pass.m_uiHeight = 11;
    pass.m_Color    = color;
    pass.m_Format   = format;
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    return descriptor.AddPass(name, "xiiClearTexturePass", properties);
  };
  const auto accumulation = clear("Weighted color", {1, 2, 3, 2}, xiiGALResourceFormat::RGBA16Float);
  const auto revealage    = clear("Coverage", {0.25f, 0, 0, 0}, xiiGALResourceFormat::R8UNormalized);
  const auto hdr          = descriptor.AddPass("Opaque radiance", "xiiTransparencyTestHDR");
  const auto resolve      = descriptor.AddPass("Resolve", "xiiWeightedBlendedOITResolvePass");
  const auto output       = descriptor.AddPass("Read color", "xiiOpaqueTestExport");
  XII_TEST_RESULT(descriptor.Connect(accumulation, "Texture", resolve, "OITAccumulate"));
  XII_TEST_RESULT(descriptor.Connect(revealage, "Texture", resolve, "OITReveal"));
  XII_TEST_RESULT(descriptor.Connect(hdr, "Texture", resolve, "HDRSceneColor"));
  const auto                                 authoredResource = xiiLoadPipelineLibraryResource("TransparencyResolve");
  xiiResourceLock<xiiRenderPipelineResource> authored(authoredResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(authored.IsValid() && authored->GetAuthoring()))
    return;
  const auto definitionId = authored->GetAuthoring()->m_Descriptor.m_Id;
  descriptor.m_References.PushBack({definitionId, authoredResource.GetResourceID()});
  const auto subgraph = descriptor.AddSubgraph("Authored transparency", definitionId);
  XII_TEST_RESULT(descriptor.Connect(resolve, "HDRSceneColor", subgraph, "HDRSceneColor"));
  XII_TEST_RESULT(descriptor.Connect(subgraph, "HDRSceneColor", output, "Input"));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Weighted transparency supplied targets", std::move(descriptor));
  xiiWorldDescription description("Resolve without scene extraction");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view       = nullptr;
  const auto viewHandle = module->CreateView("Transparency resolve", view);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 9, 11}); // Non-divisible dispatch dimensions exercise bounds checks.
  view->SetRenderPipelineResource(resource);
  world.Update();
  environment.GetDevice()->WaitIdle();
  const auto& instance = view->GetRenderPipelineInstance();
  if (!XII_TEST_BOOL(instance.GetPipeline() != nullptr))
    return;
  XII_TEST_BOOL(instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(instance.GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  const auto* binding = instance.GetOutputBinding(output, "Output");
  const auto  texture = binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  const float reveal = 64.0f / 255.0f;
  CheckOpaquePixels(environment, texture, {0.5f * (1 - reveal) + 2 * reveal, 1 * (1 - reveal) + 4 * reveal, 1.5f * (1 - reveal) + 6 * reveal, 0.5f});
  module->DestroyView(viewHandle);
}

namespace
{
  void CheckAmbientMask(xiiGPUTestingEnvironment& environment, const xiiSharedPtr<xiiGALTexture>& texture, float expected)
  {
    xiiGALTextureReadback readback(environment.GetDevice());
    auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto enqueued = readback.Enqueue(commands, {.m_pTexture = texture.Borrow()});
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return;
    auto* queue = environment.GetDevice()->GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return;
    commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
          XII_TEST_FLOAT(row[x] / 255.0f, expected, 1.1f / 255.0f);
      }
      XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    }
    commands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentAmbientOcclusionHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  const auto                          clear = [&](xiiStringView name, xiiColor color, xiiGALResourceFormat::Enum format) {
    xiiClearTexturePass pass;
    pass.m_Color  = color;
    pass.m_Format = format;
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    return leaf.AddPass(name, "xiiClearTexturePass", properties);
  };
  const auto           emptyDepth = clear("Empty depth", xiiColor::MakeZero(), xiiGALResourceFormat::R32Float);
  const auto           depth      = clear("Surface depth", {0.5f, 0, 0, 0}, xiiGALResourceFormat::R32Float);
  const auto           normal     = clear("Normals", {0.5f, 0.5f, 0, 0}, xiiGALResourceFormat::RGBA16Float);
  const auto           velocity   = clear("Velocity", xiiColor::MakeZero(), xiiGALResourceFormat::RG16Float);
  const auto           signal     = clear("Test signal", {0.25f, 0, 0, 0}, xiiGALResourceFormat::R8UNormalized);
  const auto           camera     = leaf.AddPass("Camera", "xiiViewConstantsPass");
  xiiVariantDictionary screenProperties;
  screenProperties.Insert("UseRayTracing", false);
  const auto generate = leaf.AddPass("Generate", "xiiAmbientOcclusionPass", screenProperties);
  const auto temporal = leaf.AddPass("Filter", "xiiAmbientOcclusionTemporalPass");
  XII_TEST_RESULT(leaf.Connect(emptyDepth, "Texture", generate, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(normal, "Texture", generate, "NormalRoughness"));
  XII_TEST_RESULT(leaf.Connect(signal, "Texture", temporal, "RawAO"));
  XII_TEST_RESULT(leaf.Connect(depth, "Texture", temporal, "SceneDepth"));
  XII_TEST_RESULT(leaf.Connect(normal, "Texture", temporal, "GBufferNormal"));
  XII_TEST_RESULT(leaf.Connect(velocity, "Texture", temporal, "DilatedVelocityBuffer"));
  for (auto operation : {generate, temporal})
    for (auto pin : {"PerFrameCamera", "PerFrameGlobal"})
      XII_TEST_RESULT(leaf.Connect(camera, pin, operation, pin));
  const auto output = [&](xiiStringView name, xiiUInt32 source, xiiStringView pin) {
    const auto node = leaf.AddPass(name, "xiiOpaqueTestExport");
    XII_TEST_RESULT(leaf.Connect(source, pin, node, "Input"));
    return leaf.m_Passes[node].m_Id;
  };
  const auto         generated = output("Generated AO", generate, "RawAO");
  const auto         current   = output("Filtered AO", temporal, "StableAO");
  const auto         previous  = output("Previous AO", temporal, "PreviousAO");
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First AO instance", definition.m_Id);
  root.AddSubgraph("Second AO instance", definition.m_Id);
  const auto secondId = root.m_Passes[1].m_Id;
  auto&      override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[signal].m_Id);
  xiiClearTexturePass changed;
  changed.m_Format = xiiGALResourceFormat::R8UNormalized;
  changed.m_Color  = {0.75f, 0, 0, 0};
  XII_TEST_RESULT(changed.GetProperties(override.m_Properties));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("AO instance histories", std::move(root));
  xiiWorldDescription description("AO without world extraction");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView* views[2] = {};
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    module->CreateView("Independent AO view", views[i]);
    views[i]->SetCamera(&viewCamera);
    views[i]->SetViewport(i == 0 ? xiiRectFloat(0, 0, 10, 12) : xiiRectFloat(0, 0, 14, 8));
    views[i]->SetRenderPipelineResource(resource);
  }
  xiiSharedPtr<xiiGALTexture> last[2][2];
  for (xiiUInt32 frame = 0; frame < 6; ++frame)
  {
    if (frame == 2)
    {
      views[0]->SetViewport({0, 0, 16, 10});
      views[0]->InvalidateTemporalHistory();
    }
    world.Update();
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      const auto& instance = views[view]->GetRenderPipelineInstance();
      const auto* compiled = instance.GetPipeline();
      if (!XII_TEST_BOOL(compiled != nullptr))
        return;
      XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet() && compiled->GetRuntimeFeatures().IsNoFlagSet());
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
      bool waitsForHistory = false;
      for (const auto& submission : views[view]->GetRenderGraph()->GetQueueSubmissions())
        waitsForHistory |= !submission.m_WaitFences.IsEmpty();
      XII_TEST_BOOL(waitsForHistory == (frame != 0 && !(view == 0 && frame == 2)));
      xiiSharedPtr<xiiGALTexture> next[2];
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        const auto& provenance = compiled->GetProvenance()[i];
        if (provenance.m_Element != generated && provenance.m_Element != current && provenance.m_Element != previous)
          continue;
        const auto  occurrence  = provenance.m_InstancePath[0] == secondId ? 1U : 0U;
        const auto* binding     = instance.GetOutputBinding(i, "Output");
        const bool  hasPrevious = frame != 0 && !(view == 0 && frame == 2);
        if (provenance.m_Element == previous && !hasPrevious)
        {
          XII_TEST_BOOL(binding && !binding->m_hTexture.IsValid());
          continue;
        }
        const auto texture = binding ? views[view]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
        if (!XII_TEST_BOOL(texture != nullptr))
          return;
        if (provenance.m_Element == current)
        {
          XII_TEST_BOOL(texture != last[view][occurrence]);
          next[occurrence] = texture;
        }
        if (provenance.m_Element == previous)
          XII_TEST_BOOL(texture == last[view][occurrence]);
        CheckAmbientMask(environment, texture, provenance.m_Element == generated ? 1.0f : occurrence == 0 ? 0.25f :
                                                                                                            0.75f);
      }
      XII_TEST_BOOL(next[0] && next[1] && next[0] != next[1]);
      last[view][0] = next[0];
      last[view][1] = next[1];
    }
    XII_TEST_BOOL(last[0][0] != last[1][0] && last[0][1] != last[1][1]);
  }
  for (auto* view : views)
    module->DestroyView(view->GetHandle());
}

/// Synthetic GI preparation supplies initialized resources through graph-owned uploads.
/// This lets gather operations run without any light manager, residency or extraction.
class xiiIrradianceTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIrradianceTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Light{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_ProbeConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_States{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_PageTable{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Levels{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Pool{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_VoxelConstants{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Light, m_ProbeConstants, m_States, m_PageTable, m_Levels, m_Pool, m_VoxelConstants;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [](Data& data, xiiRenderGraphBuilder& builder) {
      const auto create = [&](xiiStringView name, xiiUInt32 size, xiiUInt32 stride = 0) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = size;
        description.m_uiElementByteStride = stride;
        description.m_Mode = stride ? xiiGALBufferMode::Structured : xiiGALBufferMode::Undefined;
        description.m_BindFlags = stride ? xiiGALBindFlags::ShaderResource : xiiGALBindFlags::UniformBuffer;
        description.m_Usage = xiiGALResourceUsage::Dynamic;
        description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
        return builder.WriteBuffer(name, description, stride ? xiiGALResourceStateFlags::ShaderResource : xiiGALResourceStateFlags::ConstantBuffer);
      };
      data.m_Light = create("Test Light", sizeof(xiiEnvironmentLightConstants));
      data.m_ProbeConstants = create("Test Probe Constants", sizeof(xiiDDGIProbeConstants));
      data.m_States = create("Test Probes", sizeof(xiiGpuDDGIProbeState), sizeof(xiiGpuDDGIProbeState));
      data.m_PageTable = create("Test Pages", 8 * sizeof(xiiUInt32), sizeof(xiiUInt32));
      data.m_Levels = create("Test Levels", sizeof(xiiGpuSparseVoxelLevel), sizeof(xiiGpuSparseVoxelLevel));
      data.m_Pool = create("Test Radiance", 8 * sizeof(xiiVec4), sizeof(xiiVec4));
      data.m_VoxelConstants = create("Test Voxel Constants", sizeof(xiiSparseVoxelRadianceConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiEnvironmentLightConstants> mapped(commands, execute.GetBuffer(data.m_Light), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = xiiEnvironmentLightConstants();
        mapped->EnvironmentAmbientRadiance = xiiVec4(0.3f, 0.4f, 0.5f, 1);
      }
      {
        xiiGALMapHelper<xiiDDGIProbeConstants> mapped(commands, execute.GetBuffer(data.m_ProbeConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        mapped->MinimumCellAndAtlasWidth = xiiVec4I32(0, 0, 0, 1);
        mapped->ProbeCountsAndUpdateCount = xiiVec4U32(1, 1, 1, 0);
        mapped->SpacingHysteresisDistance = xiiVec4(4, 0.97f, 16, 0);
      }
      {
        xiiGALMapHelper<xiiGpuDDGIProbeState> mapped(commands, execute.GetBuffer(data.m_States), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        mapped->m_vPositionAndLastUpdate = xiiVec4::MakeZero();
        mapped->m_vCellAndFlags = xiiVec4I32::MakeZero(); // No valid probe: use explicit ambient fallback.
      }
      {
        xiiGALMapHelper<xiiUInt32> mapped(commands, execute.GetBuffer(data.m_PageTable), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        xiiMemoryUtils::ZeroFill(mapped.GetMappedData(), 8);
      }
      {
        xiiGALMapHelper<xiiGpuSparseVoxelLevel> mapped(commands, execute.GetBuffer(data.m_Levels), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        mapped->m_vMinimumCellAndPageOffset = xiiVec4I32(-1, -1, -1, 0);
        mapped->m_vVoxelAndBrickSize = xiiVec4(5000, 10000, 0, 0);
      }
      {
        xiiGALMapHelper<xiiVec4> mapped(commands, execute.GetBuffer(data.m_Pool), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        for (xiiUInt32 i = 0; i < 8; ++i) mapped.GetMappedData()[i] = xiiVec4(2, 4, 8, 1);
      }
      {
        xiiGALMapHelper<xiiSparseVoxelRadianceConstants> mapped(commands, execute.GetBuffer(data.m_VoxelConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        mapped->ClipmapAndUpdateCounts = xiiVec4U32(1, 2, 2, 0);
        mapped->PoolLayout = xiiVec4U32(8, 1, 0, 0);
        mapped->RadianceSettings = xiiVec4::MakeZero();
      } });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Light, emitted.first->m_Light));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_ProbeConstants, emitted.first->m_ProbeConstants));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_States, emitted.first->m_States));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_PageTable, emitted.first->m_PageTable));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Levels, emitted.first->m_Levels));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Pool, emitted.first->m_Pool));
    return context.SetOutput(m_VoxelConstants, emitted.first->m_VoxelConstants);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIrradianceTestSource, 1, xiiRTTIDefaultAllocator<xiiIrradianceTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("EnvironmentLight", m_Light)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("DDGIProbeConstants", m_ProbeConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("DDGIProbeStates", m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("SparseVoxelPageTable", m_PageTable)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("SparseVoxelLevelData", m_Levels)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("SparseVoxelRadiancePool", m_Pool)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("SparseVoxelConstants", m_VoxelConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(PipelineResources, IndependentIrradianceGathers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  const auto                          clear = [&](xiiStringView name, xiiColor color, xiiGALResourceFormat::Enum format) {
    xiiClearTexturePass pass;
    pass.m_Color  = color;
    pass.m_Format = format;
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass.GetProperties(properties));
    return leaf.AddPass(name, "xiiClearTexturePass", properties);
  };
  const auto depth           = clear("Surface Depth", {0.5f, 0, 0, 0}, xiiGALResourceFormat::R32Float);
  const auto normal          = clear("Surface Normal", {0.5f, 0.5f, 0, 0}, xiiGALResourceFormat::RG16Float);
  const auto irradianceAtlas = clear("Irradiance Atlas", xiiColor::MakeZero(), xiiGALResourceFormat::RGBA16Float);
  const auto distanceAtlas   = clear("Distance Atlas", xiiColor::MakeZero(), xiiGALResourceFormat::RG16Float);
  const auto resources       = leaf.AddPass("Prepared GI", "xiiIrradianceTestSource");
  const auto camera          = leaf.AddPass("Camera", "xiiViewConstantsPass");
  const auto ddgi            = leaf.AddPass("DDGI", "xiiDDGIProbeGatherPass");
  const auto sparse          = leaf.AddPass("Sparse", "xiiSparseVoxelGatherPass");
  for (auto operation : {ddgi, sparse})
  {
    XII_TEST_RESULT(leaf.Connect(depth, "Texture", operation, "SceneDepth"));
    XII_TEST_RESULT(leaf.Connect(normal, "Texture", operation, "GBufferNormal"));
    XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", operation, "PerFrameCamera"));
  }
  XII_TEST_RESULT(leaf.Connect(irradianceAtlas, "Texture", ddgi, "DDGIProbeIrradianceAtlas"));
  XII_TEST_RESULT(leaf.Connect(distanceAtlas, "Texture", ddgi, "DDGIProbeDistanceAtlas"));
  for (auto pin : {"DDGIProbeStates", "DDGIProbeConstants", "EnvironmentLight"})
    XII_TEST_RESULT(leaf.Connect(resources, pin, ddgi, pin));
  for (auto pin : {"SparseVoxelPageTable", "SparseVoxelLevelData", "SparseVoxelRadiancePool", "SparseVoxelConstants"})
    XII_TEST_RESULT(leaf.Connect(resources, pin, sparse, pin));
  const auto ddgiExport   = leaf.AddPass("Read DDGI", "xiiOpaqueTestExport");
  const auto sparseExport = leaf.AddPass("Read Sparse", "xiiOpaqueTestExport");
  XII_TEST_RESULT(leaf.Connect(ddgi, "DDGIIrradiance", ddgiExport, "Input"));
  XII_TEST_RESULT(leaf.Connect(sparse, "SparseVoxelIrradiance", sparseExport, "Input"));
  const auto         ddgiId = leaf.m_Passes[ddgiExport].m_Id, sparseId = leaf.m_Passes[sparseExport].m_Id;
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First Gather", definition.m_Id);
  root.AddSubgraph("Second Gather", definition.m_Id);
  const auto secondId     = root.m_Passes[1].m_Id;
  auto&      ddgiOverride = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  ddgiOverride.m_Path.PushBack(leaf.m_Passes[ddgi].m_Id);
  ddgiOverride.m_Properties.Insert("Intensity", 2.0f);
  auto& sparseOverride = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  sparseOverride.m_Path.PushBack(leaf.m_Passes[sparse].m_Id);
  sparseOverride.m_Properties.Insert("Intensity", 0.25f);
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Independent irradiance algorithms", std::move(root));
  xiiWorldDescription description("GI gathers without scene features");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera viewCamera;
  viewCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view   = nullptr;
  const auto handle = module->CreateView("GI Gather", view);
  view->SetCamera(&viewCamera);
  view->SetViewport({0, 0, 10, 12});
  view->SetRenderPipelineResource(resource);
  for (xiiUInt32 frame = 0; frame < 2; ++frame)
  {
    world.Update();
    const auto& instance = view->GetRenderPipelineInstance();
    const auto* compiled = instance.GetPipeline();
    if (!XII_TEST_BOOL(compiled != nullptr))
      return;
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    xiiUInt32 outputs = 0;
    for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
    {
      const auto& provenance = compiled->GetProvenance()[i];
      if (provenance.m_Element != ddgiId && provenance.m_Element != sparseId)
        continue;
      const bool  other   = provenance.m_InstancePath[0] == secondId;
      const auto* binding = instance.GetOutputBinding(i, "Output");
      const auto  texture = binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
      if (!XII_TEST_BOOL(texture != nullptr))
        return;
      CheckOpaquePixels(environment, texture, provenance.m_Element == ddgiId ? (other ? xiiColor(0.6f, 0.8f, 1, 0) : xiiColor(0.3f, 0.4f, 0.5f, 0)) : (other ? xiiColor(0.5f, 1, 2, 1) : xiiColor(2, 4, 8, 1)));
      ++outputs;
    }
    XII_TEST_INT(outputs, 4);
  }
  // Compile and record a final independent graph after both gather instances and their view retire.
  xiiRenderGraphContext     queued;
  xiiRenderPipelineInstance detached;
  if (!XII_TEST_RESULT(queued.Initialize("Retained irradiance gathers")) || !XII_TEST_RESULT(detached.Synchronize(resource)))
    return;
  xiiRenderGraphFrame queuedFrame;
  if (!XII_TEST_RESULT(queuedFrame.Begin()))
    return;
  auto& graph = queued.GetGraph();
  auto& cache = queued.GetResourceCache();
  cache.BeginFrame(queuedFrame.GetFrameIndex(), queuedFrame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(queuedFrame.GetFrameIndex());
  if (!XII_TEST_RESULT(detached.Build(view, graph, queued.GetBlackboard())))
    return;
  xiiDynamicArray<xiiRenderGraphTextureHandle> retained;
  xiiDynamicArray<xiiColor>                    expected;
  const auto*                                  compiled = detached.GetPipeline();
  for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
  {
    const auto& provenance = compiled->GetProvenance()[i];
    if (provenance.m_Element != ddgiId && provenance.m_Element != sparseId)
      continue;
    const auto* binding = detached.GetOutputBinding(i, "Output");
    if (!XII_TEST_BOOL(binding && binding->m_hTexture.IsValid()))
      return;
    retained.PushBack(binding->m_hTexture);
    const bool other = provenance.m_InstancePath[0] == secondId;
    expected.PushBack(provenance.m_Element == ddgiId ? (other ? xiiColor(0.6f, 0.8f, 1, 0) : xiiColor(0.3f, 0.4f, 0.5f, 0)) : (other ? xiiColor(0.5f, 1, 2, 1) : xiiColor(2, 4, 8, 1)));
  }
  detached.Clear();
  module->DestroyView(handle);
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &queued.GetBlackboard(), &cache)))
    return;
  queuedFrame.End();
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(retained.GetCount(), 4);
  for (xiiUInt32 i = 0; i < retained.GetCount(); ++i)
    CheckOpaquePixels(environment, graph.GetExportedTexture(retained[i]), expected[i]);
}

XII_CREATE_SIMPLE_TEST_GROUP(NativeRadianceComposition);
XII_CREATE_SIMPLE_TEST(NativeRadianceComposition, IndependentWeightsFogRangeAndRetiredRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("SurfaceRadianceComposition");
  xiiResourceLock<xiiRenderPipelineResource> composition(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(composition.IsValid() && composition->GetAuthoring()))
    return;
  const auto authored = composition->GetAuthoring();
  xiiUuid    directNode;
  for (const auto& boundary : authored->m_Descriptor.m_Interface)
    if (boundary.m_bInput && boundary.m_sName == "DirectLighting")
      directNode = boundary.m_Node;
  if (!XII_TEST_BOOL(directNode.IsValid()))
    return;
  for (xiiUInt32 phase = 0; phase < 6; ++phase)
  {
    xiiColor direct(1, 2, 3, 1), indirect(0.5f, 0.25f, 1, 1), emissive(0.1f, 0.2f, 0.3f, 1), signal(0.4f, 0.8f, 1.2f, 0.5f), fog(0.05f, 0.1f, 0.15f, 0.25f);
    if (phase == 1)
      signal.a = 0;
    if (phase == 2)
    {
      signal.a = 2;
      fog.a    = 2;
    }
    if (phase == 3)
    {
      signal.a = -1;
      fog.a    = -1;
    }
    if (phase == 4)
    {
      direct   = {60000, 60000, 60000, 1};
      indirect = direct;
      emissive = {10000, 10000, 10000, 1};
      signal   = {20000, 20000, 20000, 0.5f};
      fog      = {0, 0, 0, 0.75f};
    }
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
    const auto source = [&](xiiStringView name, xiiColor color, bool mismatch = false) {
      xiiClearTexturePass clear;
      clear.m_uiWidth  = mismatch ? 8 : 9;
      clear.m_uiHeight = 11;
      clear.m_Format   = xiiGALResourceFormat::RGBA32Float;
      clear.m_Color    = color;
      xiiVariantDictionary properties;
      XII_TEST_RESULT(clear.GetProperties(properties));
      return root.AddPass(name, "xiiClearTexturePass", properties);
    };
    const auto a = source("Direct", direct), b = source("Indirect", indirect, phase == 5), e = source("Emission", emissive), g = source("Near GI", signal), v = source("Fog", fog);
    const auto first = root.AddSubgraph("First composition", authored->m_Descriptor.m_Id), second = root.AddSubgraph("Second composition", authored->m_Descriptor.m_Id);
    auto&      weight = root.m_Passes[second].m_Overrides.ExpandAndGetRef();
    weight.m_Path.PushBack(directNode);
    weight.m_Properties.Insert("ScaleA", 1.5f);
    weight.m_Properties.Insert("ScaleB", 0.25f);
    for (const auto target : {first, second})
    {
      XII_TEST_RESULT(root.Connect(a, "Texture", target, "DirectLighting"));
      XII_TEST_RESULT(root.Connect(b, "Texture", target, "IndirectLighting"));
      XII_TEST_RESULT(root.Connect(e, "Texture", target, "GBufferEmissive"));
      XII_TEST_RESULT(root.Connect(g, "Texture", target, "RTFinalGI"));
      XII_TEST_RESULT(root.Connect(v, "Texture", target, "VolumetricScattering"));
    }
    for (const auto node : {first, second})
    {
      const auto out = root.AddPass(node == first ? "First export" : "Second export", "xiiOpaqueTestExport");
      XII_TEST_RESULT(root.Connect(node, "HDRSceneColor", out, "Input"));
      root.m_Interface.PushBack({node == first ? "First" : "Second", false, root.m_Passes[out].m_Id, "Output"});
    }
    xiiStringBuilder resourceName;
    resourceName.SetFormat("Radiance composition phase {}", phase);
    const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(resourceName, std::move(root));
    xiiRenderGraphContext     context;
    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(context.Initialize("Standalone radiance composition")) || !XII_TEST_RESULT(instance.Synchronize(resource)))
      return;
    XII_TEST_BOOL(instance.GetPipeline()->GetRuntimeFeatures().IsNoFlagSet() && instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    if (phase == 5)
    {
      XII_TEST_BOOL(instance.Build(nullptr, graph, context.GetBlackboard(), {}).Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("RadianceAdd requires matching") != nullptr);
      frame.End();
      continue;
    }
    if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard(), {})))
      return;
    const auto firstImage  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("First", false))->m_hTexture;
    const auto secondImage = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Second", false))->m_hTexture;
    instance.Clear(); // The queued graph owns pipelines and setup values, even without live pass objects.
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    frame.End();
    environment.GetDevice()->WaitIdle();
    const auto firstTexture = graph.GetExportedTexture(firstImage), secondTexture = graph.GetExportedTexture(secondImage);
    if (!XII_TEST_BOOL(firstTexture && secondTexture && firstTexture != secondTexture))
      return;
    for (xiiUInt32 other = 0; other < 2; ++other)
    {
      xiiColor    expected;
      const float confidence = xiiMath::Saturate(signal.a), transmission = 1 - xiiMath::Saturate(fog.a);
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        const float radiance        = (direct.GetData()[channel] * (other ? 1.5f : 1.0f) + indirect.GetData()[channel] * (other ? 0.25f : 1.0f) + emissive.GetData()[channel] + signal.GetData()[channel] * confidence) * transmission + fog.GetData()[channel];
        expected.GetData()[channel] = float(xiiFloat16(xiiMath::Clamp(radiance, 0.0f, 65504.0f)));
      }
      expected.a = 1;
      CheckOpaquePixels(environment, other ? secondTexture : firstTexture, expected);
    }
    if (phase == 0)
    {
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      xiiTestFramework::Output(xiiTestOutput::Details, "Radiance composition: %u operations, %u submissions, %u barriers.", graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
    }
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
}
