/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/RaycastComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

xiiRaycastComponentManager::xiiRaycastComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

void xiiRaycastComponentManager::Initialize()
{
  // we want to do the raycast as late as possible, ie. after animated objects and characters moved
  // such that we get the latest position that is in sync with those animated objects
  // therefore we move the update into the post async phase and set a low priority (low = updated late)
  // we DO NOT want to use post transform update, because when we move the target object
  // child objects of the target node should still get the full global transform update within this frame

  auto desc                        = xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&xiiRaycastComponentManager::Update, this), "xiiRaycastComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
  desc.m_fPriority                 = -1000;

  this->RegisterUpdateFunction(desc);
}

void xiiRaycastComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRaycastComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("DisableTargetObjectOnNoHit", m_bDisableTargetObjectOnNoHit),
    XII_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_MEMBER_PROPERTY("ForceTargetParentless", m_bForceTargetParentless),
    XII_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", xiiPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new xiiDefaultValueAttribute(xiiVariant(xiiPhysicsShapeType::Default & ~(xiiPhysicsShapeType::Trigger)))),
    XII_MEMBER_PROPERTY("CollisionLayerEndPoint", m_uiCollisionLayerEndPoint)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("CollisionLayerTrigger", m_uiCollisionLayerTrigger)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColor::YellowGreen),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRaycastComponent::xiiRaycastComponent()  = default;
xiiRaycastComponent::~xiiRaycastComponent() = default;

void xiiRaycastComponent::Deinitialize()
{
  if (m_bForceTargetParentless)
  {
    // see end of xiiRaycastComponent::Update() for details
    GetWorld()->DeleteObjectDelayed(m_hRaycastEndObject);
  }

  SUPER::Deinitialize();
}

void xiiRaycastComponent::OnActivated()
{
  SUPER::OnActivated();
}

void xiiRaycastComponent::OnDeactivated()
{
  if (m_bDisableTargetObjectOnNoHit && m_bForceTargetParentless)
  {
    xiiGameObject* pEndObject = nullptr;
    if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
    {
      pEndObject->SetActiveFlag(false);
    }
  }

  SUPER::OnDeactivated();
}

void xiiRaycastComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();
  m_hLastTriggerObjectInRay.Invalidate();

  xiiGameObject* pEndObject = nullptr;
  if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    if (!pEndObject->IsDynamic())
    {
      pEndObject->MakeDynamic();
    }
  }
}

void xiiRaycastComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hRaycastEndObject);
  s << m_fMaxDistance;
  s << m_bDisableTargetObjectOnNoHit;
  s << m_uiCollisionLayerEndPoint;
  s << m_uiCollisionLayerTrigger;
  s << m_sTriggerMessage;
  s << m_bForceTargetParentless;
  s << m_ShapeTypesToHit;
}

void xiiRaycastComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  m_hRaycastEndObject = inout_stream.ReadGameObjectHandle();
  s >> m_fMaxDistance;
  s >> m_bDisableTargetObjectOnNoHit;
  s >> m_uiCollisionLayerEndPoint;
  s >> m_uiCollisionLayerTrigger;
  s >> m_sTriggerMessage;

  if (uiVersion >= 2)
  {
    s >> m_bForceTargetParentless;
  }

  if (uiVersion >= 3)
  {
    s >> m_ShapeTypesToHit;
  }
}

void xiiRaycastComponent::SetTriggerMessage(const char* szSz)
{
  m_sTriggerMessage.Assign(szSz);
}

const char* xiiRaycastComponent::GetTriggerMessage() const
{
  return m_sTriggerMessage.GetData();
}

void xiiRaycastComponent::SetRaycastEndObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hRaycastEndObject = resolver(szReference, GetHandle(), "RaycastEndObject");
}

void xiiRaycastComponent::Update()
{
  if (m_hRaycastEndObject.IsInvalidated())
    return;

  if (!m_pPhysicsWorldModule)
  {
    // Happens in Prefab viewports
    return;
  }

  xiiGameObject* pEndObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    // early out in the future
    m_hRaycastEndObject.Invalidate();
    return;
  }

  // if the owner object moved this frame, we want the latest global position as the ray starting position
  // this is especially important when the raycast component is attached to something that animates
  GetOwner()->UpdateGlobalTransform();

  const xiiVec3 rayStartPosition = GetOwner()->GetGlobalPosition();
  const xiiVec3 rayDir           = GetOwner()->GetGlobalDirForwards().GetNormalized(); // PhysX is very picky about normalized vectors

  float                fHitDistance = m_fMaxDistance;
  xiiPhysicsCastResult hit;

  {
    xiiPhysicsQueryParameters queryParams(m_uiCollisionLayerEndPoint);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes            = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(hit, rayStartPosition, rayDir, m_fMaxDistance, queryParams))
    {
      fHitDistance = hit.m_fDistance;

      if (!pEndObject->GetActiveFlag() && m_bDisableTargetObjectOnNoHit)
      {
        pEndObject->SetActiveFlag(true);
      }
    }
    else
    {
      if (m_bDisableTargetObjectOnNoHit)
      {
        pEndObject->SetActiveFlag(false);
      }
      else
      {
        if (!pEndObject->GetActiveFlag())
        {
          pEndObject->SetActiveFlag(true);
        }
      }
    }
  }

  if (false)
  {
    xiiDebugRendererLine lines[] = {{rayStartPosition, rayStartPosition + rayDir * fHitDistance}};
    xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::GreenYellow);
  }

  if (!m_sTriggerMessage.IsEmpty() && m_uiCollisionLayerEndPoint != m_uiCollisionLayerTrigger)
  {
    xiiPhysicsCastResult      triggerHit;
    xiiPhysicsQueryParameters queryParams2(m_uiCollisionLayerTrigger);
    queryParams2.m_bIgnoreInitialOverlap = true;
    queryParams2.m_ShapeTypes            = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(triggerHit, rayStartPosition, rayDir, fHitDistance, queryParams2) && triggerHit.m_fDistance < fHitDistance)
    {
      // We have a hit, check the objects
      if (m_hLastTriggerObjectInRay != triggerHit.m_hActorObject)
      {
        // If we had another object, we now have one closer - send
        // deactivated for the old object and activate the new one
        if (!m_hLastTriggerObjectInRay.IsInvalidated())
        {
          PostTriggerMessage(xiiTriggerState::Deactivated, m_hLastTriggerObjectInRay);
        }

        // Activate the new hit
        m_hLastTriggerObjectInRay = triggerHit.m_hActorObject;
        PostTriggerMessage(xiiTriggerState::Activated, m_hLastTriggerObjectInRay);
      }
      // If it is still the same object as before we send a continuing message
      else
      {
        PostTriggerMessage(xiiTriggerState::Continuing, m_hLastTriggerObjectInRay);
      }
    }
    else
    {
      // No hit anymore?
      if (!m_hLastTriggerObjectInRay.IsInvalidated())
      {
        PostTriggerMessage(xiiTriggerState::Deactivated, m_hLastTriggerObjectInRay);
      }

      m_hLastTriggerObjectInRay.Invalidate();
    }
  }

  if (m_bForceTargetParentless)
  {
    // this is necessary to ensure perfect positioning when the target is originally attached to a moving object
    // that happens, for instance, when the target is part of a prefab, which includes the raycast component, of course
    // and the prefab is then attached to e.g. a character
    // without detaching the target object from all parents, it is not possible to ensure that it will never deviate from the
    // position set by the raycast component
    // since we now change ownership (target is not deleted with its former parent anymore)
    // this flag also means that the raycast component will delete the target object, when it dies
    pEndObject->SetParent(xiiGameObjectHandle());
  }

  pEndObject->SetGlobalPosition(rayStartPosition + fHitDistance * rayDir);
}

void xiiRaycastComponent::PostTriggerMessage(xiiTriggerState::Enum state, xiiGameObjectHandle hObject)
{
  xiiMsgTriggerTriggered msg;

  msg.m_TriggerState      = state;
  msg.m_sMessage          = m_sTriggerMessage;
  msg.m_hTriggeringObject = hObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), xiiTime::MakeZero(), xiiObjectMsgQueueType::PostTransform);
}
