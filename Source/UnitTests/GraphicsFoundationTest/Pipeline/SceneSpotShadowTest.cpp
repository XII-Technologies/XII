/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowAtlasPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Shaders/Pipeline/Passes/Shadows/SpotShadowConstants.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

class xiiSpotSamplingTestExport final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotSamplingTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_Data{xiiGetStaticRTTI<xiiSpotShadowSnapshot>(), true};
  xiiRenderPipelineBufferOutputPin m_Constants{true, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_Flags{true, xiiGALBindFlags::ShaderResource};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto data = context.GetInputData(m_Data).Downcast<const xiiSpotShadowSnapshot>();
    if (!data)
      return XII_SUCCESS;
    context.m_Graph.AddPass<xiiUInt32>("Inspect spotlight sampling", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
      builder.ExportBuffer(builder.ReadBuffer(data->m_hConstants, xiiGALResourceStateFlags::ConstantBuffer), xiiGALResourceStateFlags::ConstantBuffer);
      builder.ExportBuffer(builder.ReadBuffer(data->m_hReceivers, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {});
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Constants, data->m_hConstants));
    return context.SetOutput(m_Flags, data->m_hReceivers);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotSamplingTestExport, 1, xiiRTTIDefaultAllocator<xiiSpotSamplingTestExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Data", m_Data)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Constants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Flags", m_Flags)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(SceneSpotShadows);
XII_CREATE_SIMPLE_TEST(SceneSpotShadows, PerspectiveCasterAtlasAffectsIndividualLights)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 canonical = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> loaded(canonical, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  xiiRenderPipelineResourceHandle pipelines[2], roots[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    auto definition = loaded->GetAuthoring()->m_Descriptor;
    for (auto& pass : definition.m_Passes)
    {
      const auto type = pass.m_pType ? pass.m_pType->GetTypeName() : xiiStringView();
      if (type == "xiiAmbientOcclusionPass" || type == "xiiTemporalCameraPass" || type == "xiiTemporalAntiAliasingPass")
        pass.m_Properties.Insert("Enabled", false);
      if (pass.m_Id == xiiUuid::MakeStableUuidFromString("XII.Scene/SpotShadows"))
      {
        auto& setting = pass.m_Overrides.ExpandAndGetRef();
        setting.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneSpotShadows/World Spotlights"));
        setting.m_Properties.Insert("Enabled", variant != 0);
      }
    }
    definition.m_Interface.PushBack({"LocalLighting", false, xiiFindPipelineTestPass(definition, "xiiClusteredLocalSurfaceLightingPass"), "Lighting"});
    definition.m_Interface.PushBack({"SpotReactive", false, xiiFindPipelineTestPass(definition, "xiiClusteredLocalSurfaceLightingPass"), "ReactiveRadiance"});
    xiiStringBuilder name;
    name.SetFormat("Spot.Scene.{}", variant);
    const auto                          source = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, source.GetResourceID()});
    const auto node = root.AddSubgraph("Canonical Scene", loaded->GetAuthoring()->m_Descriptor.m_Id);
    for (const xiiStringView pin : {"HDRSceneColor", "AmbientLighting", "LocalLighting", "SpotReactive", "SceneDepth", "ObjectId", "SpotShadowAtlas", "TransparentAccumulation", "TransparentRevealage"})
    {
      const auto output = root.AddPass(pin, xiiGetStaticRTTI<xiiOpaqueTestExport>());
      XII_TEST_RESULT(root.Connect(node, pin, output, "Input"));
      root.m_Interface.PushBack({pin, false, root.m_Passes[output].m_Id, "Output"});
    }
    root.m_Interface.PushBack({"SpotShadows", false, root.m_Passes[node].m_Id, "SpotShadows"});
    const auto debug = root.AddPass("Sampling readbacks", xiiGetStaticRTTI<xiiSpotSamplingTestExport>());
    XII_TEST_RESULT(root.Connect(node, "SpotShadows", debug, "Data"));
    root.m_Interface.PushBack({"SpotConstants", false, root.m_Passes[debug].m_Id, "Constants"});
    root.m_Interface.PushBack({"SpotFlags", false, root.m_Passes[debug].m_Id, "Flags"});
    name.Append(".Root");
    roots[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(root));
    name.Append(".Reloadable");
    pipelines[variant] = xiiMakeReloadablePipelineTestResource(roots[variant], name);
  }
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  geometry.ComputeTangents();
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Spot.Box", std::move(meshDescription));
  xiiMaterialResourceDescriptor material;
  material.m_BaseColor  = {0.8f, 0.8f, 0.8f};
  material.m_fRoughness = 1;
  const auto gray       = xiiResourceManager::CreateResource<xiiMaterialResource>("Spot.Gray", xiiMaterialResourceDescriptor(material));
  material.m_FeatureFlags.Remove(xiiMaterialFeatureFlags::CastsShadows);
  const auto                noCast = xiiResourceManager::CreateResource<xiiMaterialResource>("Spot.NoCast", std::move(material));
  xiiMaterialResourceHandle masks[2];
  for (xiiUInt32 opaque = 0; opaque < 2; ++opaque)
  {
    const xiiColorGammaUB          texel(255, 255, 255, opaque ? 255 : 0);
    xiiGALTextureSubResourceData   bytes(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&texel), sizeof(texel)), sizeof(texel), sizeof(texel));
    xiiTexture2DResourceDescriptor texture;
    texture.m_TextureDescription.m_Type   = xiiGALResourceDimension::Texture2D;
    texture.m_TextureDescription.m_Size   = {1, 1};
    texture.m_TextureDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    texture.m_InitialContent              = xiiMakeArrayPtr(&bytes, 1);
    xiiStringBuilder name;
    name.SetFormat("Spot.Mask.{}", opaque);
    xiiMaterialResourceDescriptor masked;
    masked.m_BaseColor         = {0.8f, 0.8f, 0.8f};
    masked.m_fRoughness        = 1;
    masked.m_AlphaMode         = xiiMaterialAlphaMode::Mask;
    masked.m_hBaseColorTexture = xiiResourceManager::CreateResource<xiiTexture2DResource>(name, std::move(texture));
    name.Append(".Material");
    masks[opaque] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(masked));
  }
  xiiWorldDescription         worldDescription("Spotlight transport world");
  xiiUniquePtr<xiiWorld>      world  = XII_DEFAULT_NEW(xiiWorld, worldDescription);
  xiiMeshComponent*           floor  = nullptr;
  xiiMeshComponent*           caster = nullptr;
  xiiSpotLightComponent*      lights[2];
  xiiView*                    views[2];
  xiiSharedPtr<xiiGALTexture> targets[2];
  xiiCamera                   camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-4, -5, 3}, {0, 0, 0}, {0, 0, 1});
  camera.SetExposure(8);
  {
    XII_LOCK(world->GetWriteMarker());
    world->SetWorldSimulationEnabled(false);
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_bDynamic      = true;
      objectDescription.m_LocalPosition = i ? xiiVec3(0, 0, 0.5f) : xiiVec3(0, 0, -0.1f);
      objectDescription.m_LocalScaling  = i ? xiiVec3(1) : xiiVec3(8, 8, 0.2f);
      xiiGameObject* object             = nullptr;
      world->CreateObject(objectDescription, object);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, gray);
      (i ? caster : floor) = component;
      xiiGameObjectDescription lightDescription;
      lightDescription.m_bDynamic      = true;
      lightDescription.m_LocalPosition = i ? xiiVec3(-2, 1, 4) : xiiVec3(2, 1, 4);
      lightDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -lightDescription.m_LocalPosition.GetNormalized());
      world->CreateObject(lightDescription, object);
      xiiSpotLightComponent::CreateComponent(object, lights[i]);
      lights[i]->SetIntensityUnit(xiiPhotometricUnit::Candela);
      lights[i]->SetIntensity(10);
      lights[i]->SetRange(15);
      lights[i]->SetInnerSpotAngle(xiiAngle::MakeFromDegree(65));
      lights[i]->SetOuterSpotAngle(xiiAngle::MakeFromDegree(80));
      lights[i]->SetLightColor(i ? xiiColorGammaUB(0, 0, 255) : xiiColorGammaUB(255, 0, 0));
      lights[i]->SetCastShadows(true);
      lights[i]->SetActiveFlag(i == 0);
    }
    auto* module = world->GetOrCreateModule<xiiRenderWorldModule>();
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      module->CreateView("Spotlight comparison", views[variant]);
      views[variant]->SetCamera(&camera);
      views[variant]->SetRenderPipelineResource(pipelines[variant]);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(views[variant]->SetDisplayOutputSettings(display));
    }
  }
  const auto resize = [&](xiiUInt32 width, xiiUInt32 height) {
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {width, height};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[variant]   = environment.GetDevice()->CreateTexture(target);
      views[variant]->SetViewport({0, 0, float(width), float(height)});
      views[variant]->SetRenderTargetView(targets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  };
  const auto image = [&](xiiUInt32 variant, xiiStringView name) {
    const auto& instance = views[variant]->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
    return binding && binding->m_hTexture.IsValid() ? views[variant]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
  };
  {
    XII_LOCK(world->GetWriteMarker());
    resize(129, 97);
  }
  xiiDynamicArray<xiiVec4>           initial;
  xiiDynamicArray<xiiColorLinear16f> red[2], combined[2];
  double                             initialShadowEnergy = 0;
  xiiSpotLightComponent*             extraLights[2]      = {};
  for (xiiUInt32 phase = 0; phase < 26; ++phase)
  {
    XII_LOCK(world->GetWriteMarker());
    caster->SetMaterialOverride(0, phase == 1 ? noCast : gray);
    if (phase == 16 || phase == 17)
      caster->SetMaterialOverride(0, masks[phase - 16]);
    caster->SetActiveFlag(phase != 5 && phase != 7);
    lights[0]->SetCastShadows(phase != 3);
    lights[0]->SetActiveFlag(phase != 14);
    lights[1]->SetActiveFlag(phase == 13 || phase == 14 || phase == 24);
    lights[0]->SetShadowFadeOutRange(phase == 18 ? 1.0f : phase == 19 ? (camera.GetPosition() - lights[0]->GetOwner()->GetLocalPosition()).GetLength() / 0.9f :
                                       phase == 22                    ? 0 :
                                                                        20);
    if (phase == 24)
      for (xiiUInt32 light = 0; light < 2; ++light)
      {
        xiiGameObjectDescription description;
        description.m_bDynamic      = true;
        description.m_LocalPosition = light ? xiiVec3(-2, 1, 4) : xiiVec3(2, 1, 4);
        description.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -description.m_LocalPosition.GetNormalized());
        xiiGameObject* owner        = nullptr;
        world->CreateObject(description, owner);
        xiiSpotLightComponent::CreateComponent(owner, extraLights[light]);
        extraLights[light]->SetIntensityUnit(xiiPhotometricUnit::Candela);
        extraLights[light]->SetIntensity(10);
        extraLights[light]->SetRange(15);
        extraLights[light]->SetInnerSpotAngle(xiiAngle::MakeFromDegree(65));
        extraLights[light]->SetOuterSpotAngle(xiiAngle::MakeFromDegree(80));
        extraLights[light]->SetLightColor(light ? xiiColorGammaUB(0, 0, 255) : xiiColorGammaUB(255, 0, 0));
        extraLights[light]->SetShadowFadeOutRange(light ? 0.0f : 20.0f);
        extraLights[light]->SetCastShadows(true);
      }
    if (phase == 25)
      for (auto* light : extraLights)
        light->SetActiveFlag(false);
    if (phase == 6)
    {
      camera.SetCameraMode(xiiCameraMode::OrthoFixedWidth, 2, 0.1f, 100);
      camera.LookAt({0, -5, 3}, {0, 0, 0}, {0, 0, 1});
      caster->GetOwner()->SetLocalPosition(xiiVec3(1.4f, 0, 1.5f));
      caster->GetOwner()->SetLocalScaling(xiiVec3(0.4f));
      lights[0]->GetOwner()->SetLocalPosition(xiiVec3(4, 0, 4));
      lights[0]->GetOwner()->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-1, 0, -1).GetNormalized()));
    }
    if (phase == 8)
    {
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      camera.LookAt({-4, -5, 3}, {0, 0, 0}, {0, 0, 1});
      caster->GetOwner()->SetLocalPosition(xiiVec3(0, 0, 0.5f));
      caster->GetOwner()->SetLocalScaling(xiiVec3(1));
      lights[0]->GetOwner()->SetLocalPosition(xiiVec3(2, 1, 4));
      lights[0]->GetOwner()->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-2, -1, -4).GetNormalized()));
    }
    if (phase == 9 || phase == 10)
      resize(phase == 9 ? 193 : 129, phase == 9 ? 129 : 97);
    if (phase == 20 || phase == 21)
      resize(phase == 20 ? 1 : 129, phase == 20 ? 1 : 97);
    if (phase == 11)
      xiiMakeReloadablePipelineTestResource(roots[1], pipelines[1].GetResourceID());
    if (phase == 12)
      for (auto* view : views)
        view->InvalidateTemporalHistory();
    world->Update();
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>           hdr[2], ambient[2];
    xiiDynamicArray<float>             depth[2];
    xiiDynamicArray<xiiUInt32>         ids[2], final[2];
    xiiDynamicArray<xiiColorLinear16f> local[2];
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
      if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "HDRSceneColor"), hdr[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "AmbientLighting"), ambient[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "LocalLighting"), local[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "SceneDepth"), depth[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "ObjectId"), ids[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]))
        return;
    XII_TEST_BOOL(depth[0] == depth[1] && ids[0] == ids[1] && ambient[0] == ambient[1] && !image(0, "SpotShadowAtlas"));
    xiiUInt32                          changed = 0, shadowed = 0, potential = 0;
    double                             shadowEnergy = 0;
    xiiDynamicArray<xiiColorLinear16f> reactive;
    const auto                         signal = image(1, "SpotReactive");
    if (signal && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), signal, reactive))
      return;
    XII_TEST_BOOL(!image(0, "SpotReactive"));
    for (xiiUInt32 pixel = 0; pixel < hdr[0].GetCount(); ++pixel)
    {
      XII_TEST_BOOL(hdr[0][pixel].IsValid() && hdr[1][pixel].IsValid());
      changed += final[0][pixel] != final[1][pixel];
      const auto a = local[0][pixel].ToLinearFloat(), b = local[1][pixel].ToLinearFloat();
      shadowEnergy += a.r - b.r + a.b - b.b;
      shadowed += a.r > b.r + 0.001f || a.b > b.b + 0.001f;
      if (signal)
      {
        const auto value = reactive[pixel].ToLinearFloat();
        XII_TEST_BOOL(value.r == 0 || value.r == 1);
        potential += value.r > 0;
        if (a.r > b.r + 0.001f || a.b > b.b + 0.001f)
          XII_TEST_FLOAT(value.r, 1, 0);
      }
      if (phase == 6 && ids[0][pixel])
      {
        xiiGpuInstanceSource source;
        XII_TEST_RESULT(views[0]->ResolveRenderedMesh(ids[0][pixel], source));
        XII_TEST_BOOL(source.m_hOwnerObject != caster->GetOwner()->GetHandle());
      }
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        XII_TEST_BOOL(b.GetData()[channel] <= a.GetData()[channel] + 0.0001f);
        XII_TEST_FLOAT(hdr[1][pixel].GetData()[channel] - hdr[0][pixel].GetData()[channel], b.GetData()[channel] - a.GetData()[channel], 0.0001f);
      }
    }
    const auto atlas = image(1, "SpotShadowAtlas");
    if (phase == 0)
    {
      xiiDynamicArray<float> values;
      if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), atlas, values))
        return;
      xiiUInt32 nonzero = 0;
      for (const auto value : values)
      {
        XII_TEST_BOOL(xiiMath::IsFinite(value) && value >= 0 && value <= 1);
        nonzero += value > 0;
      }
      double energy = 0;
      for (const auto& value : local[0])
        energy += float(value.r);
      const auto& instance = views[1]->GetRenderPipelineInstance();
      const auto  buffer   = [&](xiiStringView name) {
        return views[1]->GetRenderGraph()->GetExportedBuffer(instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false))->m_hBuffer);
      };
      xiiSpotShadowConstants constants;
      xiiVec2U32             flags;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("SpotConstants").Borrow(), 0, constants) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer("SpotFlags").Borrow(), 0, flags))
        return;
      xiiMat4 matrix;
      static_assert(sizeof(matrix) == sizeof(constants.SpotViewProjection[0]));
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&matrix), reinterpret_cast<const xiiUInt8*>(&constants.SpotViewProjection[0]), sizeof(matrix));
      const auto origin = matrix.GetColumn(3);
      XII_TEST_BOOL(nonzero > 8 && energy > 0 && flags.x != 0 && origin.w > 0 && origin.z / origin.w > 0 && origin.z / origin.w < 1);
      XII_TEST_INT(constants.SpotShadowLayout.x, 1);
      const auto snapshot = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("SpotShadows", false))->m_pFrameData.Downcast<const xiiSpotShadowSnapshot>();
      XII_TEST_BOOL(matrix == snapshot->m_Views[0].m_ViewProjection);
      XII_TEST_INT(constants.SpotLightIndices.x, snapshot->m_Views[0].m_uiLightIndex);
    }
    XII_TEST_BOOL((atlas != nullptr) == (phase != 3 && phase != 18));
    XII_TEST_BOOL((signal != nullptr) == (atlas != nullptr));
    if (phase == 1 || phase == 3 || phase == 5 || phase == 7 || phase == 16 || phase == 18)
      XII_TEST_INT(shadowed, 0);
    else if (phase != 20)
      XII_TEST_BOOL(shadowed > 8 && changed > 8);
    if (phase == 0)
      initialShadowEnergy = shadowEnergy;
    if (phase == 19)
      XII_TEST_BOOL(shadowEnergy > initialShadowEnergy * 0.4 && shadowEnergy < initialShadowEnergy * 0.6);
    if (phase == 22)
      XII_TEST_BOOL(shadowEnergy > 0 && shadowEnergy < initialShadowEnergy);
    if (shadowed > 8)
      XII_TEST_BOOL(potential > 8);
    if (phase == 0)
    {
      initial = hdr[1];
      red[0]  = local[0];
      red[1]  = local[1];
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
      {
        xiiImage capture;
        capture.ResetAndAlloc(targets[variant]->GetDescription());
        xiiMemoryUtils::Copy(capture.GetPixelPointer<xiiUInt32>(), final[variant].GetData(), final[variant].GetCount());
        XII_TEST_RESULT(capture.SaveTo(xiiStringBuilder(":imgout/SceneSpot-", variant ? "Enabled" : "Disabled", ".png")));
      }
    }
    if (phase == 2 || phase == 4 || phase == 8 || phase == 10 || phase == 11 || phase == 12 || phase == 15 || phase == 21 || phase == 23 || phase == 25)
      XII_TEST_BOOL(initial == hdr[1]);
    if (phase == 24)
    {
      XII_TEST_BOOL(atlas->GetDescription().m_Size == xiiSizeU32(2048, 2048));
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
        for (xiiUInt32 pixel = 0; pixel < local[variant].GetCount(); ++pixel)
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
            XII_TEST_FLOAT(local[variant][pixel].ToLinearFloat().GetData()[channel], combined[variant][pixel].ToLinearFloat().GetData()[channel] * 2, 0.0003f);
    }
    if (phase == 13)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
        combined[variant] = local[variant];
    if (phase == 14)
      for (xiiUInt32 variant = 0; variant < 2; ++variant)
        for (xiiUInt32 pixel = 0; pixel < local[variant].GetCount(); ++pixel)
        {
          const auto sum    = red[variant][pixel].ToLinearFloat() + local[variant][pixel].ToLinearFloat();
          const auto actual = combined[variant][pixel].ToLinearFloat();
          for (xiiUInt32 channel = 0; channel < 3; ++channel)
            XII_TEST_FLOAT(actual.GetData()[channel], sum.GetData()[channel], 0.0002f);
        }
    xiiTestFramework::Output(xiiTestOutput::Details, "Spotlight phase %u: %u shadowed pixels, %u final changes", phase, shadowed, changed);
  }

  xiiMeshComponent*         pane = nullptr;
  xiiMaterialResourceHandle panes[2];
  for (xiiUInt32 unlit = 0; unlit < 2; ++unlit)
  {
    xiiMaterialResourceDescriptor description;
    description.m_BaseColor    = {0.8f, 0.8f, 0.8f, 0.75f};
    description.m_fRoughness   = 1;
    description.m_AlphaMode    = xiiMaterialAlphaMode::Blend;
    description.m_ShadingModel = unlit ? xiiMaterialShadingModel::Unlit : xiiMaterialShadingModel::Lit;
    xiiStringBuilder name;
    name.SetFormat("Spot.Transparent.{}", unlit);
    panes[unlit] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(description));
  }
  {
    XII_LOCK(world->GetWriteMarker());
    xiiGameObjectDescription description;
    description.m_bDynamic      = true;
    description.m_LocalPosition = {0, 0, 0.25f};
    description.m_LocalScaling  = {4, 4, 0.02f};
    xiiGameObject* object       = nullptr;
    world->CreateObject(description, object);
    xiiMeshComponent::CreateComponent(object, pane);
    pane->SetMesh(mesh);
  }
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    {
      XII_LOCK(world->GetWriteMarker());
      pane->SetMaterialOverride(0, panes[phase == 3 ? 1 : 0]);
      caster->SetMaterialOverride(0, phase == 1 ? noCast : gray);
      world->Update();
    }
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>   layers[2];
    xiiDynamicArray<float>     reveal[2], depths[2];
    xiiDynamicArray<xiiUInt32> ids[2], final[2];
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      if (!XII_TEST_RESULT_MSG(views[variant]->GetLastRenderResult(), "%s", views[variant]->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "TransparentAccumulation"), layers[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "TransparentRevealage"), reveal[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "SceneDepth"), depths[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(variant, "ObjectId"), ids[variant]) ||
          !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), targets[variant], final[variant]))
        return;
    }
    XII_TEST_BOOL(reveal[0] == reveal[1] && depths[0] == depths[1] && ids[0] == ids[1]);
    xiiUInt32 affected = 0, finalChanges = 0;
    for (xiiUInt32 pixel = 0; pixel < layers[0].GetCount(); ++pixel)
    {
      XII_TEST_BOOL(layers[0][pixel].IsValid() && layers[1][pixel].IsValid());
      affected += !layers[0][pixel].IsEqual(layers[1][pixel], 1e-5f);
      finalChanges += final[0][pixel] != final[1][pixel];
    }
    if (phase == 1 || phase == 3)
      XII_TEST_INT(affected, 0);
    else
      XII_TEST_BOOL(affected > 8 && finalChanges > 8);
    xiiTestFramework::Output(xiiTestOutput::Details, "Transparent spotlight phase %u: %u shaded layer pixels; %u final changes", phase, affected, finalChanges);
  }
  {
    XII_LOCK(world->GetWriteMarker());
    pane->SetActiveFlag(false);
  }

  xiiRenderPipelineResourceHandle temporal[2], temporalSources[2];
  const auto                      cameraPass = xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiTemporalCameraPass");
  const auto                      taaPass    = xiiFindPipelineTestPass(loaded->GetAuthoring()->m_Descriptor, "xiiTemporalAntiAliasingPass");
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiResourceLock<xiiRenderPipelineResource> root(roots[1], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    auto                                       definition    = root->GetAuthoring()->m_Descriptor;
    auto&                                      cameraSetting = definition.m_Passes[0].m_Overrides.ExpandAndGetRef();
    cameraSetting.m_Path.PushBack(cameraPass);
    cameraSetting.m_Properties.Insert("Enabled", true);
    auto& taa = definition.m_Passes[0].m_Overrides.ExpandAndGetRef();
    taa.m_Path.PushBack(taaPass);
    taa.m_Properties.Insert("Enabled", true);
    taa.m_Properties.Insert("BaseBlendAlpha", variant ? 0.08f : 1.0f);
    xiiStringBuilder name;
    name.SetFormat("Spot.Temporal.{}", variant);
    temporalSources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition));
    name.Append(".Reloadable");
    temporal[variant] = xiiMakeReloadablePipelineTestResource(temporalSources[variant], name);
  }
  xiiWorldDescription         foreignDescription("Independent blue spotlight world");
  xiiUniquePtr<xiiWorld>      foreign = XII_DEFAULT_NEW(xiiWorld, foreignDescription);
  xiiView*                    foreignViews[2];
  xiiSharedPtr<xiiGALTexture> foreignTargets[2];
  xiiSpotLightComponent*      foreignLight = nullptr;
  xiiCamera                   foreignCamera;
  foreignCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  foreignCamera.LookAt({-4, -5, 3}, {0, 0, 0}, {0, 0, 1});
  foreignCamera.SetExposure(8);
  {
    XII_LOCK(foreign->GetWriteMarker());
    foreign->SetWorldSimulationEnabled(false);
    for (xiiUInt32 object = 0; object < 2; ++object)
    {
      xiiGameObjectDescription description;
      description.m_bDynamic      = true;
      description.m_LocalPosition = object ? xiiVec3(0, 0, 0.5f) : xiiVec3(0, 0, -0.1f);
      description.m_LocalScaling  = object ? xiiVec3(1) : xiiVec3(8, 8, 0.2f);
      xiiGameObject* owner        = nullptr;
      foreign->CreateObject(description, owner);
      xiiMeshComponent* component = nullptr;
      xiiMeshComponent::CreateComponent(owner, component);
      component->SetMesh(mesh);
      component->SetMaterialOverride(0, gray);
    }
    xiiGameObjectDescription description;
    description.m_bDynamic      = true;
    description.m_LocalPosition = {2, 1, 4};
    description.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -description.m_LocalPosition.GetNormalized());
    xiiGameObject* owner        = nullptr;
    foreign->CreateObject(description, owner);
    xiiSpotLightComponent::CreateComponent(owner, foreignLight);
    foreignLight->SetIntensityUnit(xiiPhotometricUnit::Candela);
    foreignLight->SetIntensity(10);
    foreignLight->SetRange(15);
    foreignLight->SetInnerSpotAngle(xiiAngle::MakeFromDegree(65));
    foreignLight->SetOuterSpotAngle(xiiAngle::MakeFromDegree(80));
    foreignLight->SetLightColor(xiiColorGammaUB(0, 0, 255));
    foreignLight->SetCastShadows(true);
    for (xiiUInt32 variant = 0; variant < 2; ++variant)
    {
      foreign->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent spotlight temporal", foreignViews[variant]);
      foreignViews[variant]->SetCamera(&foreignCamera);
      foreignViews[variant]->SetRenderPipelineResource(temporal[variant]);
      foreignViews[variant]->SetViewport({0, 0, 129, 97});
      foreignTargets[variant] = environment.GetDevice()->CreateTexture(targets[variant]->GetDescription());
      foreignViews[variant]->SetRenderTargetView(foreignTargets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      display.m_fBloomStrength  = 0;
      XII_TEST_RESULT(foreignViews[variant]->SetDisplayOutputSettings(display));
    }
  }
  const auto foreignImage = [&](xiiUInt32 variant, xiiStringView name) {
    const auto& instance = foreignViews[variant]->GetRenderPipelineInstance();
    const auto* binding  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false));
    return binding && binding->m_hTexture.IsValid() ? foreignViews[variant]->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : xiiSharedPtr<xiiGALTexture>();
  };
  xiiDynamicArray<xiiColorLinear16f> previousSignal;
  xiiUInt32                          disappeared = 0;
  for (xiiUInt32 phase = 0; phase < 11; ++phase)
  {
    {
      XII_LOCK(foreign->GetWriteMarker());
      foreignLight->SetCastShadows(phase != 8);
      foreignLight->SetActiveFlag(phase != 10);
      if (phase == 2 || phase == 7)
        for (auto* view : foreignViews)
          view->InvalidateTemporalHistory();
      if (phase == 4)
        for (xiiUInt32 variant = 0; variant < 2; ++variant)
        {
          auto description        = foreignTargets[variant]->GetDescription();
          description.m_Size      = {97, 73};
          foreignTargets[variant] = environment.GetDevice()->CreateTexture(description);
          foreignViews[variant]->SetViewport({0, 0, 97, 73});
          foreignViews[variant]->SetRenderTargetView(foreignTargets[variant]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
        }
      if (phase == 5)
        for (xiiUInt32 variant = 0; variant < 2; ++variant)
          xiiMakeReloadablePipelineTestResource(temporalSources[variant], temporal[variant].GetResourceID());
      if (phase == 6)
        foreignLight->GetOwner()->SetLocalPosition(xiiVec3(2.2f, 1, 4));
      foreign->Update();
    }
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4>           current, resolved;
    xiiDynamicArray<xiiColorLinear16f> signal;
    for (auto* view : foreignViews)
      if (!XII_TEST_RESULT_MSG(view->GetLastRenderResult(), "%s", view->GetRenderPipelineInstance().GetLastBuildError().GetStartPointer()))
        return;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), foreignImage(0, "HDRSceneColor"), current) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), foreignImage(1, "HDRSceneColor"), resolved))
      return;
    const auto reactive = foreignImage(1, "SpotReactive");
    if (reactive && !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), reactive, signal))
      return;
    XII_TEST_BOOL((reactive != nullptr) == (phase != 8 && phase != 10));
    XII_TEST_BOOL(foreignImage(0, "SpotShadowAtlas") != foreignImage(1, "SpotShadowAtlas") || !reactive);
    xiiUInt32 marked = 0;
    for (xiiUInt32 pixel = 0; pixel < resolved.GetCount(); ++pixel)
    {
      XII_TEST_BOOL(resolved[pixel].IsValid());
      if (reactive && float(signal[pixel].r) > 0)
      {
        ++marked;
        XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
      }
      if (phase == 0 || phase == 2 || phase == 4 || phase == 5 || phase == 7)
        XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
      if (phase == 8 || phase == 10)
      {
        const auto width = foreignTargets[1]->GetDescription().m_Size.width;
        if (pixel >= width + 1 && pixel + width + 1 < resolved.GetCount() && pixel % width > 0 && pixel % width + 1 < width)
        {
          bool interior = true;
          for (xiiInt32 y = -1; y <= 1; ++y)
            for (xiiInt32 x = -1; x <= 1; ++x)
              interior &= float(previousSignal[pixel + y * width + x].r) > 0;
          if (interior)
          {
            ++disappeared;
            XII_TEST_BOOL(resolved[pixel].IsEqual(current[pixel], 1e-5f));
          }
        }
      }
    }
    XII_TEST_BOOL(reactive ? marked > 8 : marked == 0);
    previousSignal = signal;
    {
      XII_LOCK(world->GetWriteMarker());
      world->Update();
    }
    environment.GetDevice()->WaitIdle();
    xiiDynamicArray<xiiVec4> primary;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), image(1, "HDRSceneColor"), primary))
      return;
    XII_TEST_BOOL(primary == initial);
    xiiTestFramework::Output(xiiTestOutput::Details, "Independent spotlight phase %u: %u reactive pixels; %u disappeared verified", phase, marked, disappeared);
  }
  XII_TEST_BOOL(disappeared > 8);
  {
    XII_LOCK(foreign->GetWriteMarker());
    foreignLight->SetActiveFlag(true);
    foreign->Update();
  }
  {
    xiiRenderGraphTestContext retained;
    if (!XII_TEST_RESULT(retained.Initialize("Retained spotlight shadows")))
      return;
    xiiRenderPipelineInstance recording;
    if (!XII_TEST_RESULT(recording.Synchronize(temporal[1])))
      return;
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = retained.GetGraph();
    auto& cache = retained.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    {
      XII_LOCK(foreign->GetWriteMarker());
      if (!XII_TEST_RESULT(recording.Build(foreignViews[1], graph, retained.GetBlackboard())))
        return;
    }
    const auto output = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("LocalLighting", false))->m_hTexture;
    const auto atlas  = recording.GetInterfaceBinding(recording.GetPipeline()->FindInterface("SpotShadowAtlas", false))->m_hTexture;
    graph.EndSetup();
    recording.Clear();
    foreign.Clear();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &retained.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    xiiDynamicArray<xiiColorLinear16f> retired;
    xiiDynamicArray<float>             depth;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(output), retired) ||
        !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(atlas), depth))
      return;
    xiiUInt32 bluePixels = 0, writtenDepth = 0;
    for (const auto& pixel : retired)
      bluePixels += float(pixel.b) > float(pixel.r) * 10 && float(pixel.b) > 0.001f;
    for (const auto value : depth)
      writtenDepth += value > 0;
    XII_TEST_BOOL(bluePixels > 8 && writtenDepth > 8);
  }
  xiiRenderPipelineResourceDescriptor flat;
  if (!XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(flat, pipelines[1])))
    return;
  {
    XII_LOCK(world->GetWriteMarker());
    lights[1]->SetActiveFlag(true);
  }
  for (xiiUInt32 failure = 0; failure < 2; ++failure)
  {
    auto definition = flat;
    if (failure == 0)
      for (xiiUInt32 connection = definition.m_Connections.GetCount(); connection-- > 0;)
      {
        const auto& edge = definition.m_Connections[connection];
        if (edge.m_sInput == "SpotShadowAtlas" && definition.m_Passes[edge.m_uiTarget].m_pType->GetTypeName() == "xiiClusteredLocalSurfaceLightingPass")
        {
          XII_TEST_RESULT(definition.Disconnect(connection));
        }
      }
    for (auto& pass : definition.m_Passes)
      if (pass.m_pType && pass.m_pType->GetTypeName() == "xiiSpotShadowPass")
      {
        if (failure == 1)
          pass.m_Properties.Insert("MaximumLights", 1U);
      }
    xiiStringBuilder name;
    name.SetFormat("Spot.Invalid.{}", failure);
    views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, xiiRenderPipelineResourceDescriptor(definition)));
    {
      xiiTestLogInterface   expected;
      xiiTestLogSystemScope expectedScope(&expected);
      expected.ExpectMessage(failure == 0 ? "Spotlight sampling requires a matching completed D32 atlas" : "Spotlight shadow capacity exceeded", xiiLogMsgType::ErrorMsg, 1);
      expected.ExpectMessage("Failed to execute render graph for view", xiiLogMsgType::ErrorMsg, 1);
      XII_LOCK(world->GetWriteMarker());
      world->Update();
      XII_TEST_BOOL(views[0]->GetLastRenderResult().Failed());
      XII_TEST_RESULT(views[1]->GetLastRenderResult());
    }
    // The disabled workload must bypass even deliberately incomplete inputs.
    for (auto& pass : definition.m_Passes)
      if (pass.m_pType && pass.m_pType->GetTypeName() == "xiiSpotShadowPass")
        pass.m_Properties.Insert("Enabled", false);
    name.Append(".Disabled");
    views[0]->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(definition)));
    {
      XII_LOCK(world->GetWriteMarker());
      world->Update();
    }
    XII_TEST_RESULT(views[0]->GetLastRenderResult());
  }
  xiiRenderPipelineResourceDescriptor atlasDefinition;
  const auto                          atlasPass = atlasDefinition.AddPass("Spotlight atlas capability", xiiGetStaticRTTI<xiiShadowAtlasPass>());
  atlasDefinition.m_Interface.PushBack({"Spots", true, atlasDefinition.m_Passes[atlasPass].m_Id, "SpotShadows"});
  atlasDefinition.m_Interface.PushBack({"Atlas", false, atlasDefinition.m_Passes[atlasPass].m_Id, "ShadowAtlas"});
  const auto                atlasResource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Spot.DeviceLimit", std::move(atlasDefinition));
  xiiRenderPipelineInstance atlasInstance;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(atlasInstance.Synchronize(atlasResource)) || !XII_TEST_RESULT(context.Initialize("Spotlight image limits")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  graph.BeginSetup(frame.GetFrameIndex());
  xiiStaticArray<xiiSpotShadowDescription, 4> descriptions;
  descriptions.SetCount(2);
  const auto               limit = environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension;
  xiiRenderPipelineBinding binding;
  binding.m_pFrameData = XII_DEFAULT_NEW(xiiSpotShadowSnapshot, graph.GetSetupSerial(), descriptions, limit, xiiRenderGraphBufferHandle(), xiiRenderGraphBufferHandle());
  const xiiRenderPipelineInputBinding supplied(graph, atlasInstance.GetPipeline()->FindInterface("Spots", true), binding);
  XII_TEST_BOOL(atlasInstance.Build(nullptr, graph, context.GetBlackboard(), {}, xiiMakeArrayPtr(&supplied, 1)).Failed());
  XII_TEST_BOOL(atlasInstance.GetLastBuildError().FindSubString("2D image limit") != nullptr);
  graph.EndSetup();
  frame.End();
}
