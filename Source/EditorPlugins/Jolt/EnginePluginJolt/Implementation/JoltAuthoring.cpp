/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginJolt/JoltAuthoring.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/View.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Joints/JoltJointComponent.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltAuthoringSyncObject, 1, xiiRTTIDefaultAllocator<xiiJoltAuthoringSyncObject>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Component", m_ComponentGuid),
    XII_MEMBER_PROPERTY("Visible", m_bVisible),
    XII_MEMBER_PROPERTY("AllShapes", m_bAllShapes),
    XII_MEMBER_PROPERTY("Modes", m_uiModes),
    XII_MEMBER_PROPERTY("LayerRevision", m_uiLayerRevision),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_IMPLEMENT_WORLD_MODULE(xiiJoltAuthoringWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltAuthoringWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct xiiJoltAuthoringWorldModule::State
{
  struct Selection
  {
    xiiUuid m_Document;
    xiiUuid m_Component;
    bool    m_bVisible;
  };
  xiiMap<xiiUuid, Selection>                                    m_Selection;
  xiiMap<xiiComponentHandle, xiiUniquePtr<xiiJoltShapePreview>> m_Previews;
  xiiSet<xiiComponentHandle>                                    m_Visible;
  xiiSet<xiiComponentHandle>                                    m_Selected;
  bool                                                          m_bAllShapes      = false;
  xiiUInt32                                                     m_uiLayerRevision = 0;
};

xiiJoltAuthoringSyncObject::xiiJoltAuthoringSyncObject() = default;
xiiJoltAuthoringSyncObject::~xiiJoltAuthoringSyncObject()
{
  // The document registry also guards cleanup during context destruction and engine restart.
  if (auto* context = xiiEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid()); context && context->GetWorld() == m_pWorld && m_pWorld)
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    if (auto* module = m_pWorld->GetModule<xiiJoltAuthoringWorldModule>())
      module->Remove(GetGuid());
  }
}
void xiiJoltAuthoringSyncObject::UpdateForEngine(xiiWorld* pWorld)
{
  m_pWorld = pWorld;
  pWorld->GetOrCreateModule<xiiJoltAuthoringWorldModule>()->Update(*this);
}

xiiJoltAuthoringWorldModule::xiiJoltAuthoringWorldModule(xiiWorld* pWorld) : xiiWorldModule(pWorld), m_pState(XII_DEFAULT_NEW(State)) {}
xiiJoltAuthoringWorldModule::~xiiJoltAuthoringWorldModule() = default;
void xiiJoltAuthoringWorldModule::WorldClear()
{
  m_pState->m_Previews.Clear();
  m_pState->m_Visible.Clear();
  m_pState->m_Selected.Clear();
}
void xiiJoltAuthoringWorldModule::Update(const xiiJoltAuthoringSyncObject& object)
{
  m_pState->m_Selection[object.GetGuid()] = {object.GetDocumentGuid(), object.m_ComponentGuid, object.m_bVisible};
  if (!object.m_ComponentGuid.IsValid())
  {
    m_pState->m_bAllShapes = object.m_bAllShapes;
    if (object.m_uiModes != 0 || GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    {
      xiiBitflags<xiiJoltDebugMode> modes;
      modes.SetValue(object.m_uiModes);
      auto* physics = GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>();
      physics->SetDebugModes(modes);
      if (m_pState->m_uiLayerRevision != object.m_uiLayerRevision)
      {
        physics->ReloadCollisionLayerConfig().LogFailure();
        m_pState->m_uiLayerRevision = object.m_uiLayerRevision;
      }
    }
  }
}
void xiiJoltAuthoringWorldModule::Remove(xiiUuid guid)
{
  const auto* entry = m_pState->m_Selection.GetValue(guid);
  if (entry && !entry->m_Component.IsValid())
  {
    m_pState->m_bAllShapes = false;
    if (auto* physics = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
      physics->SetDebugModes({});
  }
  m_pState->m_Selection.Remove(guid);
}

namespace
{
  template <typename Manager>
  void Gather(xiiWorld& world, xiiSet<xiiComponentHandle>& handles)
  {
    if (auto* manager = world.GetModule<Manager>())
      for (auto it = manager->GetComponents(); it.IsValid(); ++it)
        if (it->IsActiveAndInitialized())
          handles.Insert(it->GetHandle());
  }

  bool JointFrame(xiiWorld& world, xiiGameObjectHandle actorHandle, const xiiTransform& local, bool worldAnchor, xiiTransform& result)
  {
    if (actorHandle.IsInvalidated())
    {
      result          = local;
      result.m_vScale = xiiVec3(1.0f);
      return worldAnchor;
    }
    xiiGameObject* object = nullptr;
    if (!world.TryGetObject(actorHandle, object))
      return false;
    bool actor = false;
    for (const auto* component : object->GetComponents())
      if (const auto* body = xiiDynamicCast<const xiiJoltActorComponent*>(component); body && body->IsActiveAndInitialized())
        actor = true;
    if (!actor)
      return false;
    object->UpdateGlobalTransform();
    const auto& pose   = object->GetGlobalTransform();
    result.m_vPosition = pose.TransformPosition(local.m_vPosition);
    result.m_qRotation = pose.m_qRotation * local.m_qRotation;
    result.m_vScale    = xiiVec3(1.0f);
    return result.m_vPosition.IsValid() && result.m_qRotation.IsValid();
  }

  void Frame(const xiiDebugRendererContext& context, const xiiTransform& pose)
  {
    const auto           p       = pose.m_vPosition;
    xiiDebugRendererLine lines[] = {{p, p + pose.m_qRotation * xiiVec3(0.35f, 0, 0), xiiColor::Red},
                                    {p, p + pose.m_qRotation * xiiVec3(0, 0.35f, 0), xiiColor::Green},
                                    {p, p + pose.m_qRotation * xiiVec3(0, 0, 0.35f), xiiColor::Blue}};
    xiiDebugRenderer::DrawLines(context, lines, xiiColor::White);
  }

  void Joint(const xiiDebugRendererContext& context, xiiJoltJointComponent& joint, bool selected)
  {
    if (joint.GetLocalFrameA().m_vScale != xiiVec3(1.0f) || joint.GetLocalFrameB().m_vScale != xiiVec3(1.0f))
    {
      xiiDebugRenderer::Draw3DText(context, "Joint frame scales must be one", joint.GetOwner()->GetGlobalPosition(), xiiColor::Orange);
      return;
    }
    xiiTransform a, b;
    auto&        world  = *joint.GetWorld();
    const bool   validA = JointFrame(world, joint.GetActorA(), joint.GetLocalFrameA(), true, a);
    const bool   validB = JointFrame(world, joint.GetActorB(), joint.GetLocalFrameB(), false, b);
    const bool   self   = joint.GetActorA() == joint.GetActorB() && !joint.GetActorA().IsInvalidated();
    if (!validA || !validB || self)
    {
      xiiDebugRenderer::Draw3DText(context, self ? "Joint actors must be different" : "Assign valid joint actor references", joint.GetOwner()->GetGlobalPosition(), xiiColor::Orange);
      return;
    }
    Frame(context, a);
    Frame(context, b);
    const auto                 color = selected ? xiiColor::Yellow : xiiColor::CornflowerBlue;
    const xiiDebugRendererLine connection(a.m_vPosition, b.m_vPosition, color);
    xiiDebugRenderer::DrawLines(context, xiiMakeArrayPtr(&connection, 1), xiiColor::White);
    xiiDebugRenderer::DrawCross(context, a.m_vPosition, 0.12f, color);
    xiiDebugRenderer::DrawCross(context, b.m_vPosition, 0.12f, color);
    for (const auto handle : {joint.GetActorA(), joint.GetActorB()})
    {
      xiiGameObject* actor = nullptr;
      if (world.TryGetObject(handle, actor))
      {
        const xiiDebugRendererLine line(actor->GetGlobalPosition(), handle == joint.GetActorA() ? a.m_vPosition : b.m_vPosition, xiiColor::Grey);
        xiiDebugRenderer::DrawLines(context, xiiMakeArrayPtr(&line, 1), xiiColor::White);
      }
    }
    if (auto* hinge = xiiDynamicCast<xiiJoltHingeJointComponent*>(&joint))
    {
      if (hinge->GetLimitsEnabled())
        xiiDebugRenderer::DrawAngle(context, hinge->GetLowerLimit(), hinge->GetUpperLimit(), xiiColor(1, 0.6f, 0, 0.15f), color, a, xiiVec3::MakeAxisY(), xiiVec3::MakeAxisX());
      if (hinge->GetMotorMode() == xiiJoltJointMotorMode::Position)
      {
        auto target        = a;
        target.m_qRotation = a.m_qRotation * xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), hinge->GetTargetAngle());
        xiiDebugRenderer::DrawArrow(context, 0.5f, xiiColor::Cyan, target, xiiVec3::MakeAxisY());
      }
      else if (hinge->GetMotorMode() == xiiJoltJointMotorMode::Velocity)
        xiiDebugRenderer::Draw3DText(context, xiiFmt("{} rad/s", hinge->GetTargetVelocity()), a.m_vPosition, xiiColor::Cyan);
    }
    else if (auto* slider = xiiDynamicCast<xiiJoltSliderJointComponent*>(&joint))
    {
      const auto axis = a.m_qRotation * xiiVec3::MakeAxisX();
      if (slider->GetLimitsEnabled())
      {
        const auto                 low  = a.m_vPosition + axis * slider->GetLowerLimit();
        const auto                 high = a.m_vPosition + axis * slider->GetUpperLimit();
        const xiiDebugRendererLine range(low, high, color);
        xiiDebugRenderer::DrawLines(context, xiiMakeArrayPtr(&range, 1), xiiColor::White);
        xiiDebugRenderer::DrawCross(context, low, 0.1f, color);
        xiiDebugRenderer::DrawCross(context, high, 0.1f, color);
      }
      if (slider->GetMotorMode() == xiiJoltJointMotorMode::Position)
        xiiDebugRenderer::DrawCross(context, a.m_vPosition + axis * slider->GetTargetPosition(), 0.15f, xiiColor::Cyan);
      else if (slider->GetMotorMode() == xiiJoltJointMotorMode::Velocity)
        xiiDebugRenderer::Draw3DText(context, xiiFmt("{} m/s", slider->GetTargetVelocity()), a.m_vPosition, xiiColor::Cyan);
    }
    else if (auto* distance = xiiDynamicCast<xiiJoltDistanceJointComponent*>(&joint))
    {
      auto axis = b.m_vPosition - a.m_vPosition;
      axis.NormalizeIfNotZero(xiiVec3::MakeAxisX()).IgnoreResult();
      if (axis.IsValid())
      {
        xiiDebugRenderer::DrawCross(context, a.m_vPosition + axis * distance->GetMinDistance(), 0.1f, color);
        xiiDebugRenderer::DrawCross(context, a.m_vPosition + axis * distance->GetMaxDistance(), 0.1f, color);
      }
      xiiDebugRenderer::Draw3DText(context, xiiFmt("Distance {} to {} m", distance->GetMinDistance(), distance->GetMaxDistance()), b.m_vPosition, color);
    }
  }
} // namespace

void xiiJoltAuthoringWorldModule::Draw(const xiiView& view)
{
  const xiiDebugRendererContext context(view);
  // During simulation (including pause), inspection uses the existing native debug drawing.
  // Authored previews never replace the solver's shapes or constraint poses.
  if (const auto* physics = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>(); physics && physics->HasSimulationState())
  {
    m_pState->m_Previews.Clear();
    return;
  }
  auto& state = *m_pState;
  state.m_Visible.Clear();
  state.m_Selected.Clear();
  for (const auto& entry : state.m_Selection)
  {
    const auto& selected = entry.Value();
    if (!selected.m_bVisible || !selected.m_Component.IsValid())
      continue;
    if (auto* document = xiiEngineProcessDocumentContext::GetDocumentContext(selected.m_Document); document && document->GetWorld() == GetWorld())
    {
      const auto handle = document->GetContext().m_ComponentMap.GetHandle(selected.m_Component);
      if (!handle.IsInvalidated())
      {
        state.m_Visible.Insert(handle);
        state.m_Selected.Insert(handle);
      }
    }
  }
  if (state.m_bAllShapes)
  {
    Gather<xiiJoltBoxShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltSphereShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltCapsuleShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltCylinderShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltConvexHullShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltMeshShapeComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltFixedJointComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltPointJointComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltHingeJointComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltSliderJointComponentManager>(*GetWorld(), state.m_Visible);
    Gather<xiiJoltDistanceJointComponentManager>(*GetWorld(), state.m_Visible);
  }
  xiiUInt32 budget = 300000;
  for (auto handle : state.m_Visible)
  {
    if (budget == 0)
      break;
    xiiComponent* component = nullptr;
    if (!GetWorld()->TryGetComponent(handle, component) || !component->IsActiveAndInitialized())
      continue;
    const auto& tags = component->GetOwner()->GetTags();
    if (tags.IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !tags.IsAnySet(view.m_IncludeTags)))
      continue;
    const bool selected = state.m_Selected.Contains(handle);
    if (auto* joint = xiiDynamicCast<xiiJoltJointComponent*>(component))
    {
      Joint(context, *joint, selected);
      continue;
    }
    auto* shape = xiiDynamicCast<xiiJoltShapeComponent*>(component);
    if (!shape)
      continue;
    auto& preview = state.m_Previews[handle];
    if (!preview)
      preview = XII_DEFAULT_NEW(xiiJoltShapePreview);
    if (xiiJoltPhysicsWorldModule::UpdateShapePreview(*shape, *preview).Failed())
    {
      xiiDebugRenderer::Draw3DText(context, preview->m_sStatus.GetView(), shape->GetOwner()->GetGlobalPosition(), preview->m_bPending ? xiiColor::Grey : xiiColor::Orange);
      continue;
    }
    const auto     color = selected ? xiiColor::Yellow : xiiColor::CornflowerBlue;
    xiiGameObject* actor = nullptr;
    if (selected && GetWorld()->TryGetObject(preview->m_hActor, actor))
    {
      const xiiDebugRendererLine ownership(preview->m_Transform.m_vPosition, actor->GetGlobalPosition(), xiiColor::Grey);
      xiiDebugRenderer::DrawLines(context, xiiMakeArrayPtr(&ownership, 1), xiiColor::White);
      xiiDebugRenderer::Draw3DText(context, xiiFmt("Actor: {}", actor->GetName()), actor->GetGlobalPosition(), color);
    }
    const auto& geometry = preview->GetGeometry();
    for (xiiUInt32 i = 0; i < geometry.m_Lines.GetCount() && budget;)
    {
      xiiStaticArray<xiiDebugRendererLine, 256> lines;
      while (i < geometry.m_Lines.GetCount() && lines.GetCount() < 256 && budget)
      {
        const auto& line = geometry.m_Lines[i++];
        lines.PushBack({line.m_vStart, line.m_vEnd, color});
        --budget;
      }
      xiiDebugRenderer::DrawLines(context, lines, xiiColor::White, preview->m_Transform);
    }
    if (geometry.m_bTruncated)
      xiiDebugRenderer::Draw3DText(context, "Collision preview geometry budget exceeded", shape->GetOwner()->GetGlobalPosition(), xiiColor::Orange);
  }
  for (auto it = state.m_Previews.GetIterator(); it.IsValid();)
  {
    xiiComponent* component = nullptr;
    if (!state.m_Visible.Contains(it.Key()) || !GetWorld()->TryGetComponent(it.Key(), component))
      it = state.m_Previews.Remove(it);
    else
      ++it;
  }
  if (!budget)
    xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt", "Authored collision preview line budget exceeded", xiiColor::Orange);
}

XII_STATICLINK_FILE(EnginePluginJolt, EnginePluginJolt_Implementation_JoltAuthoring);
