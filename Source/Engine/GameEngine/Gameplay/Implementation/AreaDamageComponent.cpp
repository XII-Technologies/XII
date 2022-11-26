#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/AreaDamageComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAreaDamageComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OnCreation", m_bTriggerOnCreation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("Damage", m_fDamage)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_MEMBER_PROPERTY("Impulse", m_fImpulse)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(ApplyAreaDamage),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiSphereVisualizerAttribute("Radius", xiiColor::OrangeRed),
    new xiiSphereManipulatorAttribute("Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiPhysicsOverlapResultArray g_OverlapResults;

xiiAreaDamageComponent::xiiAreaDamageComponent()  = default;
xiiAreaDamageComponent::~xiiAreaDamageComponent() = default;

void xiiAreaDamageComponent::ApplyAreaDamage()
{
  if (!IsActiveAndSimulating())
    return;

  XII_PROFILE_SCOPE("ApplyAreaDamage");

  xiiPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const xiiVec3 vOwnPosition = GetOwner()->GetGlobalPosition();

  xiiPhysicsQueryParameters query(m_uiCollisionLayer);
  query.m_ShapeTypes.Remove(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Trigger);

  pPhysicsInterface->QueryShapesInSphere(g_OverlapResults, m_fRadius, vOwnPosition, query);

  const float fInvRadius = 1.0f / m_fRadius;

  for (const auto& hit : g_OverlapResults.m_Results)
  {
    if (!hit.m_hActorObject.IsInvalidated())
    {
      xiiGameObject* pObject = nullptr;
      if (GetWorld()->TryGetObject(hit.m_hActorObject, pObject))
      {
        const xiiVec3 vTargetPos    = pObject->GetGlobalPosition();
        const xiiVec3 vDistToTarget = vTargetPos - vOwnPosition;
        xiiVec3       vDirToTarget  = vDistToTarget;
        const float   fDistance     = vDirToTarget.GetLength();

        if (fDistance >= 0.01f)
        {
          // if the direction is valid (non-zero), just normalize it
          vDirToTarget /= fDistance;
        }
        else
        {
          // otherwise, if we are so close, that the distance is zero, pick a random direction away from it
          vDirToTarget.CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
        }

        // linearly scale damage and impulse down by distance
        const float fScale = 1.0f - xiiMath::Min(fDistance * fInvRadius, 1.0f);

        // apply a physical impulse
        if (m_fImpulse != 0.0f)
        {
          xiiMsgPhysicsAddImpulse msg;
          msg.m_vGlobalPosition  = vTargetPos;
          msg.m_vImpulse         = vDirToTarget * m_fImpulse * fScale;
          msg.m_uiObjectFilterID = hit.m_uiObjectFilterID;

          pObject->SendMessage(msg);
        }

        // apply damage
        if (m_fDamage != 0.0f)
        {
          xiiMsgDamage msg;
          msg.m_fDamage          = static_cast<double>(m_fDamage) * static_cast<double>(fScale);
          msg.m_vImpactDirection = vDirToTarget;
          msg.m_vGlobalPosition  = vOwnPosition + vDistToTarget * 0.9f; // rough guess for a position where to apply the damage

          xiiGameObject* pShape = nullptr;
          if (GetWorld()->TryGetObject(hit.m_hShapeObject, pShape))
          {
            msg.m_sHitObjectName = pShape->GetName();
          }
          else
          {
            msg.m_sHitObjectName = pObject->GetName();
          }

          // delay the damage a little bit for nicer chain reactions
          pObject->PostEventMessage(msg, this, xiiTime::Milliseconds(200));
        }
      }
    }
  }
}

void xiiAreaDamageComponent::OnSimulationStarted()
{
  if (m_bTriggerOnCreation)
  {
    ApplyAreaDamage();
  }
}

void xiiAreaDamageComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_bTriggerOnCreation;
  s << m_fRadius;
  s << m_uiCollisionLayer;
  s << m_fDamage;
  s << m_fImpulse;
}

void xiiAreaDamageComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_bTriggerOnCreation;
  s >> m_fRadius;
  s >> m_uiCollisionLayer;
  s >> m_fDamage;
  s >> m_fImpulse;
}

//////////////////////////////////////////////////////////////////////////

xiiAreaDamageComponentManager::xiiAreaDamageComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld), m_pPhysicsInterface(nullptr)
{
}

void xiiAreaDamageComponentManager::Initialize()
{
  SUPER::Initialize();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_AreaDamageComponent);
