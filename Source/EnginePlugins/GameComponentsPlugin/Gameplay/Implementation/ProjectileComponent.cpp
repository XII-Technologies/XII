#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProjectileReaction, 2)
  XII_ENUM_CONSTANT(xiiProjectileReaction::Absorb),
  XII_ENUM_CONSTANT(xiiProjectileReaction::Reflect),
  XII_ENUM_CONSTANT(xiiProjectileReaction::Bounce),
  XII_ENUM_CONSTANT(xiiProjectileReaction::Attach),
  XII_ENUM_CONSTANT(xiiProjectileReaction::PassThrough)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiProjectileSurfaceInteraction, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiProjectileSurfaceInteraction>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_ENUM_MEMBER_PROPERTY("Reaction", xiiProjectileReaction, m_Reaction),
    XII_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new xiiDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    XII_MEMBER_PROPERTY("Impulse", m_fImpulse),
    XII_MEMBER_PROPERTY("Damage", m_fDamage),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiProjectileComponent, 6, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    XII_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant())),
    XII_MEMBER_PROPERTY("SpawnPrefabOnStatic", m_bSpawnPrefabOnStatic),
    XII_ACCESSOR_PROPERTY("OnDeathPrefab", GetDeathPrefab, SetDeathPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", xiiPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new xiiDefaultValueAttribute(xiiVariant(xiiPhysicsShapeType::Default & ~(xiiPhysicsShapeType::Trigger)))),
    XII_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.4f, xiiColor::OrangeRed),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiProjectileSurfaceInteraction::SetSurface(const char* szSurface)
{
  xiiSurfaceResourceHandle hSurface;

  if (!xiiStringUtils::IsNullOrEmpty(szSurface))
  {
    hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szSurface);
  }

  m_hSurface = hSurface;
}

const char* xiiProjectileSurfaceInteraction::GetSurface() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

xiiProjectileComponent::xiiProjectileComponent()
{
  m_fMetersPerSecond   = 10.0f;
  m_uiCollisionLayer   = 0;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
  m_bSpawnPrefabOnStatic = false;
}

xiiProjectileComponent::~xiiProjectileComponent() = default;

void xiiProjectileComponent::Update()
{
  xiiPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<xiiPhysicsWorldModuleInterface>();

  if (pPhysicsInterface)
  {
    xiiGameObject* pEntity = GetOwner();

    const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    xiiVec3 vNewPosition;

    // gravity
    if (m_fGravityMultiplier != 0.0f && m_fMetersPerSecond > 0.0f) // mps == 0 for attached state
    {
      const xiiVec3 vGravity = pPhysicsInterface->GetGravity() * m_fGravityMultiplier;

      m_vVelocity += vGravity * fTimeDiff;
    }

    xiiVec3 vCurDirection = m_vVelocity * fTimeDiff;
    float   fDistance     = 0.0f;

    if (!vCurDirection.IsZero())
      fDistance = vCurDirection.GetLengthAndNormalize();

    xiiPhysicsQueryParameters queryParams(m_uiCollisionLayer);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes            = m_ShapeTypesToHit;

    xiiPhysicsCastResult castResult;
    if (pPhysicsInterface->Raycast(castResult, pEntity->GetGlobalPosition(), vCurDirection, fDistance, queryParams))
    {
      const xiiSurfaceResourceHandle hSurface = castResult.m_hSurface.IsValid() ? castResult.m_hSurface : m_hFallbackSurface;

      const xiiInt32 iInteraction = FindSurfaceInteraction(hSurface);

      if (iInteraction == -1)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = castResult.m_vPosition;
      }
      else
      {
        const auto& interaction = m_SurfaceInteractions[iInteraction];

        if (!interaction.m_sInteraction.IsEmpty())
        {
          TriggerSurfaceInteraction(hSurface, castResult.m_hActorObject, castResult.m_vPosition, castResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
        }

        // if we hit some valid object
        if (!castResult.m_hActorObject.IsInvalidated())
        {
          xiiGameObject* pObject = nullptr;

          // apply a physical impulse
          if (interaction.m_fImpulse > 0.0f)
          {
            if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
            {
              xiiMsgPhysicsAddImpulse msg;
              msg.m_vGlobalPosition       = castResult.m_vPosition;
              msg.m_vImpulse              = vCurDirection * interaction.m_fImpulse;
              msg.m_uiObjectFilterID      = castResult.m_uiObjectFilterID;
              msg.m_pInternalPhysicsShape = castResult.m_pInternalPhysicsShape;
              msg.m_pInternalPhysicsActor = castResult.m_pInternalPhysicsActor;

              pObject->SendMessage(msg);
            }
          }

          // apply damage
          if (interaction.m_fDamage > 0.0f)
          {
            // skip the TryGetObject if we already did that above
            if (pObject != nullptr || GetWorld()->TryGetObject(castResult.m_hShapeObject, pObject))
            {
              xiiMsgDamage msg;
              msg.m_fDamage          = interaction.m_fDamage;
              msg.m_vGlobalPosition  = castResult.m_vPosition;
              msg.m_vImpactDirection = vCurDirection;

              xiiGameObject* pHitShape = nullptr;
              if (GetWorld()->TryGetObject(castResult.m_hShapeObject, pHitShape))
              {
                msg.m_sHitObjectName = pHitShape->GetName();
              }
              else
              {
                msg.m_sHitObjectName = pObject->GetName();
              }

              pObject->SendEventMessage(msg, this);
            }
          }
        }

        if (interaction.m_Reaction == xiiProjectileReaction::Absorb)
        {
          SpawnDeathPrefab();


          GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
          vNewPosition = castResult.m_vPosition;
        }
        else if (interaction.m_Reaction == xiiProjectileReaction::Reflect || interaction.m_Reaction == xiiProjectileReaction::Bounce)
        {
          /// \todo Should reflect around the actual hit position
          /// \todo Should preserve travel distance while reflecting

          // const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition(); // vPos;

          const xiiVec3 vNewDirection = vCurDirection.GetReflectedVector(castResult.m_vNormal);

          xiiQuat qRot = xiiQuat::MakeShortestRotation(vCurDirection, vNewDirection);

          GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());

          m_vVelocity = qRot * m_vVelocity;

          if (interaction.m_Reaction == xiiProjectileReaction::Bounce)
          {
            xiiResourceLock<xiiSurfaceResource> pSurface(hSurface, xiiResourceAcquireMode::BlockTillLoaded);

            if (pSurface)
            {
              m_vVelocity *= pSurface->GetDescriptor().m_fPhysicsRestitution;
            }

            if (m_vVelocity.GetLength() < 1.0f)
            {
              m_vVelocity          = xiiVec3::MakeZero();
              m_fGravityMultiplier = 0.0f;

              if (m_bSpawnPrefabOnStatic)
              {
                SpawnDeathPrefab();
                GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
              }
            }
          }
        }
        else if (interaction.m_Reaction == xiiProjectileReaction::Attach)
        {
          m_fMetersPerSecond = 0.0f;
          vNewPosition       = castResult.m_vPosition;

          xiiGameObject* pObject;
          if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
          {
            pObject->AddChild(GetOwner()->GetHandle(), xiiGameObject::TransformPreservation::PreserveGlobal);
          }
        }
        else if (interaction.m_Reaction == xiiProjectileReaction::PassThrough)
        {
          vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
        }
      }
    }
    else
    {
      vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
    }

    GetOwner()->SetGlobalPosition(vNewPosition);
  }
}

void xiiProjectileComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hDeathPrefab;

  // Version 3
  s << m_hFallbackSurface;

  s << m_SurfaceInteractions.GetCount();
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_hSurface;

    xiiProjectileReaction::StorageType storage = ia.m_Reaction;
    s << storage;

    s << ia.m_sInteraction;

    // Version 3
    s << ia.m_fImpulse;

    // Version 4
    s << ia.m_fDamage;
  }

  // Version 5
  s << m_ShapeTypesToHit;

  // Version 6
  s << m_bSpawnPrefabOnStatic;
}

void xiiProjectileComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hDeathPrefab;

  if (uiVersion >= 3)
  {
    s >> m_hFallbackSurface;
  }

  xiiUInt32 count;
  s >> count;
  m_SurfaceInteractions.SetCount(count);
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    auto& ia = m_SurfaceInteractions[i];
    s >> ia.m_hSurface;

    xiiProjectileReaction::StorageType storage = 0;
    s >> storage;
    ia.m_Reaction = (xiiProjectileReaction::Enum)storage;

    s >> ia.m_sInteraction;

    if (uiVersion >= 3)
    {
      s >> ia.m_fImpulse;
    }

    if (uiVersion >= 4)
    {
      s >> ia.m_fDamage;
    }
  }

  if (uiVersion >= 5)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 6)
  {
    s >> m_bSpawnPrefabOnStatic;
  }
}


xiiInt32 xiiProjectileComponent::FindSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface) const
{
  xiiSurfaceResourceHandle hCurSurf = hSurface;

  while (hCurSurf.IsValid())
  {
    for (xiiUInt32 i = 0; i < m_SurfaceInteractions.GetCount(); ++i)
    {
      if (hCurSurf == m_SurfaceInteractions[i].m_hSurface)
        return i;
    }

    // get parent surface
    {
      xiiResourceLock<xiiSurfaceResource> pSurf(hCurSurf, xiiResourceAcquireMode::BlockTillLoaded);
      hCurSurf = pSurf->GetDescriptor().m_hBaseSurface;
    }
  }

  return -1;
}


void xiiProjectileComponent::TriggerSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface, xiiGameObjectHandle hObject, const xiiVec3& vPos, const xiiVec3& vNormal, const xiiVec3& vDirection, const char* szInteraction)
{
  xiiResourceLock<xiiSurfaceResource> pSurface(hSurface, xiiResourceAcquireMode::BlockTillLoaded);
  pSurface->InteractWithSurface(GetWorld(), hObject, vPos, vNormal, vDirection, xiiTempHashedString(szInteraction), &GetOwner()->GetTeamID());
}

static xiiHashedString s_sSuicide = xiiMakeHashedString("Suicide");

void xiiProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    xiiMsgComponentInternalTrigger msg;
    msg.m_sMessage = s_sSuicide;

    PostMessage(msg, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hDeathPrefab.IsValid())
    {
      xiiResourceManager::PreloadResource(m_hDeathPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void xiiProjectileComponent::SpawnDeathPrefab()
{
  if (!m_bSpawnPrefabOnStatic)
    return;

  if (m_hDeathPrefab.IsValid())
  {
    xiiResourceLock<xiiPrefabResource> pPrefab(m_hDeathPrefab, xiiResourceAcquireMode::AllowLoadingFallback);

    xiiPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options, nullptr);
  }
}

void xiiProjectileComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  SpawnDeathPrefab();

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}


void xiiProjectileComponent::SetDeathPrefab(const char* szPrefab)
{
  xiiPrefabResourceHandle hPrefab;

  if (!xiiStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(szPrefab);
  }

  m_hDeathPrefab = hPrefab;
}

const char* xiiProjectileComponent::GetDeathPrefab() const
{
  if (!m_hDeathPrefab.IsValid())
    return "";

  return m_hDeathPrefab.GetResourceID();
}

void xiiProjectileComponent::SetFallbackSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }
  if (m_hFallbackSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hFallbackSurface);
}

const char* xiiProjectileComponent::GetFallbackSurfaceFile() const
{
  if (!m_hFallbackSurface.IsValid())
    return "";

  return m_hFallbackSurface.GetResourceID();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiProjectileComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiProjectileComponentPatch_1_2() :
    xiiGraphPatch("xiiProjectileComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

class xiiProjectileComponentPatch_5_6 : public xiiGraphPatch
{
public:
  xiiProjectileComponentPatch_5_6() :
    xiiGraphPatch("xiiProjectileComponent", 6)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("TimeoutPrefab", "DeathPrefab");
  }
};

xiiProjectileComponentPatch_1_2 g_xiiProjectileComponentPatch_1_2;
