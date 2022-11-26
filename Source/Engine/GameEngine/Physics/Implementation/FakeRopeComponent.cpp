#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiFakeRopeComponent, 2, xiiComponentMode::Static)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
      XII_ACCESSOR_PROPERTY("AttachToOrigin", GetAttachToOrigin, SetAttachToOrigin)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_ACCESSOR_PROPERTY("AttachToAnchor", GetAttachToAnchor, SetAttachToAnchor)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new xiiDefaultValueAttribute(32), new xiiClampValueAttribute(2, 200)),
      XII_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new xiiDefaultValueAttribute(0.2f)),
      XII_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
      XII_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 10.0f)),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Effects/Ropes"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiFakeRopeComponent::xiiFakeRopeComponent()  = default;
xiiFakeRopeComponent::~xiiFakeRopeComponent() = default;

void xiiFakeRopeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiPieces;
  s << m_fSlack;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  stream.WriteGameObjectHandle(m_hAnchor);

  s << m_fWindInfluence;
}

void xiiFakeRopeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = stream.GetStream();

  s >> m_uiPieces;
  s >> m_fSlack;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  m_hAnchor = stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_fWindInfluence;
  }
}

void xiiFakeRopeComponent::OnActivated()
{
  m_uiPreviewHash = 0;
  m_RopeSim.m_Nodes.Clear();
  m_RopeSim.m_fSegmentLength = -1.0f;

  m_uiCheckEquilibriumCounter = GetOwner()->GetStableRandomSeed() & 63;

  SendPreviewPose();
}

void xiiFakeRopeComponent::OnDeactivated()
{
  // tell the render components, that the rope is gone
  m_RopeSim.m_Nodes.Clear();
  SendCurrentPose();

  SUPER::OnDeactivated();
}

xiiResult xiiFakeRopeComponent::ConfigureRopeSimulator()
{
  if (!m_bIsDynamic)
    return XII_SUCCESS;

  if (!IsActiveAndInitialized())
    return XII_FAILURE;

  xiiSimdVec4f anchorB;

  xiiGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return XII_FAILURE;

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      anchorB                      = m_RopeSim.m_Nodes.PeekBack().m_vPosition;
      m_RopeSim.m_bLastNodeIsFixed = false;
      m_uiSleepCounter             = 0;
    }
  }
  else
  {
    anchorB = xiiSimdConversion::ToVec3(pAnchor->GetGlobalPosition());
  }

  // only early out, if we are not in edit mode
  m_bIsDynamic = !IsActiveAndSimulating() || GetOwner()->IsDynamic() || (pAnchor != nullptr && pAnchor->IsDynamic());

  const xiiSimdVec4f anchorA = xiiSimdConversion::ToVec3(GetOwner()->GetGlobalPosition());

  m_RopeSim.m_fDampingFactor = xiiMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len            = (anchorA - anchorB).GetLength<3>();
    m_RopeSim.m_fSegmentLength = (len + len * m_fSlack) / m_uiPieces;
  }

  if (const xiiPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    if (m_RopeSim.m_vAcceleration != pModule->GetGravity())
    {
      m_uiSleepCounter          = 0;
      m_RopeSim.m_vAcceleration = pModule->GetGravity();
    }
  }

  if (m_uiPieces < m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter = 0;
    m_RopeSim.m_Nodes.SetCount(m_uiPieces);
  }
  else if (m_uiPieces > m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter         = 0;
    const xiiUInt32 uiOldNum = m_RopeSim.m_Nodes.GetCount();

    m_RopeSim.m_Nodes.SetCount(m_uiPieces);

    for (xiiUInt32 i = uiOldNum; i < m_uiPieces; ++i)
    {
      m_RopeSim.m_Nodes[i].m_vPosition         = anchorA + ((anchorB - anchorA) * (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes[0].m_vPosition != anchorA).AnySet<3>())
      {
        m_uiSleepCounter                 = 0;
        m_RopeSim.m_Nodes[0].m_vPosition = anchorA;
      }
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes.PeekBack().m_vPosition != anchorB).AnySet<3>())
      {
        m_uiSleepCounter                         = 0;
        m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchorB;
      }
    }
  }

  return XII_SUCCESS;
}

void xiiFakeRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  xiiUInt32 uiHash = 0;

  xiiGameObject* pAnchor;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return;

  xiiVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash      = xiiHashingUtils::xxHash32(&pos, sizeof(xiiVec3), uiHash);

  pos    = pAnchor->GetGlobalPosition();
  uiHash = xiiHashingUtils::xxHash32(&pos, sizeof(xiiVec3), uiHash);

  uiHash = xiiHashingUtils::xxHash32(&m_fSlack, sizeof(float), uiHash);
  uiHash = xiiHashingUtils::xxHash32(&m_fDamping, sizeof(float), uiHash);
  uiHash = xiiHashingUtils::xxHash32(&m_uiPieces, sizeof(xiiUInt16), uiHash);

  if (uiHash == m_uiPreviewHash)
    return;

  m_uiPreviewHash            = uiHash;
  m_RopeSim.m_fSegmentLength = -1.0f;

  if (ConfigureRopeSimulator().Failed())
    return;

  m_RopeSim.SimulateTillEquilibrium(0.003f, 100);

  SendCurrentPose();
}

void xiiFakeRopeComponent::RuntimeUpdate()
{
  if (ConfigureRopeSimulator().Failed())
    return;

  xiiVec3 acc(0);

  if (const xiiPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    acc += pModule->GetGravity();
  }
  else
  {
    acc += xiiVec3(0, 0, -9.81f);
  }

  if (m_fWindInfluence > 0.0f)
  {
    if (const xiiWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<xiiWindWorldModuleInterface>())
    {
      const xiiSimdVec4f ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

      xiiVec3 vWind = pWind->GetWindAt(xiiSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition));
      vWind += pWind->GetWindAt(xiiSimdConversion::ToVec3(m_RopeSim.m_Nodes[0].m_vPosition));
      vWind *= 0.5f * m_fWindInfluence;

      acc += vWind;
      acc += pWind->ComputeWindFlutter(vWind, xiiSimdConversion::ToVec3(ropeDir), 0.5f, GetOwner()->GetStableRandomSeed());
    }
  }

  if (m_RopeSim.m_vAcceleration != acc)
  {
    m_RopeSim.m_vAcceleration = acc;
    m_uiSleepCounter          = 0;
  }

  if (m_uiSleepCounter > 10)
    return;

  xiiUInt64 uiFramesVisible = GetOwner()->GetNumFramesSinceVisible();
  if (uiFramesVisible > 60)
  {
    return;
  }


  m_RopeSim.SimulateRope(GetWorld()->GetClock().GetTimeDiff());

  ++m_uiCheckEquilibriumCounter;
  if (m_uiCheckEquilibriumCounter > 64)
  {
    m_uiCheckEquilibriumCounter = 0;

    if (m_RopeSim.HasEquilibrium(0.01f))
    {
      ++m_uiSleepCounter;
    }
    else
    {
      m_uiSleepCounter = 0;
    }
  }

  SendCurrentPose();
}

void xiiFakeRopeComponent::SendCurrentPose()
{
  xiiMsgRopePoseUpdated poseMsg;

  xiiDynamicArray<xiiTransform> pieces(xiiFrameAllocator::GetCurrentAllocator());

  if (m_RopeSim.m_Nodes.GetCount() >= 2)
  {
    const xiiTransform tRoot = GetOwner()->GetGlobalTransform();

    pieces.SetCountUninitialized(m_RopeSim.m_Nodes.GetCount());

    xiiTransform tGlobal;
    tGlobal.m_vScale.Set(1);

    for (xiiUInt32 i = 0; i < pieces.GetCount() - 1; ++i)
    {
      const xiiSimdVec4f p0  = m_RopeSim.m_Nodes[i].m_vPosition;
      const xiiSimdVec4f p1  = m_RopeSim.m_Nodes[i + 1].m_vPosition;
      xiiSimdVec4f       dir = p1 - p0;

      dir.NormalizeIfNotZero<3>();

      tGlobal.m_vPosition = xiiSimdConversion::ToVec3(p0);
      tGlobal.m_qRotation.SetShortestRotation(xiiVec3::UnitXAxis(), xiiSimdConversion::ToVec3(dir));

      pieces[i].SetLocalTransform(tRoot, tGlobal);
    }

    {
      tGlobal.m_vPosition = xiiSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition);
      // tGlobal.m_qRotation is the same as from the previous bone

      pieces.PeekBack().SetLocalTransform(tRoot, tGlobal);
    }


    poseMsg.m_LinkTransforms = pieces;
  }

  GetOwner()->PostMessage(poseMsg, xiiTime::Zero(), xiiObjectMsgQueueType::AfterInitialized);
}

void xiiFakeRopeComponent::SetAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor(resolver(szReference, GetHandle(), "Anchor"));
}

void xiiFakeRopeComponent::SetAnchor(xiiGameObjectHandle hActor)
{
  m_hAnchor        = hActor;
  m_bIsDynamic     = true;
  m_uiSleepCounter = 0;
}

void xiiFakeRopeComponent::SetSlack(float val)
{
  m_fSlack                   = val;
  m_RopeSim.m_fSegmentLength = -1.0f;
  m_bIsDynamic               = true;
  m_uiSleepCounter           = 0;
}

void xiiFakeRopeComponent::SetAttachToOrigin(bool val)
{
  m_RopeSim.m_bFirstNodeIsFixed = val;
  m_bIsDynamic                  = true;
  m_uiSleepCounter              = 0;
}

bool xiiFakeRopeComponent::GetAttachToOrigin() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

void xiiFakeRopeComponent::SetAttachToAnchor(bool val)
{
  m_RopeSim.m_bLastNodeIsFixed = val;
  m_bIsDynamic                 = true;
  m_uiSleepCounter             = 0;
}

bool xiiFakeRopeComponent::GetAttachToAnchor() const
{
  return m_RopeSim.m_bLastNodeIsFixed;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiFakeRopeComponentManager::xiiFakeRopeComponentManager(xiiWorld* pWorld) :
  xiiComponentManager(pWorld)
{
}

xiiFakeRopeComponentManager::~xiiFakeRopeComponentManager() = default;

void xiiFakeRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiFakeRopeComponentManager::Update, this);
    desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void xiiFakeRopeComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->SendPreviewPose();
      }
    }

    return;
  }

  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->RuntimeUpdate();
      }
    }
  }
}
