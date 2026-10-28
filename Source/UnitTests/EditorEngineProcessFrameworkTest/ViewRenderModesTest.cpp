/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/ViewRenderModePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

#include <bit>

class xiiViewModeTestCapture : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewModeTestCapture, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Color", xiiRenderPipelinePin::Texture, true});
    pins.PushBack({"Output", xiiRenderPipelinePin::Texture, false});
    pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
    for (const auto name : {"Depth", "Surface"})
    {
      pins.PushBack({name, xiiRenderPipelinePin::Texture, true});
      pins.PeekBack().m_bOptional = true;
      pins.PushBack({name, xiiRenderPipelinePin::Texture, false});
      pins.PeekBack().m_bOptional = true;
      pins.PeekBack().m_Access    = xiiRenderPipelinePinAccess::Read;
    }
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_hTexture, m_hDepth, m_hSurface;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
        data.m_hTexture = builder.ReadTexture(context.GetInputTexture("Color"), xiiGALResourceStateFlags::CopySource);
        builder.ExportTexture(data.m_hTexture, xiiGALResourceStateFlags::CopySource);
        const auto exportOptional = [&](xiiStringView name) {
          auto texture = context.GetInputTexture(name);
          if (texture.IsValid())
          {
            texture = builder.ReadTexture(texture, xiiGALResourceStateFlags::CopySource);
            builder.ExportTexture(texture, xiiGALResourceStateFlags::CopySource);
          }
          return texture;
        };
        data.m_hDepth = exportOptional("Depth");
        data.m_hSurface = exportOptional("Surface"); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    if (pass.first->m_hDepth.IsValid())
      XII_SUCCEED_OR_RETURN(context.SetOutput("Depth", pass.first->m_hDepth));
    if (pass.first->m_hSurface.IsValid())
      XII_SUCCEED_OR_RETURN(context.SetOutput("Surface", pass.first->m_hSurface));
    return context.SetOutput("Output", pass.first->m_hTexture);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewModeTestCapture, 1, xiiRTTIDefaultAllocator<xiiViewModeTestCapture>)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  void TestViewRenderModes(bool enableMeshShaders)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(enableMeshShaders)))
      return;
    if (enableMeshShaders && environment.GetDevice()->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
    {
      xiiTestFramework::Output(xiiTestOutput::Message, "Native Forward+ mesh shaders are unavailable; this variant was not exercised.");
      return;
    }
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    xiiRenderPipelineResourceDescriptor definition;
    if (!XII_TEST_RESULT(xiiLoadPipelineTestDescriptor(definition, xiiLoadPipelineLibraryResource("Editor"))))
      return;
    xiiUInt32 visualization = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < definition.m_Passes.GetCount(); ++i)
      if (definition.m_Passes[i].m_pType == xiiGetStaticRTTI<xiiViewRenderModePass>())
        visualization = i;
    if (!XII_TEST_BOOL(visualization != xiiInvalidIndex))
      return;
    const auto capture = definition.AddPass("Read actual diagnostic pixels", xiiGetStaticRTTI<xiiViewModeTestCapture>());
    XII_TEST_RESULT(definition.Connect(visualization, "DisplayLinearColor", capture, "Color"));
    const auto connections = definition.m_Connections;
    for (const auto& connection : connections)
      if (connection.m_uiTarget == visualization)
      {
        if (connection.m_sInput == "SceneDepth")
          XII_TEST_RESULT(definition.Connect(connection.m_uiSource, connection.m_sOutput, capture, "Depth"));
        if (connection.m_sInput == "WireframeSurface")
          XII_TEST_RESULT(definition.Connect(connection.m_uiSource, connection.m_sOutput, capture, "Surface"));
      }
    const auto  pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("ViewRenderModes.Editor", std::move(definition));
    xiiGeometry box;
    box.AddBox(xiiVec3(1), true);
    box.AddBox(xiiVec3(1), true);
    box.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor meshDescription;
    auto&                     buffer = meshDescription.GetMeshBufferDescriptor();
    buffer.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
    auto* vertices = reinterpret_cast<xiiMeshPackedVertex*>(buffer.m_VertexData.GetData());
    for (xiiUInt32 i = 0; i < buffer.GetVertexCount(); ++i)
    {
      vertices[i].m_vNormal    = {0, 0, 1};
      vertices[i].m_vTangent   = {1, 0, 0, 1};
      vertices[i].m_vTexCoord0 = {vertices[i].m_vPosition.x + 0.5f, vertices[i].m_vPosition.y + 0.5f};
      vertices[i].m_vTexCoord1 = {0.75f, 0.125f};
      vertices[i].m_Color0     = xiiColorLinearUB(64, 128, 192, 255);
    }
    meshDescription.AddLOD(0);
    meshDescription.AddSection(24, 0, 0, 0);
    meshDescription.AddSection(12, 24, 0, 1);
    meshDescription.ComputeBounds();
    const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>("ViewRenderModes.Mesh", std::move(meshDescription));
    xiiMaterialResourceDescriptor materialDescription;
    materialDescription.m_BaseColor  = {0.2f, 0.4f, 0.8f};
    materialDescription.m_fRoughness = 0.25f;
    materialDescription.m_fMetallic  = 0.75f;
    const auto          material     = xiiResourceManager::CreateResource<xiiMaterialResource>("ViewRenderModes.Material", std::move(materialDescription));
    xiiWorldDescription worldDescription("Analytical diagnostic geometry");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiGameObject* object = nullptr;
    world.CreateObject(xiiGameObjectDescription(), object);
    xiiLODMeshComponent* component = nullptr;
    xiiLODMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    component->SetMaterialOverride(0, material);
    component->SetForcedLOD(0);
    xiiCamera                   cameras[2];
    xiiView*                    views[2] = {};
    xiiSharedPtr<xiiGALTexture> targets[2];
    auto*                       module = world.GetOrCreateModule<xiiRenderWorldModule>();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      cameras[i].SetCameraMode(i ? xiiCameraMode::OrthoFixedHeight : xiiCameraMode::PerspectiveFixedFovY, i ? 4.0f : 60.0f, i ? -10.0f : 0.1f, 100);
      cameras[i].LookAt({0, 0, 3}, {0, 0, 0}, {0, 1, 0});
      cameras[i].SetExposure(1);
      module->CreateView("Independent diagnostic view", views[i]);
      views[i]->SetCamera(&cameras[i]);
      views[i]->SetViewport({0, 0, 65, 49});
      views[i]->SetRenderPipelineResource(pipeline);
      views[i]->SetPipelineParameter("Editor.SelectionOverlay", false);
      views[i]->SetPipelineParameter("Editor.ShapeIcons", false);
      views[i]->SetPipelineParameter("Editor.Picking", false);
      views[i]->SetPipelineParameter("Editor.PickSelected", false);
      xiiDisplayOutputSettings display;
      display.m_Exposure.m_Mode = xiiExposureMode::Manual;
      XII_TEST_RESULT(views[i]->SetDisplayOutputSettings(display));
      xiiGALTextureCreationDescription target;
      target.m_Type      = xiiGALResourceDimension::Texture2D;
      target.m_Size      = {65, 49};
      target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget;
      targets[i]         = environment.GetDevice()->CreateTexture(target);
      views[i]->SetRenderTargetView(targets[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
    const auto update = [&]() {
      world.Update();
      environment.GetDevice()->WaitIdle();
    };
    for (xiiUInt32 frame = 0; frame < 6; ++frame)
      update();
    struct Pixel
    {
      XII_DECLARE_POD_TYPE();
      xiiFloat16 m_Channels[4];
    };
    const auto texture = [&](xiiView* view, xiiStringView pin) -> xiiSharedPtr<xiiGALTexture> {
      if (!XII_TEST_RESULT(view->GetLastRenderResult()))
        return nullptr;
      const auto& instance = view->GetRenderPipelineInstance();
      for (xiiUInt32 i = 0; i < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
        if (xiiDynamicCast<const xiiViewModeTestCapture*>(instance.GetPass(i)))
        {
          const auto* binding = instance.GetOutputBinding(i, pin);
          return binding ? view->GetRenderGraph()->GetExportedTexture(binding->m_hTexture) : nullptr;
        }
      XII_TEST_BOOL(false);
      return nullptr;
    };
    const auto read = [&](xiiView* view, xiiUInt32 x, xiiUInt32 y, xiiColor& result) {
      auto  image = texture(view, "Output");
      Pixel pixel;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), x, y, pixel))
        return false;
      result = {float(pixel.m_Channels[0]), float(pixel.m_Channels[1]), float(pixel.m_Channels[2]), float(pixel.m_Channels[3])};
      return true;
    };
    const auto check = [&](xiiViewRenderMode::Enum mode, xiiColor expected) {
      views[0]->SetViewRenderMode(mode);
      views[1]->SetViewRenderMode(xiiViewRenderMode::BaseColor);
      update();
      for (const auto& operation : views[0]->GetRenderPipelineInstance().GetPipeline()->GetDescriptor().m_Passes)
        if (operation.m_pType->GetTypeName() == "xiiToneMappingPass")
          for (const auto& pass : views[0]->GetRenderGraph()->GetCompiledPasses())
            if (pass.m_sName.GetView() == operation.m_sName)
              XII_TEST_BOOL(pass.m_bIsCulled);
      xiiColor actual, other, background;
      if (!read(views[0], 32, 24, actual) || !read(views[1], 32, 24, other) || !read(views[0], 0, 0, background))
        return;
      XII_TEST_FLOAT(actual.r, expected.r, 0.003f);
      XII_TEST_FLOAT(actual.g, expected.g, 0.003f);
      XII_TEST_FLOAT(actual.b, expected.b, 0.003f);
      XII_TEST_FLOAT(other.r, 0.2f, 0.003f);
      XII_TEST_FLOAT(other.g, 0.4f, 0.003f);
      XII_TEST_FLOAT(other.b, 0.8f, 0.003f);
      XII_TEST_FLOAT(background.r + background.g + background.b, 0, 0.001f);
    };
    for (const auto path : {xiiRenderPath::ForwardPlus, xiiRenderPath::Deferred})
    {
      views[0]->SetRenderPath(path);
      check(xiiViewRenderMode::VertexNormals, {0.5f, 0.5f, 1});
      check(xiiViewRenderMode::VertexTangents, {1, 0.5f, 0.5f});
      check(xiiViewRenderMode::VertexBitangents, {0.5f, 1, 0.5f});
      check(xiiViewRenderMode::VertexColors, {64.0f / 255, 128.0f / 255, 192.0f / 255});
      check(xiiViewRenderMode::UV0, {0.5f, 0.5f, 0});
      check(xiiViewRenderMode::UV1, {0.75f, 0.125f, 0});
      check(xiiViewRenderMode::StaticVsDynamic, {0.1f, 0.4f, 1});
      check(xiiViewRenderMode::NormalMap, {0.5f, 0.5f, 1});
      check(xiiViewRenderMode::Roughness, {0.25f, 0.25f, 0.25f});
      check(xiiViewRenderMode::Metallic, {0.75f, 0.75f, 0.75f});
      check(xiiViewRenderMode::Specular, {0.5f, 0.5f, 0.5f});
      check(xiiViewRenderMode::UVDensity, {0.5f, 1, 0.5f});
      check(xiiViewRenderMode::LODLevel, {192.0f / 255, 144.0f / 255, 137.0f / 255});
      check(xiiViewRenderMode::MeshletID, {192.0f / 255, 144.0f / 255, 137.0f / 255});
      check(xiiViewRenderMode::Overdraw, {0, 4.0f * xiiMath::Log2(3.0f) / 5.0f - 0.5f, 1});
      const float area = 0.5f * xiiMath::Square(49.0f / (2.0f * xiiMath::Tan(xiiAngle::MakeFromDegree(30)) * 2.5f));
      const float heat = xiiMath::Log2(area) / 12.0f;
      check(xiiViewRenderMode::TriangleSize, {4 * heat - 1.5f, 1, 2.5f - 4 * heat});
    }
    component->SetForcedLOD(1);
    check(xiiViewRenderMode::Overdraw, {0, 0.3f, 1});
    views[0]->SetViewRenderMode(xiiViewRenderMode::LODLevel);
    update();
    xiiColor lod1;
    if (read(views[0], 32, 24, lod1))
      XII_TEST_BOOL(xiiMath::Abs(lod1.r - 192.0f / 255) > 0.05f);
    // Exposure is a full-view scalar diagnostic and deliberately includes the background.
    views[0]->SetViewRenderMode(xiiViewRenderMode::Exposure);
    update();
    xiiColor exposure;
    if (read(views[0], 0, 0, exposure))
    {
      XII_TEST_FLOAT(exposure.r, 0.5f, 0.001f);
      XII_TEST_FLOAT(exposure.g, 1, 0.001f);
      XII_TEST_FLOAT(exposure.b, 0.5f, 0.001f);
    }
    cameras[0].SetExposure(16);
    update();
    if (read(views[0], 0, 0, exposure))
    {
      XII_TEST_FLOAT(exposure.r, 1, 0.001f);
      XII_TEST_FLOAT(exposure.g, 1, 0.001f);
      XII_TEST_FLOAT(exposure.b, 0, 0.001f);
    }
    cameras[0].SetExposure(1);
    // Read the resolved production output transforms, independently of scene
    // coverage. LUT entries change with grading; curves track operator/exposure.
    const auto originalDisplay = views[0]->GetDisplayOutputSettings();
    for (xiiUInt32 phase = 0; phase < 2; ++phase)
    {
      auto display                         = originalDisplay;
      display.m_ColorGrading.m_fSaturation = phase ? 0.0f : 1.0f;
      display.m_ColorGrading.m_fContrast   = phase ? 1.5f : 1.0f;
      XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(display));
      views[0]->SetViewRenderMode(xiiViewRenderMode::ColorGradingLUT);
      update();
      for (xiiUInt32 y : {0U, 24U, 48U})
        for (xiiUInt32 x : {0U, 32U, 64U})
        {
          const xiiUInt32 column = xiiUInt32((float(x) + 0.5f) / 65.0f * 256.0f);
          const float     r = float(column % 16) / 15.0f, g = float(xiiUInt32((float(y) + 0.5f) / 49.0f * 16.0f)) / 15.0f, b = float(column / 16) / 15.0f;
          const float     luminance = r * 0.2126f + g * 0.7152f + b * 0.0722f;
          xiiColor        actual;
          if (read(views[0], x, y, actual))
            for (xiiUInt32 channel = 0; channel < 3; ++channel)
              XII_TEST_FLOAT(actual.GetData()[channel], phase ? xiiMath::Max((luminance - 0.18f) * 1.5f + 0.18f, 0.0f) : xiiColor(r, g, b).GetData()[channel], 0.003f);
        }
    }
    XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(originalDisplay));
    for (const auto op : {xiiToneMappingOperator::ACES, xiiToneMappingOperator::Reinhard})
      for (float exposureValue : {1.0f, 4.0f})
      {
        auto display                  = originalDisplay;
        display.m_ToneMappingOperator = op;
        XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(display));
        cameras[0].SetExposure(exposureValue);
        views[0]->SetViewRenderMode(xiiViewRenderMode::ToneMappingCurve);
        update();
        for (xiiUInt32 x : {12U, 32U, 52U})
        {
          const float     value  = xiiMath::Pow(2.0f, (float(x) + 0.5f) / 65.0f * 20.0f - 10.0f) * exposureValue;
          const float     mapped = op == xiiToneMappingOperator::Reinhard ? value / (value + 1.0f) : xiiMath::Saturate(value * (2.51f * value + 0.03f) / (value * (2.43f * value + 0.59f) + 0.14f));
          const xiiUInt32 y      = xiiUInt32(xiiMath::Clamp((1.0f - mapped) * 49.0f - 0.5f, 0.0f, 48.0f));
          const float     line   = xiiMath::Saturate(1.5f - xiiMath::Abs((1.0f - (float(y) + 0.5f) / 49.0f - mapped) * 49.0f));
          xiiColor        actual;
          if (read(views[0], x, y, actual))
          {
            XII_TEST_FLOAT(actual.r, xiiMath::Lerp(0.015f, 0.2f, line), 0.003f);
            XII_TEST_FLOAT(actual.g, xiiMath::Lerp(0.015f, 1.0f, line), 0.003f);
          }
        }
      }
    cameras[0].SetExposure(1);
    XII_TEST_RESULT(views[0]->SetDisplayOutputSettings(originalDisplay));
    // Every advertised mode must execute the production resource composition.
    xiiGameObjectDescription sunDescription;
    sunDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), -xiiVec3::MakeAxisZ());
    xiiGameObject* sunObject       = nullptr;
    world.CreateObject(sunDescription, sunObject);
    xiiDirectionalLightComponent* sun = nullptr;
    xiiDirectionalLightComponent::CreateComponent(sunObject, sun);
    sun->SetCastShadows(false);
    sun->SetIntensityUnit(xiiPhotometricUnit::Lux);
    sun->SetIntensity(xiiMath::Pi<float>());
    const auto     sunColor = xiiEvaluateLightColor(xiiColorLinearUB(255, 255, 255), sun->GetTemperature());
    const xiiColor baseColor(0.2f, 0.4f, 0.8f);
    update();
    for (const auto path : {xiiRenderPath::ForwardPlus, xiiRenderPath::Deferred})
    {
      views[0]->SetRenderPath(path);
      check(xiiViewRenderMode::LightingOnly, {sunColor.r / (1 + sunColor.r), sunColor.g / (1 + sunColor.g), sunColor.b / (1 + sunColor.b)});
      xiiColor diffuse, specular;
      for (xiiUInt32 channel = 0; channel < 3; ++channel)
      {
        const float base            = baseColor.GetData()[channel];
        const float fresnel         = 0.01f + 0.75f * base;
        const float d               = (1.0f - fresnel) * 0.25f * base * sunColor.GetData()[channel];
        const float s               = 64.0f * fresnel * sunColor.GetData()[channel];
        diffuse.GetData()[channel]  = d / (1.0f + d);
        specular.GetData()[channel] = s / (1.0f + s);
      }
      check(xiiViewRenderMode::DiffuseOnly, diffuse);
      check(xiiViewRenderMode::SpecularOnly, specular);
      check(xiiViewRenderMode::LightComplexity, xiiColor::Black);
      views[0]->SetViewRenderMode(xiiViewRenderMode::ClusterID);
      update();
      for (xiiUInt32 i = 0; i < views[0]->GetRenderPipelineInstance().GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        const auto& instance = views[0]->GetRenderPipelineInstance();
        if (instance.GetPipeline()->GetDescriptor().m_Passes[i].m_pType->GetTypeName() != "xiiLightClusterViewPass")
          continue;
        const auto* binding = instance.GetOutputBinding(i, "Layout");
        const auto* layout  = binding ? xiiDynamicCast<const xiiLightClusterFrameData*>(binding->m_pFrameData.Borrow()) : nullptr;
        if (!XII_TEST_BOOL(layout != nullptr))
          continue;
        const auto& c        = layout->m_Constants;
        const auto  slice    = xiiUInt32(xiiMath::Log2(2.5f / c.DepthRange.x) / c.DepthRange.z * float(c.ClusterCounts.z));
        xiiUInt32   identity = (slice * c.ClusterCounts.y + 24 / c.Viewport.z) * c.ClusterCounts.x + 32 / c.Viewport.z + 1;
        identity ^= identity >> 16;
        identity *= 0x7feb352dU;
        identity ^= identity >> 15;
        identity *= 0x846ca68bU;
        identity ^= identity >> 16;
        check(xiiViewRenderMode::ClusterID, {float(identity & 255) / 255, float((identity >> 8) & 255) / 255, float((identity >> 16) & 255) / 255});
      }
    }
    sun->SetActiveFlag(false);
    xiiPointLightComponent* points[2] = {};
    for (auto*& point : points)
    {
      xiiGameObjectDescription description;
      description.m_LocalPosition = {0, 0, 2};
      xiiGameObject* owner        = nullptr;
      world.CreateObject(description, owner);
      xiiPointLightComponent::CreateComponent(owner, point);
      point->SetIntensityUnit(xiiPhotometricUnit::Candela);
      point->SetIntensity(1);
      point->SetRange(100);
      point->SetActiveFlag(false);
    }
    for (xiiUInt32 count = 1; count <= 2; ++count)
    {
      points[count - 1]->SetActiveFlag(true);
      const float heat = xiiMath::Log2(float(count + 1)) / 5;
      for (const auto path : {xiiRenderPath::ForwardPlus, xiiRenderPath::Deferred})
      {
        views[0]->SetRenderPath(path);
        check(xiiViewRenderMode::LightComplexity, {0, 4 * heat - 0.5f, 1});
      }
    }
    for (auto* point : points)
      point->SetActiveFlag(false);
    check(xiiViewRenderMode::ShadowMask, xiiColor::White);
    check(xiiViewRenderMode::ShadowCascadeIndex, xiiColor::Black);
    sunObject->MakeDynamic();
    sunObject->SetLocalRotation(xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3(-0.8f, 0, -1).GetNormalized()));
    sun->SetCastShadows(true);
    sun->SetActiveFlag(true);
    for (xiiUInt32 frame = 0; frame < 3; ++frame)
      update();
    check(xiiViewRenderMode::ShadowMask, xiiColor::White);
    xiiGameObjectDescription occluderDescription;
    occluderDescription.m_LocalPosition = {0.8f, 0, 1.5f};
    occluderDescription.m_LocalScaling  = xiiVec3(0.4f);
    xiiGameObject* occluderObject       = nullptr;
    world.CreateObject(occluderDescription, occluderObject);
    xiiStaticMeshComponent* occluder = nullptr;
    xiiStaticMeshComponent::CreateComponent(occluderObject, occluder);
    occluder->SetMesh(mesh);
    occluder->SetMaterialOverride(0, material);
    for (xiiUInt32 frame = 0; frame < 3; ++frame)
      update();
    check(xiiViewRenderMode::ShadowMask, xiiColor::Black);
    for (const auto path : {xiiRenderPath::ForwardPlus, xiiRenderPath::Deferred})
      for (float distance : {3.0f, 30.0f, 75.0f})
      {
        views[0]->SetRenderPath(path);
        cameras[0].LookAt({0, 0, distance}, {0, 0, 0}, {0, 1, 0});
        views[0]->SetViewRenderMode(xiiViewRenderMode::ShadowCascadeIndex);
        update();
        const auto& instance = views[0]->GetRenderPipelineInstance();
        bool        checked  = false;
        for (xiiUInt32 i = 0; i < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
        {
          if (instance.GetPipeline()->GetDescriptor().m_Passes[i].m_pType->GetTypeName() != "xiiShadowCascadePass")
            continue;
          const auto* binding  = instance.GetOutputBinding(i, "CascadeSnapshot");
          const auto* snapshot = binding ? xiiDynamicCast<const xiiShadowCascadeSnapshot*>(binding->m_pFrameData.Borrow()) : nullptr;
          if (!snapshot)
            continue;
          xiiUInt32 cascade = 0;
          while (cascade < snapshot->m_Cascades.GetCount() && distance - 0.5f > snapshot->m_Cascades[cascade].m_fSplitFar)
            ++cascade;
          xiiUInt32 identity = cascade < snapshot->m_Cascades.GetCount() ? cascade + 1 : 0;
          identity ^= identity >> 16;
          identity *= 0x7feb352dU;
          identity ^= identity >> 15;
          identity *= 0x846ca68bU;
          identity ^= identity >> 16;
          check(xiiViewRenderMode::ShadowCascadeIndex, {float(identity & 255) / 255, float((identity >> 8) & 255) / 255, float((identity >> 16) & 255) / 255});
          checked = true;
        }
        XII_TEST_BOOL(checked);
      }
    cameras[0].LookAt({0, 0, 3}, {0, 0, 0}, {0, 1, 0});
    occluder->SetActiveFlag(false);
    sun->SetActiveFlag(false);
    for (xiiUInt32 value = 0; value < xiiViewRenderMode::ENUM_COUNT; ++value)
    {
      const auto mode = static_cast<xiiViewRenderMode::Enum>(value);
      if (!xiiViewRenderMode::IsSupportedVisualizationMode(mode))
        continue;
      views[0]->SetViewRenderMode(mode);
      update();
      XII_TEST_RESULT_MSG(views[0]->GetLastRenderResult(), "Advertised mode %u failed.", value);
    }
    views[0]->SetViewRenderMode(xiiViewRenderMode::Default);
    update();
    xiiColor normalRendering;
    read(views[0], 32, 24, normalRendering);
    check(xiiViewRenderMode::VertexNormals, {0.5f, 0.5f, 1});
    views[0]->SetViewRenderMode(xiiViewRenderMode::Default);
    update();
    xiiColor restored;
    if (read(views[0], 32, 24, restored))
    {
      XII_TEST_FLOAT(restored.r, normalRendering.r, 0.001f);
      XII_TEST_FLOAT(restored.g, normalRendering.g, 0.001f);
      XII_TEST_FLOAT(restored.b, normalRendering.b, 0.001f);
    }
    xiiMaterialResourceDescriptor alternateDescription;
    alternateDescription.m_BaseColor  = {0.8f, 0.1f, 0.3f};
    alternateDescription.m_fMetallic  = 0.1f;
    alternateDescription.m_fRoughness = 0.9f;
    const auto alternate              = xiiResourceManager::CreateResource<xiiMaterialResource>("ViewRenderModes.AlternateMaterial", std::move(alternateDescription));
    component->SetMaterialOverride(0, alternate);
    views[0]->SetViewRenderMode(xiiViewRenderMode::BaseColor);
    update();
    xiiColor changed;
    if (read(views[0], 32, 24, changed))
    {
      XII_TEST_FLOAT(changed.r, 0.8f, 0.003f);
      XII_TEST_FLOAT(changed.g, 0.1f, 0.003f);
      XII_TEST_FLOAT(changed.b, 0.3f, 0.003f);
    }
    component->SetMaterialOverride(0, material);
    xiiGameObjectDescription behindDescription;
    behindDescription.m_LocalPosition = {0.4f, 0, -1};
    xiiGameObject* behind             = nullptr;
    world.CreateObject(behindDescription, behind);
    behind->MakeDynamic();
    xiiDynamicMeshComponent* moving = nullptr;
    xiiDynamicMeshComponent::CreateComponent(behind, moving);
    moving->SetMesh(mesh);
    moving->SetMaterialOverride(0, alternate);
    for (xiiUInt32 frame = 0; frame < 3; ++frame)
      update();
    // Read the actual edge query and solid prepass as well as the final output.
    views[0]->SetViewRenderMode(xiiViewRenderMode::Wireframe);
    update();
    xiiDynamicArray<xiiVec4U32> surfaces;
    xiiDynamicArray<float>      depths;
    xiiDynamicArray<Pixel>      edges;
    if (xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), texture(views[0], "Surface"), surfaces) &&
        xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), texture(views[0], "Depth"), depths) &&
        xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), texture(views[0], "Output"), edges))
    {
      xiiUInt32 visible = 0, occluded = 0;
      for (xiiUInt32 i = 0; i < surfaces.GetCount(); ++i)
        if (surfaces[i].x != 0)
        {
          const bool hidden = std::bit_cast<float>(surfaces[i].y) + 1e-7f < depths[i];
          visible += !hidden;
          occluded += hidden;
          XII_TEST_FLOAT(float(edges[i].m_Channels[0]), hidden ? 0.0f : 0.8f, 0.001f);
        }
      XII_TEST_BOOL(visible > 10 && occluded > 10);
    }
    views[0]->SetViewRenderMode(xiiViewRenderMode::InstanceID);
    update();
    xiiColor frontIdentity, backIdentity;
    read(views[0], 32, 24, frontIdentity);
    component->SetActiveFlag(false);
    update();
    if (read(views[0], 32, 24, backIdentity))
      XII_TEST_BOOL(xiiMath::Abs(frontIdentity.r - backIdentity.r) + xiiMath::Abs(frontIdentity.g - backIdentity.g) + xiiMath::Abs(frontIdentity.b - backIdentity.b) > 0.1f);
    views[0]->SetViewRenderMode(xiiViewRenderMode::StaticVsDynamic);
    update();
    if (read(views[0], 32, 24, changed))
    {
      XII_TEST_FLOAT(changed.r, 1, 0.003f);
      XII_TEST_FLOAT(changed.g, 0.3f, 0.003f);
      XII_TEST_FLOAT(changed.b, 0.1f, 0.003f);
    }
    views[0]->SetViewRenderMode(xiiViewRenderMode::MotionVectors);
    update();
    const auto previousObjectClip = views[0]->GetViewProjectionMatrix(xiiCameraEye::Left) * xiiVec4(0.4f, 0, -0.5f, 1);
    const auto currentObjectClip  = views[0]->GetViewProjectionMatrix(xiiCameraEye::Left) * xiiVec4(0.45f, 0, -0.5f, 1);
    behind->SetLocalPosition(xiiVec3(0.45f, 0, -1));
    update();
    xiiColor velocity;
    if (read(views[0], 32, 24, velocity))
      XII_TEST_FLOAT(velocity.r, 0.5f + 5.0f * (currentObjectClip.x / currentObjectClip.w - previousObjectClip.x / previousObjectClip.w), 0.003f);
    update();
    if (read(views[0], 32, 24, velocity))
    {
      XII_TEST_FLOAT(velocity.r, 0.5f, 0.003f);
      XII_TEST_FLOAT(velocity.g, 0.5f, 0.003f);
    }
    cameras[0].MoveGlobally(0, 0.1f, 0);
    update();
    if (read(views[0], 32, 24, velocity))
      XII_TEST_BOOL(xiiMath::Abs(velocity.r - 0.5f) + xiiMath::Abs(velocity.g - 0.5f) > 0.02f);
    moving->SetActiveFlag(false);
    component->SetActiveFlag(true);
    cameras[0].LookAt({0, 0, 3}, {0, 0, 0}, {0, 1, 0});
    auto resized   = targets[0]->GetDescription();
    resized.m_Size = {97, 73};
    targets[0]     = environment.GetDevice()->CreateTexture(resized);
    views[0]->SetRenderTargetView(targets[0]->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    views[0]->SetViewport({0, 0, 97, 73});
    views[0]->SetViewRenderMode(xiiViewRenderMode::VertexNormals);
    update();
    if (read(views[0], 48, 36, changed))
    {
      XII_TEST_FLOAT(changed.r, 0.5f, 0.003f);
      XII_TEST_FLOAT(changed.g, 0.5f, 0.003f);
      XII_TEST_FLOAT(changed.b, 1, 0.003f);
    }
    const auto reloadable = xiiMakeReloadablePipelineTestResource(pipeline, "ViewRenderModes.Reloadable");
    views[0]->SetRenderPipelineResource(reloadable);
    update();
    XII_TEST_BOOL(xiiMakeReloadablePipelineTestResource(pipeline, reloadable.GetResourceID()) == reloadable);
    update();
    if (read(views[0], 48, 36, changed))
      XII_TEST_FLOAT(changed.b, 1, 0.003f);
    object->MakeDynamic();
    object->SetLocalScaling(xiiVec3(-1, 1, 1));
    views[0]->SetViewRenderMode(xiiViewRenderMode::VertexBitangents);
    update();
    if (read(views[0], 48, 36, changed))
    {
      XII_TEST_FLOAT(changed.r, 0.5f, 0.003f);
      XII_TEST_FLOAT(changed.g, 1, 0.003f);
      XII_TEST_FLOAT(changed.b, 0.5f, 0.003f);
    }
    for (auto* view : views)
      module->DestroyView(view->GetHandle());
  }
} // namespace

XII_CREATE_SIMPLE_TEST(CanonicalRenderer, ViewRenderModes)
{
  TestViewRenderModes(false);
}

XII_CREATE_SIMPLE_TEST(CanonicalRenderer, ViewRenderModesWithMeshShaders)
{
  TestViewRenderModes(true);
}
