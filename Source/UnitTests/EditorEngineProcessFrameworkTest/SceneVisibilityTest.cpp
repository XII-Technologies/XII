/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  class VisibilityTestRenderData : public xiiRenderData
  {
    XII_ADD_DYNAMIC_REFLECTION(VisibilityTestRenderData, xiiRenderData);
  };

  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(VisibilityTestRenderData, 1, xiiRTTIDefaultAllocator<VisibilityTestRenderData>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  class VisibilityTestComponent;
  using VisibilityTestManager = xiiComponentManager<VisibilityTestComponent, xiiBlockStorageType::FreeList>;

  class VisibilityTestComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(VisibilityTestComponent, xiiComponent, VisibilityTestManager);

  public:
    mutable xiiUInt32 m_uiExtractions = 0;
    bool              m_bCache        = true;

    void Extract(xiiMsgExtractRenderData& message) const
    {
      ++m_uiExtractions;
      auto* data = GetWorld()->GetModule<xiiRenderWorldModule>()->CreateRenderDataForThisFrame<VisibilityTestRenderData>(this);
      message.AddRenderData(data, m_bCache ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
    }
  };

  // clang-format off
  XII_BEGIN_COMPONENT_TYPE(VisibilityTestComponent, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, Extract),
    }
    XII_END_MESSAGEHANDLERS;
  }
  XII_END_COMPONENT_TYPE;
  // clang-format on

  bool ContainsObject(const xiiView& view, const xiiGameObject& object)
  {
    for (auto* data : view.GetExtractedRenderData()->GetAllRenderData())
    {
      if (data->m_hOwnerObject == object.GetHandle())
        return true;
    }
    return false;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneVisibility)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Scene visibility test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto* module       = world.GetOrCreateModule<xiiRenderWorldModule>();
  auto* manager      = world.GetOrCreateComponentManager<VisibilityTestManager>();
  auto  createObject = [&](VisibilityTestComponent*& component, xiiGameObjectHandle parent = {}) {
    xiiGameObject*           object = nullptr;
    xiiGameObjectDescription objectDescription;
    objectDescription.m_hParent = parent;
    world.CreateObject(objectDescription, object);
    manager->CreateComponent(object, component);
    return object;
  };

  VisibilityTestComponent *plainComponent, *firstComponent, *secondComponent, *hiddenComponent;
  auto*                    hidden = createObject(hiddenComponent);
  auto*                    plain  = createObject(plainComponent, hidden->GetHandle());
  auto*                    first  = createObject(firstComponent);
  auto*                    second = createObject(secondComponent);
  secondComponent->m_bCache       = false;
  const auto& firstTag            = xiiTagRegistry::GetGlobalRegistry().RegisterTag("VisibilityTestFirst");
  const auto& secondTag           = xiiTagRegistry::GetGlobalRegistry().RegisterTag("VisibilityTestSecond");
  const auto& hiddenTag           = xiiTagRegistry::GetGlobalRegistry().RegisterTag("VisibilityTestHidden");
  first->SetTag(firstTag);
  second->SetTag(secondTag);
  hidden->SetTag(firstTag);
  hidden->SetTag(hiddenTag);

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Size      = {16, 16};
  textureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  textureDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto texture                   = environment.GetDevice()->CreateTexture(textureDescription);
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView *unfiltered = nullptr, *filtered = nullptr;
  module->CreateView("Unfiltered", unfiltered);
  module->CreateView("Filtered", filtered);
  struct ClearPassData
  {
    xiiRenderGraphTextureHandle m_Target;
  };
  for (auto* view : {unfiltered, filtered})
  {
    view->SetCamera(&camera);
    view->SetViewport(xiiRectFloat(0, 0, 16, 16));
    view->SetRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetRenderEnabled(true);
    // Exercise normal world extraction and execution without making this visibility test depend on scene shading.
    view->SetRenderGraphBuilder([&texture](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
      graph.AddPass<ClearPassData>("Visibility test output", xiiGALCommandQueueFlags::Graphics, [&texture](ClearPassData& data, xiiRenderGraphBuilder& builder) {
          auto target = builder.ImportTexture("Visibility test target", texture, texture->GetResourceState());
          data.m_Target = builder.WriteTexture(target, xiiGALResourceStateFlags::RenderTarget); }, [](const ClearPassData& data, xiiRenderGraphPassContext& context) { context.GetCommandList().ClearRenderTargetView(context.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); }, true);
    });
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Independent views, include OR and exclusion precedence")
  {
    filtered->m_IncludeTags.Set(firstTag);
    filtered->m_IncludeTags.Set(secondTag);
    filtered->m_ExcludeTags.Set(hiddenTag);
    world.Update();
    XII_TEST_INT(unfiltered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 4);
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 2);
    XII_TEST_BOOL(ContainsObject(*filtered, *first));
    XII_TEST_BOOL(ContainsObject(*filtered, *second));
    XII_TEST_BOOL(!ContainsObject(*filtered, *hidden));
    XII_TEST_BOOL(!ContainsObject(*filtered, *plain));

    const auto firstExtractions  = firstComponent->m_uiExtractions;
    const auto secondExtractions = secondComponent->m_uiExtractions;
    world.Update();
    XII_TEST_INT(firstComponent->m_uiExtractions, firstExtractions);
    XII_TEST_INT(secondComponent->m_uiExtractions, secondExtractions + 2);
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetStaticRenderData().GetCount(), 1);
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetDynamicRenderData().GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Filters change after static cache population")
  {
    filtered->m_ExcludeTags.Set(firstTag);
    world.Update();
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    XII_TEST_BOOL(ContainsObject(*unfiltered, *first));
    filtered->m_ExcludeTags.Remove(firstTag);
    world.Update();
    XII_TEST_BOOL(ContainsObject(*filtered, *first));

    filtered->m_IncludeTags.Clear();
    world.Update();
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 3);
    XII_TEST_BOOL(ContainsObject(*filtered, *plain));
    filtered->m_IncludeTags.Set(secondTag);
    world.Update();
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    filtered->m_IncludeTags.Clear();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Object tags and activation change with cached render data")
  {
    first->SetTag(hiddenTag);
    second->SetTag(hiddenTag);
    world.Update();
    XII_TEST_INT(filtered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    XII_TEST_INT(unfiltered->GetExtractedRenderData()->GetAllRenderData().GetCount(), 4);
    first->RemoveTag(hiddenTag);
    second->RemoveTag(hiddenTag);
    plain->SetActiveFlag(false);
    world.Update();
    XII_TEST_BOOL(!ContainsObject(*unfiltered, *plain));
    XII_TEST_BOOL(!ContainsObject(*filtered, *plain));
    XII_TEST_BOOL(ContainsObject(*filtered, *first));
    XII_TEST_BOOL(ContainsObject(*filtered, *second));
    plain->SetActiveFlag(true);
    world.Update();
    XII_TEST_BOOL(ContainsObject(*unfiltered, *plain));
    XII_TEST_BOOL(ContainsObject(*filtered, *plain));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tags are per object, activation follows the hierarchy")
  {
    XII_TEST_BOOL(!ContainsObject(*filtered, *hidden));
    XII_TEST_BOOL(ContainsObject(*filtered, *plain));
    hidden->SetActiveFlag(false);
    world.Update();
    XII_TEST_BOOL(!ContainsObject(*filtered, *plain));
    XII_TEST_BOOL(!ContainsObject(*unfiltered, *hidden));
    hidden->SetActiveFlag(true);
    world.Update();
    XII_TEST_BOOL(ContainsObject(*filtered, *plain));
    XII_TEST_BOOL(ContainsObject(*unfiltered, *hidden));
  }
}
