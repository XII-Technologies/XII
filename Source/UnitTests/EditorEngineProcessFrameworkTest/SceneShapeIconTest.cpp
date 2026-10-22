/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

namespace
{
  class IconMissingComponent;
  using IconMissingManager = xiiComponentManager<IconMissingComponent, xiiBlockStorageType::FreeList>;
  class IconMissingComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(IconMissingComponent, xiiComponent, IconMissingManager);
  };
  XII_BEGIN_COMPONENT_TYPE(IconMissingComponent, 1, xiiComponentMode::Static)
  XII_END_COMPONENT_TYPE;

  class IconGammaComponent;
  using IconGammaManager = xiiComponentManager<IconGammaComponent, xiiBlockStorageType::FreeList>;
  class IconGammaComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(IconGammaComponent, xiiComponent, IconGammaManager);

  public:
    xiiColor        m_Linear = xiiColor::Lime;
    xiiColorGammaUB m_Gamma  = xiiColorGammaUB(xiiColor::Red);
  };
  XII_BEGIN_COMPONENT_TYPE(IconGammaComponent, 1, xiiComponentMode::Static)
    {
      XII_BEGIN_PROPERTIES
      {
        XII_MEMBER_PROPERTY("Linear", m_Linear),
        XII_MEMBER_PROPERTY("Gamma", m_Gamma),
      } XII_END_PROPERTIES;
    }
  XII_END_COMPONENT_TYPE;

  class IconLinearComponent;
  using IconLinearManager = xiiComponentManager<IconLinearComponent, xiiBlockStorageType::FreeList>;
  class IconLinearComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(IconLinearComponent, xiiComponent, IconLinearManager);

  public:
    xiiColor m_Color = xiiColor::Lime;
  };
  XII_BEGIN_COMPONENT_TYPE(IconLinearComponent, 1, xiiComponentMode::Static)
    {
      XII_BEGIN_PROPERTIES
      {
        XII_MEMBER_PROPERTY("Color", m_Color),
      } XII_END_PROPERTIES;
    }
  XII_END_COMPONENT_TYPE;

  class IconTestSource : public xiiRenderPipelinePass
  {
    XII_ADD_DYNAMIC_REFLECTION(IconTestSource, xiiRenderPipelinePass);

  public:
    xiiRenderPipelineTextureOutputPin m_Color;
    xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::D32Float};
    xiiResult                         Build(xiiRenderPipelinePassContext& context) override
    {
      struct Data
      {
        xiiRenderGraphTextureHandle m_Color, m_Depth;
        float                       m_fDepth = 0;
      };
      auto                        output = context.m_pView->GetRenderTargetView()->GetTexture();
      xiiRenderGraphTextureHandle color, depth;
      context.m_Graph.AddPass<Data>("Icon test source", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
        color = data.m_Color = builder.WriteTexture(builder.ImportTexture("Icon test color", output, output->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
        builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Blue); });
      context.m_Graph.AddPass<Data>("Icon test source depth", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
        auto description = output->GetDescription();
        description.m_Size = {48, 48}; // Deliberately different from display resolution.
        description.m_Format = xiiGALResourceFormat::D32Float;
        description.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        depth = data.m_Depth = builder.WriteTexture("Icon test depth", description, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged();
        if (const auto* value = context.m_Parameters.GetValue("Test.Depth")) data.m_fDepth = value->Get<float>(); }, [](const Data& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, data.m_fDepth, 0); });
      XII_SUCCEED_OR_RETURN(context.SetOutput(m_Color, color));
      return context.SetOutput(m_Depth, depth);
    }
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(IconTestSource, 1, xiiRTTIDefaultAllocator<IconTestSource>)
    {
      XII_BEGIN_PROPERTIES
      {
        XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
        XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
      } XII_END_PROPERTIES;
    }
  XII_END_DYNAMIC_REFLECTED_TYPE;

  class IconTestPublish : public xiiRenderPipelinePass
  {
    XII_ADD_DYNAMIC_REFLECTION(IconTestPublish, xiiRenderPipelinePass);

  public:
    xiiRenderPipelineTextureInputPin m_Color;
    xiiRenderPipelineTextureInputPin m_Picking{xiiGALResourceFormat::Unknown, true};
    xiiResult                        Build(xiiRenderPipelinePassContext& context) override
    {
      struct Data
      {
        XII_DECLARE_POD_TYPE();
      };
      context.m_Graph.AddPass<Data>("Icon test publish", xiiGALCommandQueueFlags::Graphics, [&](Data&, xiiRenderGraphBuilder& builder) {
        builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(m_Color), xiiGALResourceStateFlags::CopySource), xiiGALResourceStateFlags::CopySource);
        if (context.GetInputTexture(m_Picking).IsValid())
          builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(m_Picking), xiiGALResourceStateFlags::CopySource), xiiGALResourceStateFlags::CopySource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
      return XII_SUCCESS;
    }
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(IconTestPublish, 1, xiiRTTIDefaultAllocator<IconTestPublish>)
    {
      XII_BEGIN_PROPERTIES
      {
        XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
        XII_MEMBER_PROPERTY("Picking", m_Picking)->AddFlags(xiiPropertyFlags::ReadOnly),
      } XII_END_PROPERTIES;
    }
  XII_END_DYNAMIC_REFLECTED_TYPE;

  bool Capture(xiiGALDevice& device, xiiGALTexture* texture, xiiDynamicArray<xiiUInt32>& pixels)
  {
    if (!XII_TEST_BOOL(texture != nullptr))
      return false;
    xiiGALTextureReadback                readback(&device);
    xiiGALCommandListCreationDescription description;
    description.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto commands            = device.CreateCommandList(description);
    commands->Begin();
    xiiGALTextureReadback::ReadbackRequest request;
    request.m_pTexture  = texture;
    const auto enqueued = readback.Enqueue(commands.Borrow(), request);
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return false;
    auto* queue = device.GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    queue->WaitForFenceValue(queue->Submit(commands.Borrow()));
    readback.WaitForNextCompleted();
    auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    xiiGALTextureMipLevelData      mip;
    if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      commands->End();
      return false;
    }
    pixels.SetCount(capture.m_uiWidth * capture.m_uiHeight);
    for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      xiiMemoryUtils::Copy(pixels.GetData() + y * capture.m_uiWidth, xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth);
    const auto result = commands->UnmapTextureSubresource(capture.m_pStagingTexture, mip);
    commands->End();
    return XII_TEST_RESULT(result);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneShapeIcons)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  auto makeTexture = [](xiiStringView id, bool mask) {
    xiiUInt8 pixels[8 * 8 * 4];
    for (xiiUInt32 i = 0; i < 64; ++i)
    {
      pixels[i * 4] = pixels[i * 4 + 1] = pixels[i * 4 + 2] = 64;
      pixels[i * 4 + 3]                                     = mask && (i % 8) < 4 ? 0 : 255;
    }
    xiiGALTextureSubResourceData   content(xiiConstByteBlobPtr(pixels, sizeof(pixels)), 32, sizeof(pixels));
    xiiTexture2DResourceDescriptor descriptor;
    descriptor.m_TextureDescription.m_Size      = {8, 8};
    descriptor.m_TextureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    descriptor.m_InitialContent                 = xiiMakeArrayPtr(&content, 1);
    descriptor.m_SamplerDescription.m_MinFilter = xiiGALFilterType::Point;
    descriptor.m_SamplerDescription.m_MagFilter = xiiGALFilterType::Point;
    return xiiResourceManager::CreateResource<xiiTexture2DResource>(id, std::move(descriptor));
  };
  auto gammaTexture  = makeTexture("Editor/ShapeIcons/IconGammaComponent.dds", true);
  auto linearTexture = makeTexture("Editor/ShapeIcons/IconLinearComponent.dds", false);

  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Source", IconTestSource().GetDynamicRTTI());
  xiiVariantDictionary settings;
  settings.Insert("Size", 4.0f);
  settings.Insert("MaxScreenSize", 32.0f);
  definition.AddPass("Icons", xiiSceneShapeIconPass().GetDynamicRTTI(), settings);
  definition.AddPass("Publish", IconTestPublish().GetDynamicRTTI());
  XII_TEST_RESULT(definition.Connect(0, "Color", 1, "Color"));
  XII_TEST_RESULT(definition.Connect(0, "SceneDepth", 1, "SceneDepth"));
  XII_TEST_RESULT(definition.Connect(1, "Color", 2, "Color"));
  XII_TEST_RESULT(definition.Connect(1, "Picking", 2, "Picking"));
  auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneShapeIconTest", std::move(definition));

  xiiWorldDescription description("Scene shape icons");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view   = nullptr;
  module->CreateView("Shape icons", view);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 96, 96});
  xiiGALTextureCreationDescription outputDescription;
  outputDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  outputDescription.m_Size      = {96, 96};
  outputDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  outputDescription.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto output                   = environment.GetDevice()->CreateTexture(outputDescription);
  view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  view->SetRenderPipelineResource(pipeline);

  xiiGameObject* object = nullptr;
  world.CreateObject(xiiGameObjectDescription(), object);
  IconMissingComponent* noIcon = nullptr;
  world.GetOrCreateComponentManager<IconMissingManager>()->CreateComponent(object, noIcon);
  IconGammaComponent* component    = nullptr;
  auto*               gammaManager = world.GetOrCreateComponentManager<IconGammaManager>();
  gammaManager->CreateComponent(object, component);
  auto pass = [&]() {
    return xiiDynamicCast<const xiiSceneShapeIconPass*>(view->GetRenderPipelineInstance().GetPass(1));
  };
  xiiDynamicArray<xiiUInt32> colors, picking;
  auto                       capture = [&]() {
    if (!Capture(*environment.GetDevice(), output.Borrow(), colors))
      return false;
    auto*       graph   = view->GetRenderGraph();
    const auto* binding = view->GetRenderPipelineInstance().GetOutputBinding(1, "Picking");
    auto        texture = binding ? graph->GetExportedTexture(binding->m_hTexture) : nullptr;
    return Capture(*environment.GetDevice(), texture.Borrow(), picking);
  };
  auto channel = [&](xiiUInt32 x, xiiUInt32 y, xiiUInt32 c) {
    return static_cast<xiiUInt8>((colors[y * 96 + x] >> (8 * c)) & 255);
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native resource pipeline, reflected gamma color, alpha-matched picking")
  {
    world.Update();
    if (!XII_TEST_BOOL(pass() != nullptr))
      return;
    XII_TEST_INT(pass()->GetIcons().GetCount(), 1);
    XII_TEST_BOOL(pass()->GetIcons()[0].m_hComponent == component->GetHandle());
    XII_TEST_FLOAT(pass()->GetIcons()[0].m_vScreenSize.x * 96, 32, 0.001f);
    XII_TEST_BOOL(pass()->GetIcons()[0].m_Color.IsEqualRGBA(xiiColor::Red, 0.001f));
    if (capture())
    {
      XII_TEST_INT(picking[48 * 96 + 40], 0);
      XII_TEST_INT(channel(40, 48, 2), 255);
      XII_TEST_INT(picking[48 * 96 + 56], 1);
      XII_TEST_FLOAT(channel(56, 48, 0), 188, 1);
      XII_TEST_INT(channel(56, 48, 1), 0);
      XII_TEST_INT(channel(56, 48, 2), 0);
      XII_TEST_INT(picking[12 * 96 + 12], 0);
      xiiSceneShapeIconPass::PickingResult hit;
      XII_TEST_RESULT(pass()->PickIcon(*view, 56, 48, hit));
      XII_TEST_BOOL(hit.m_hObject == object->GetHandle());
      XII_TEST_BOOL(hit.m_hComponent == component->GetHandle());
      XII_TEST_VEC3(hit.m_vWorldPosition, xiiVec3::MakeZero(), 0);
      XII_TEST_RESULT(pass()->PickIcon(*view, 40, 48, hit));
      XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
      XII_TEST_BOOL(hit.m_hComponent.IsInvalidated());
      XII_TEST_VEC3(hit.m_vWorldPosition, xiiVec3::MakeZero(), 0);
      XII_TEST_BOOL(pass()->PickIcon(*view, 96, 48, hit).Failed());
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scene depth occludes both color and IDs at a different resolution")
  {
    view->SetPipelineParameter("Test.Depth", 1.0f);
    world.Update();
    if (capture())
    {
      XII_TEST_INT(picking[48 * 96 + 56], 0);
      XII_TEST_INT(channel(56, 48, 2), 255);
    }
    view->SetPipelineParameter("Test.Depth", 0.0f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Closest visible icon wins, with no duplicate icon for multiple components")
  {
    xiiGameObjectDescription farther;
    farther.m_LocalPosition = xiiVec3(2, 0, 0);
    xiiGameObject* behind   = nullptr;
    world.CreateObject(farther, behind);
    IconLinearComponent* linear        = nullptr;
    auto*                linearManager = world.GetOrCreateComponentManager<IconLinearManager>();
    linearManager->CreateComponent(behind, linear);
    IconLinearComponent* extra = nullptr;
    linearManager->CreateComponent(object, extra);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 2);
    XII_TEST_BOOL(pass()->GetIcons()[0].m_hObject == behind->GetHandle());
    XII_TEST_BOOL(pass()->GetIcons()[1].m_hObject == object->GetHandle());
    if (capture())
    {
      XII_TEST_INT(picking[48 * 96 + 40], 1);
      XII_TEST_FLOAT(channel(40, 48, 1), 188, 1);
      XII_TEST_INT(picking[48 * 96 + 56], 2);
      XII_TEST_FLOAT(channel(56, 48, 0), 188, 1);
      xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> hits;
      XII_TEST_RESULT(pass()->PickIconsInRect(*view, {32, 32, 32, 32}, hits));
      if (XII_TEST_INT(hits.GetCount(), 2))
      {
        XII_TEST_BOOL(hits[0].m_hObject == behind->GetHandle());
        XII_TEST_BOOL(hits[1].m_hObject == object->GetHandle());
      }
      XII_TEST_RESULT(pass()->PickIconsInRect(*view, {0, 0, 16, 16}, hits));
      XII_TEST_INT(hits.GetCount(), 0);
      XII_TEST_BOOL(pass()->PickIconsInRect(*view, {90, 0, 16, 16}, hits).Failed());
      XII_TEST_INT(hits.GetCount(), 0);
    }
    world.DeleteObjectNow(behind->GetHandle());
    linearManager->DeleteComponent(extra->GetHandle());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Object/component visibility, view filters and stale ID clearing")
  {
    const auto& include = xiiTagRegistry::GetGlobalRegistry().RegisterTag("ShapeIconIncluded");
    const auto& exclude = xiiTagRegistry::GetGlobalRegistry().RegisterTag("ShapeIconExcluded");
    view->m_IncludeTags.Set(include);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
    if (capture())
      XII_TEST_INT(picking[48 * 96 + 56], 0);
    object->SetTag(include);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 1);
    view->m_ExcludeTags.Set(exclude);
    object->SetTag(exclude);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
    object->RemoveTag(exclude);
    component->SetActiveFlag(false);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
    component->SetActiveFlag(true);
    object->SetActiveFlag(false);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
    object->SetActiveFlag(true);
    object->SetHideShapeIcon();
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Projected-quad culling, perspective scale and orthographic scale")
  {
    view->m_IncludeTags.Clear();
    auto create = [&](const xiiVec3& position) {
      xiiGameObjectDescription objectDescription;
      objectDescription.m_LocalPosition = position;
      xiiGameObject* created            = nullptr;
      world.CreateObject(objectDescription, created);
      IconLinearComponent* linear = nullptr;
      world.GetOrCreateComponentManager<IconLinearManager>()->CreateComponent(created, linear);
      return created;
    };
    auto* far          = create(xiiVec3(48, 0, 0));
    auto* partial      = create(camera.GetCenterPosition() + camera.GetCenterDirForwards() * 2.0f + camera.GetCenterDirRight() * (-1.1f * 2 * 0.57735027f));
    auto* offscreen    = create(xiiVec3(0, 100, 0));
    auto* behindCamera = create(xiiVec3(-3, 0, 0));
    auto* hidden       = create(xiiVec3::MakeZero());
    hidden->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden"));
    auto* editorObject = create(xiiVec3::MakeZero());
    editorObject->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor"));
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 2);
    XII_TEST_BOOL(pass()->GetIcons()[0].m_hObject == far->GetHandle());
    XII_TEST_FLOAT(pass()->GetIcons()[0].m_vScreenSize.y * 96, 6.651075f, 0.001f);
    XII_TEST_BOOL(pass()->GetIcons()[1].m_hObject == partial->GetHandle());
    XII_TEST_FLOAT(pass()->GetIcons()[1].m_vScreenPosition.x, -0.05f, 0.001f);
    if (capture())
    {
      XII_TEST_INT(picking[48 * 96], 2);
      XII_TEST_FLOAT(channel(0, 48, 1), 188, 1);
    }
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 24, 0.1f, 100);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 2);
    for (const auto& icon : pass()->GetIcons())
      XII_TEST_FLOAT(icon.m_vScreenSize.y * 96, 16, 0.001f);
    for (auto* created : {far, partial, offscreen, behindCamera, hidden, editorObject})
      world.DeleteObjectNow(created->GetHandle());
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared definitions retain independent pass instances and visibility")
  {
    xiiGameObject* independentObject = nullptr;
    world.CreateObject(xiiGameObjectDescription(), independentObject);
    IconLinearComponent* independentComponent = nullptr;
    world.GetOrCreateComponentManager<IconLinearManager>()->CreateComponent(independentObject, independentComponent);
    xiiView*   other  = nullptr;
    const auto handle = module->CreateView("Independent shape icons", other);
    other->SetCamera(&camera);
    other->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
    other->SetViewport({0, 0, 96, 96});
    auto otherOutput = environment.GetDevice()->CreateTexture(outputDescription);
    other->SetRenderTargetView(otherOutput->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    other->SetRenderPipelineResource(pipeline);
    other->SetPipelineParameter("Editor.ShapeIcons", false);
    world.Update();
    const auto* otherPass = xiiDynamicCast<const xiiSceneShapeIconPass*>(other->GetRenderPipelineInstance().GetPass(1));
    if (XII_TEST_BOOL(otherPass != nullptr))
    {
      XII_TEST_INT(pass()->GetIcons().GetCount(), 1);
      XII_TEST_BOOL(otherPass != pass());
      XII_TEST_BOOL(other->GetRenderPipelineInstance().GetPipeline() == view->GetRenderPipelineInstance().GetPipeline());
      XII_TEST_INT(otherPass->GetIcons().GetCount(), 0);
      XII_TEST_BOOL(!other->GetRenderGraph()->FindTexture("Icons: Picking").IsValid());
      environment.GetDevice()->WaitIdle();
      xiiSceneShapeIconPass::PickingResult hit;
      XII_TEST_RESULT(pass()->PickIcon(*view, 48, 48, hit));
      XII_TEST_BOOL(hit.m_hObject == independentObject->GetHandle());
      XII_TEST_BOOL(pass()->PickIcon(*other, 48, 48, hit).Failed());
      XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
    }
    other->SetPipelineParameter("Editor.Picking", true);
    other->SetPipelineParameter("Editor.ShapeIcons", true);
    world.Update(); // Destroy a view with a freshly submitted picking capture; no CPU wait.
    module->DestroyView(handle);
    world.DeleteObjectNow(independentObject->GetHandle());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nonblocking snapshots reject camera changes, deleted identities and disabled captures")
  {
    xiiGameObject*      pickedObject    = nullptr;
    const auto          pickedHandle    = world.CreateObject(xiiGameObjectDescription(), pickedObject);
    IconGammaComponent* pickedComponent = nullptr;
    gammaManager->CreateComponent(pickedObject, pickedComponent);
    world.Update();
    environment.GetDevice()->WaitIdle();
    // World object storage may compact when earlier blocks' deleted objects are reclaimed.
    if (!XII_TEST_BOOL(world.TryGetObject(pickedHandle, pickedObject)))
      return;
    xiiSceneShapeIconPass::PickingResult hit;
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 48, hit));
    XII_TEST_BOOL(hit.m_hComponent == pickedComponent->GetHandle());
    auto replacedOutput = environment.GetDevice()->CreateTexture(outputDescription);
    view->SetRenderTargetView(replacedOutput->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    XII_TEST_BOOL(pass()->PickIcon(*view, 56, 48, hit).Failed());
    view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    camera.MoveGlobally(0, 0.25f, 0);
    XII_TEST_BOOL(pass()->PickIcon(*view, 56, 48, hit).Failed());
    XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
    camera.MoveGlobally(0, -0.25f, 0);
    view->SetViewport({16, 8, 64, 64});
    XII_TEST_BOOL(pass()->PickIcon(*view, 56, 48, hit).Failed());
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hObject == pickedHandle);
    XII_TEST_BOOL(pass()->PickIcon(*view, 0, 0, hit).Failed());
    const auto oldHandle = pickedComponent->GetHandle();
    gammaManager->DeleteComponent(oldHandle);
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hComponent.IsInvalidated());
    gammaManager->CreateComponent(pickedObject, pickedComponent);
    XII_TEST_BOOL(pickedComponent->GetHandle() != oldHandle);
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hComponent.IsInvalidated());
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 1);
    if (XII_TEST_BOOL(!pass()->GetIcons().IsEmpty()))
      XII_TEST_BOOL(pass()->GetIcons()[0].m_hComponent == pickedComponent->GetHandle());
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hComponent == pickedComponent->GetHandle());
    pickedObject->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden"));
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
    pickedObject->RemoveTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden"));
    view->SetPipelineParameter("Editor.Picking", false);
    world.Update();
    XII_TEST_BOOL(pass()->PickIcon(*view, 56, 40, hit).Failed());
    view->SetPipelineParameter("Editor.Picking", true);
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hObject == pickedHandle);
    world.DeleteObjectNow(pickedHandle);
    XII_TEST_RESULT(pass()->PickIcon(*view, 56, 40, hit));
    XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
    view->SetViewport({0, 0, 96, 96});
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resize drains old captures and replaces the staging pool")
  {
    auto resizedDescription   = outputDescription;
    resizedDescription.m_Size = {64, 64};
    auto resized              = environment.GetDevice()->CreateTexture(resizedDescription);
    view->SetRenderTargetView(resized->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetViewport({0, 0, 64, 64});
    world.Update();
    environment.GetDevice()->WaitIdle();
    world.Update();
    environment.GetDevice()->WaitIdle();
    xiiSceneShapeIconPass::PickingResult hit;
    XII_TEST_RESULT(pass()->PickIcon(*view, 32, 32, hit));
    XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
    view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetViewport({0, 0, 96, 96});
    for (xiiUInt32 i = 0; i < 8; ++i)
      world.Update();
    environment.GetDevice()->WaitIdle();
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_RESULT(pass()->PickIcon(*view, 48, 48, hit));
    XII_TEST_BOOL(hit.m_hObject.IsInvalidated());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid pass settings fail before emitting graph work")
  {
    xiiSceneShapeIconPass                 invalid;
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    invalid.GetPins(pins);
    XII_TEST_RESULT(invalid.InitializePinBindings(pins));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    xiiVariantDictionary         parameters;
    xiiRenderGraphBlackboard     blackboard;
    xiiRenderPipelinePassContext context{view, *view->GetRenderGraph(), blackboard, parameters, pins, bindings, "Invalid icon settings"};
    invalid.m_fSize = -1;
    XII_TEST_BOOL(invalid.Build(context).Failed());
    invalid.m_fSize = xiiMath::NaN<float>();
    XII_TEST_BOOL(invalid.Build(context).Failed());
    invalid.m_fSize          = 1;
    invalid.m_fMaxScreenSize = -1;
    XII_TEST_BOOL(invalid.Build(context).Failed());
    XII_TEST_INT(invalid.GetIcons().GetCount(), 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Disable and camera preview usage pass through without a picking texture")
  {
    view->SetPipelineParameter("Editor.ShapeIcons", false);
    world.Update();
    XII_TEST_INT(pass()->GetIcons().GetCount(), 0);
    XII_TEST_BOOL(!view->GetRenderPipelineInstance().GetOutputBinding(1, "Picking")->m_hTexture.IsValid());
    view->SetPipelineParameter("Editor.ShapeIcons", true);
    view->SetCameraUsageHint(xiiCameraUsageHint::RenderTarget);
    world.Update();
    XII_TEST_BOOL(!view->GetRenderPipelineInstance().GetOutputBinding(1, "Picking")->m_hTexture.IsValid());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit icon pipeline is a valid serializable native definition")
  {
    xiiResourceLock<xiiRenderPipelineResource> loaded(pipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
      return;
    auto                                  native = loaded->GetAuthoring()->m_Descriptor;
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    xiiStringBuilder                      error;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(native, compiled, &error));
    XII_TEST_BOOL_MSG(compiled != nullptr, "%s", error.GetData());
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(native.Serialize(writer));
    xiiMemoryStreamReader               reader(&storage);
    xiiRenderPipelineResourceDescriptor restored;
    XII_TEST_RESULT(restored.Deserialize(reader));
    XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, compiled, &error));
  }
}
