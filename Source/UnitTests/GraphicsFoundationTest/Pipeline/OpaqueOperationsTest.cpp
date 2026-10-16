/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

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
    const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](xiiRenderGraphTextureHandle& data, xiiRenderGraphBuilder& builder) {
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
        const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiFloat16*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
        {
          XII_TEST_FLOAT(static_cast<float>(row[x * 4]), expected.r, 0.01f);
          XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 1]), expected.g, 0.01f);
          XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 2]), expected.b, 0.01f);
          XII_TEST_FLOAT(static_cast<float>(row[x * 4 + 3]), expected.a, 0.01f);
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
  const auto direct         = clear("Direct", {1, 2, 3, 1});
  const auto indirect       = clear("Indirect", {0.5f, 0.25f, 1, 1});
  const auto emissive       = clear("Emissive", {0.1f, 0.2f, 0.3f, 1});
  const auto gi             = clear("Near GI", {0.4f, 0.8f, 1.2f, 0.5f});
  const auto volume         = clear("Volume", {0.05f, 0.1f, 0.15f, 0.25f});
  const auto environmentMap = clear("Environment", {0.2f, 0.4f, 0.6f, 1});
  const auto traced         = clear("Traced reflections", {0.4f, 0.8f, 1.2f, 0.25f});
  const auto radiance       = clear("Screen reflection signal", {0.8f, 0.4f, 0.2f, 0.5f});
  const auto depth          = clear("Depth", xiiColor::MakeZero(), xiiGALResourceFormat::R32Float);
  const auto attributes     = clear("Material", {1, 0, 0, 0}); // Roughness rejects screen traversal deterministically.
  const auto surface        = clear("Previous surface", xiiColor::MakeZero());
  const auto objects        = clear("Object identity", xiiColor::MakeZero(), xiiGALResourceFormat::R32UInt);
  const auto camera         = leaf.AddPass("Camera", "xiiViewConstantsPass");
  const auto opaque         = leaf.AddPass("Opaque", "xiiOpaqueLightingCompositePass");
  const auto trace          = leaf.AddPass("Trace", "xiiScreenSpaceReflectionsPass");
  const auto temporal       = leaf.AddPass("Temporal", "xiiScreenSpaceReflectionsTemporalPass");
  const auto composite      = leaf.AddPass("Composite", "xiiHybridReflectionCompositePass");
  const auto connect        = [&](xiiUInt32 source, xiiUInt32 target, xiiStringView pin) {
    XII_TEST_RESULT(leaf.Connect(source, "Texture", target, pin));
  };
  connect(direct, opaque, "DirectLighting");
  connect(indirect, opaque, "IndirectLighting");
  connect(emissive, opaque, "GBufferEmissive");
  connect(gi, opaque, "RTFinalGI");
  connect(volume, opaque, "VolumetricScattering");
  connect(environmentMap, composite, "EnvironmentSpecular");
  connect(traced, composite, "RTFinalReflections");
  for (auto operation : {trace, temporal})
  {
    connect(depth, operation, "SceneDepth");
    connect(attributes, operation, "GBufferNormal");
    connect(attributes, operation, "GBufferMaterial");
    XII_TEST_RESULT(leaf.Connect(camera, "PerFrameCamera", operation, "PerFrameCamera"));
  }
  connect(attributes, trace, "GBufferAlbedo");
  connect(attributes, trace, "BRDFLut");
  connect(depth, trace, "HiZClosestPyramid");
  connect(depth, trace, "HiZPyramid");
  XII_TEST_RESULT(leaf.Connect(camera, "PerFrameGlobal", trace, "PerFrameGlobal"));
  XII_TEST_RESULT(leaf.Connect(opaque, "HDRSceneColor", trace, "HDRSceneColor"));
  XII_TEST_RESULT(leaf.Connect(opaque, "HDRSceneColor", composite, "HDRSceneColor"));
  connect(radiance, temporal, "SSRRawTerm");
  connect(attributes, temporal, "DilatedVelocityBuffer");
  connect(objects, temporal, "GBufferObjectId");
  connect(surface, temporal, "ReSTIRDISurfacePrevious");
  XII_TEST_RESULT(leaf.Connect(temporal, "SSRTerm", composite, "SSRTerm"));
  const auto exportPin = [&](xiiStringView name, xiiUInt32 source, xiiStringView pin) {
    const auto output = leaf.AddPass(name, "xiiOpaqueTestExport");
    XII_TEST_RESULT(leaf.Connect(source, pin, output, "Input"));
    return leaf.m_Passes[output].m_Id;
  };
  const auto         outputId   = exportPin("Composite readback", composite, "HDRSceneColor");
  const auto         traceId    = exportPin("Trace readback", trace, "SSRRawTerm");
  const auto         historyId  = exportPin("Current reflection readback", temporal, "SSRTerm");
  const auto         previousId = exportPin("Previous reflection readback", temporal, "PreviousReflection");
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First reflection instance", definition.m_Id);
  root.AddSubgraph("Second reflection instance", definition.m_Id);
  const auto secondId = root.m_Passes[1].m_Id;
  auto&      override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
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
        CheckOpaquePixels(environment, texture, other ? xiiColor(1.525f, 2.1875f, 3.65f, 1) : xiiColor(1.725f, 2.2875f, 3.7f, 1));
      else if (provenance.m_Element == traceId)
        CheckOpaquePixels(environment, texture, xiiColor::MakeZero());
      else
        CheckOpaquePixels(environment, texture, other ? xiiColor(0.4f, 0.2f, 0.1f, 0.5f) : xiiColor(0.8f, 0.4f, 0.2f, 0.5f));
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
