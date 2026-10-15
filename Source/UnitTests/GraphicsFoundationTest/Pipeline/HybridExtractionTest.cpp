/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

class xiiExtractionTestPacket : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExtractionTestPacket, xiiRenderData);

public:
  xiiUInt32 m_uiValue = 0;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractionTestPacket, 1, xiiRTTIDefaultAllocator<xiiExtractionTestPacket>)
XII_END_DYNAMIC_REFLECTED_TYPE;

using xiiExtractionTestComponentManager = xiiComponentManager<class xiiExtractionTestComponent, xiiBlockStorageType::FreeList>;
class xiiExtractionTestComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiExtractionTestComponent, xiiRenderComponent, xiiExtractionTestComponentManager);

public:
  xiiRenderExtractionCategory::Enum        m_Category      = xiiRenderExtractionCategory::Geometry;
  mutable xiiUInt32                        m_uiExtractions = 0;
  xiiUInt32                                m_uiValue       = 1;
  xiiBitflags<xiiRenderExtractionCategory> GetRenderExtractionCategories() const override { return m_Category; }
  bool                                     IsRenderDataViewIndependent() const override { return true; }
  xiiResult                                GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& alwaysVisible, xiiMsgUpdateLocalBounds&) override
  {
    bounds        = xiiBoundingBoxSphere::MakeZero();
    alwaysVisible = true;
    return XII_SUCCESS;
  }
  void SetValue(xiiUInt32 value)
  {
    m_uiValue = value;
    InvalidateCachedRenderData();
  }
  void Extract(xiiMsgExtractRenderData& msg) const
  {
    ++m_uiExtractions;
    auto* packet      = GetWorld()->GetModule<xiiRenderWorldModule>()->CreateRenderDataForThisFrame<xiiExtractionTestPacket>(this);
    packet->m_uiValue = m_uiValue;
    msg.AddRenderData(packet, GetOwner()->IsStatic() ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
  }
};
XII_BEGIN_COMPONENT_TYPE(xiiExtractionTestComponent, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, Extract),
    } XII_END_MESSAGEHANDLERS;
  }
XII_END_COMPONENT_TYPE;

class xiiExtractionDerivedModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiExtractionDerivedModule, xiiWorldModule);

public:
  explicit xiiExtractionDerivedModule(xiiWorld* world) : xiiWorldModule(world) {}
  xiiComponentHandle m_Target;
  bool               m_bObservedGeometryDemand = false;
  void               Initialize() override
  {
    auto update    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiExtractionDerivedModule::Update, this);
    update.m_Phase = xiiWorldUpdatePhase::PostTransform;
    RegisterUpdateFunction(update);
  }
  void Update(const xiiWorldModule::UpdateContext&)
  {
    xiiExtractionTestComponent* component = nullptr;
    if (!GetWorld()->TryGetComponent(m_Target, component))
      return;
    m_bObservedGeometryDemand = GetWorld()->GetModule<xiiRenderWorldModule>()->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Geometry);
    component->SetValue(11);
    m_Target.Invalidate();
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractionDerivedModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_IMPLEMENT_WORLD_MODULE(xiiExtractionDerivedModule);

class xiiExtractionDemandPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExtractionDemandPass, xiiRenderPipelinePass);

public:
  xiiUInt32                                m_uiCategories = 0;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return static_cast<xiiRenderExtractionCategory::Enum>(m_uiCategories); }
  void                                     GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult                                Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractionDemandPass, 1, xiiRTTIDefaultAllocator<xiiExtractionDemandPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Categories", m_uiCategories),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(PipelineExtraction);
XII_CREATE_SIMPLE_TEST(PipelineExtraction, HybridWorldDemand)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Views aggregate demand and reuse only requested component packets")
  {
    xiiRenderPipelineResourceDescriptor shared;
    xiiVariantDictionary                geometryProperties;
    geometryProperties.Insert("Categories", static_cast<xiiUInt32>(xiiRenderExtractionCategory::Geometry));
    shared.AddPass("Data", "xiiExtractionDemandPass", geometryProperties);
    xiiGraphDefinition definition;
    XII_TEST_RESULT(shared.MakeGraphDefinition(definition));
    xiiRenderPipelineResourceDescriptor composed;
    composed.m_Definitions.PushBack(definition);
    composed.AddSubgraph("Geometry", shared.m_Id);
    composed.AddSubgraph("Lights", shared.m_Id);
    auto& override = composed.m_Passes[1].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(shared.m_Passes[0].m_Id);
    override.m_Properties.Insert("Categories", static_cast<xiiUInt32>(xiiRenderExtractionCategory::Lights));
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(composed, compiled));
    XII_TEST_BOOL(compiled->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Geometry));
    XII_TEST_BOOL(compiled->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Lights));
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
    xiiWorldDescription description("Hybrid Extraction Test");
    description.m_bReportErrorWhenStaticObjectMoves = false;
    xiiWorld world(description);
    XII_LOCK(world.GetWriteMarker());
    auto*          module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiGameObject* object = nullptr;
    world.CreateObject(xiiGameObjectDescription(), object);
    auto*                       components = world.GetOrCreateComponentManager<xiiExtractionTestComponentManager>();
    xiiExtractionTestComponent *geometry, *light;
    components->CreateComponent(object, geometry);
    components->CreateComponent(object, light);
    light->m_Category = xiiRenderExtractionCategory::Lights;
    light->m_uiValue  = 2;
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    xiiView*                                views[4];
    const xiiRenderExtractionCategory::Enum categories[] = {xiiRenderExtractionCategory::Geometry, xiiRenderExtractionCategory::Lights, xiiRenderExtractionCategory::None, xiiRenderExtractionCategory::Geometry};
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      xiiVariantDictionary properties;
      properties.Insert("Categories", static_cast<xiiUInt32>(categories[i]));
      xiiRenderPipelineResourceDescriptor pipeline;
      pipeline.AddPass("Demand", "xiiExtractionDemandPass", properties);
      xiiStringBuilder name;
      name.SetFormat("Extraction Demand {}", i);
      const auto resource = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(name, std::move(pipeline));
      module->CreateView(name, views[i]);
      views[i]->SetCamera(&camera);
      views[i]->SetViewport(xiiRectFloat(0, 0, 16, 16));
      views[i]->SetRenderPipelineResource(resource);
      XII_TEST_BOOL(!views[i]->IsGpuSceneInitialized());
    }
    world.Update();
    XII_TEST_BOOL(module->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Geometry));
    XII_TEST_BOOL(module->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Lights));
    XII_TEST_INT(geometry->m_uiExtractions, 1);
    XII_TEST_INT(light->m_uiExtractions, 1);
    XII_TEST_INT(views[0]->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    XII_TEST_INT(views[1]->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1);
    XII_TEST_BOOL(views[2]->GetExtractedRenderData()->GetAllRenderData().IsEmpty());
    XII_TEST_BOOL(!views[2]->IsGpuSceneInitialized());
    XII_TEST_BOOL(!views[0]->IsGpuSceneInitialized() && !views[1]->IsGpuSceneInitialized());
    XII_TEST_INT(views[2]->GetExtractionStatistics().m_uiDispatchedComponents, 0);
    XII_TEST_INT(views[3]->GetExtractionStatistics().m_uiDispatchedComponents, 0);
    XII_TEST_BOOL(views[0]->GetExtractedRenderData()->GetAllRenderData()[0] == views[3]->GetExtractedRenderData()->GetAllRenderData()[0]);
    world.Update();
    XII_TEST_INT(geometry->m_uiExtractions, 1);
    XII_TEST_INT(light->m_uiExtractions, 1);
    const auto* snapshot = static_cast<const xiiExtractionTestPacket*>(views[0]->GetExtractedRenderData()->GetAllRenderData()[0]);
    geometry->SetValue(7);
    XII_TEST_INT(snapshot->m_uiValue, 1); // Invalidating the cache must not destroy this frame's packet.
    world.Update();
    XII_TEST_INT(geometry->m_uiExtractions, 2);
    XII_TEST_INT(light->m_uiExtractions, 1);
    XII_TEST_INT(static_cast<xiiExtractionTestPacket*>(views[0]->GetExtractedRenderData()->GetAllRenderData()[0])->m_uiValue, 7);
    object->SetLocalPosition(xiiVec3(5, 0, 0));
    world.Update();
    XII_TEST_FLOAT(views[0]->GetExtractedRenderData()->GetAllRenderData()[0]->m_GlobalTransform.m_vPosition.x, 5.0f, 0.0001f);
    views[0]->SetRenderEnabled(false);
    views[3]->SetRenderEnabled(false);
    world.Update();
    XII_TEST_BOOL(!module->GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Geometry));
    auto* derived     = world.GetOrCreateModule<xiiExtractionDerivedModule>();
    derived->m_Target = geometry->GetHandle();
    views[0]->SetRenderEnabled(true);
    views[3]->SetRenderEnabled(true);
    world.Update();
    XII_TEST_INT(static_cast<xiiExtractionTestPacket*>(views[0]->GetExtractedRenderData()->GetAllRenderData()[0])->m_uiValue, 11);
    XII_TEST_BOOL(derived->m_bObservedGeometryDemand);
    for (auto* view : views)
      module->DestroyView(view->GetHandle());
  }
}
