#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/FakeRopeComponent.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiFakeRopeComponent, 3, xiiComponentMode::Static)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("Anchor1", DummyGetter, SetAnchor1Reference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
      XII_ACCESSOR_PROPERTY("Anchor2", DummyGetter, SetAnchor2Reference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
      XII_ACCESSOR_PROPERTY("AttachToAnchor1", GetAttachToAnchor1, SetAttachToAnchor1)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_ACCESSOR_PROPERTY("AttachToAnchor2", GetAttachToAnchor2, SetAttachToAnchor2)->AddAttributes(new xiiDefaultValueAttribute(true)),
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

void xiiFakeRopeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiPieces;
  s << m_fSlack;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  inout_stream.WriteGameObjectHandle(m_hAnchor1);
  inout_stream.WriteGameObjectHandle(m_hAnchor2);

  s << m_fWindInfluence;
}

void xiiFakeRopeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_uiPieces;
  s >> m_fSlack;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  if (uiVersion >= 3)
  {
    m_hAnchor1 = inout_stream.ReadGameObjectHandle();
  }

  m_hAnchor2 = inout_stream.ReadGameObjectHandle();

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

  xiiGameObjectHandle hAnchor1 = m_hAnchor1;
  xiiGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return XII_FAILURE;

  xiiSimdVec4f anchor1;
  xiiSimdVec4f anchor2;

  xiiGameObject* pAnchor1 = nullptr;
  xiiGameObject* pAnchor2 = nullptr;

  if (!GetWorld()->TryGetObject(hAnchor1, pAnchor1))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return XII_FAILURE;

    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      anchor1                       = m_RopeSim.m_Nodes[0].m_vPosition;
      m_RopeSim.m_bFirstNodeIsFixed = false;
      m_uiSleepCounter              = 0;
    }
  }
  else
  {
    anchor1 = xiiSimdConversion::ToVec3(pAnchor1->GetGlobalPosition());
  }

  if (!GetWorld()->TryGetObject(hAnchor2, pAnchor2))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return XII_FAILURE;

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      anchor2                      = m_RopeSim.m_Nodes.PeekBack().m_vPosition;
      m_RopeSim.m_bLastNodeIsFixed = false;
      m_uiSleepCounter             = 0;
    }
  }
  else
  {
    anchor2 = xiiSimdConversion::ToVec3(pAnchor2->GetGlobalPosition());
  }

  // only early out, if we are not in edit mode
  m_bIsDynamic = !IsActiveAndSimulating() || (pAnchor1 != nullptr && pAnchor1->IsDynamic()) || (pAnchor2 != nullptr && pAnchor2->IsDynamic());

  m_RopeSim.m_fDampingFactor = xiiMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len            = (anchor1 - anchor2).GetLength<3>();
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
      m_RopeSim.m_Nodes[i].m_vPosition         = anchor1 + ((anchor2 - anchor1) * (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes[0].m_vPosition != anchor1).AnySet<3>())
      {
        m_uiSleepCounter                 = 0;
        m_RopeSim.m_Nodes[0].m_vPosition = anchor1;
      }
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes.PeekBack().m_vPosition != anchor2).AnySet<3>())
      {
        m_uiSleepCounter                         = 0;
        m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchor2;
      }
    }
  }

  return XII_SUCCESS;
}

void xiiFakeRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  xiiGameObject* pAnchor1 = nullptr;
  xiiGameObject* pAnchor2 = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor1, pAnchor1))
    pAnchor1 = GetOwner();
  if (!GetWorld()->TryGetObject(m_hAnchor2, pAnchor2))
    pAnchor2 = GetOwner();

  if (pAnchor1 == pAnchor2)
    return;

  xiiUInt32 uiHash = 0;

  xiiVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash      = xiiHashingUtils::xxHash32(&pos, sizeof(xiiVec3), uiHash);

  pos    = pAnchor1->GetGlobalPosition();
  uiHash = xiiHashingUtils::xxHash32(&pos, sizeof(xiiVec3), uiHash);

  pos    = pAnchor2->GetGlobalPosition();
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

  xiiVisibilityState visType = GetOwner()->GetVisibilityState();

  if (visType == xiiVisibilityState::Invisible)
    return;

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
      tGlobal.m_qRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiSimdConversion::ToVec3(dir));

      pieces[i] = xiiTransform::MakeLocalTransform(tRoot, tGlobal);
    }

    {
      tGlobal.m_vPosition = xiiSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition);
      // tGlobal.m_qRotation is the same as from the previous bone

      pieces.PeekBack() = xiiTransform::MakeLocalTransform(tRoot, tGlobal);
    }

    poseMsg.m_LinkTransforms = pieces;
  }

  GetOwner()->PostMessage(poseMsg, xiiTime::MakeZero(), xiiObjectMsgQueueType::AfterInitialized);
}

void xiiFakeRopeComponent::SetAnchor1Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor1(resolver(szReference, GetHandle(), "Anchor1"));
}

void xiiFakeRopeComponent::SetAnchor2Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor2(resolver(szReference, GetHandle(), "Anchor2"));
}

void xiiFakeRopeComponent::SetAnchor1(xiiGameObjectHandle hActor)
{
  m_hAnchor1       = hActor;
  m_bIsDynamic     = true;
  m_uiSleepCounter = 0;
}

void xiiFakeRopeComponent::SetAnchor2(xiiGameObjectHandle hActor)
{
  m_hAnchor2       = hActor;
  m_bIsDynamic     = true;
  m_uiSleepCounter = 0;
}

void xiiFakeRopeComponent::SetSlack(float fVal)
{
  m_fSlack                   = fVal;
  m_RopeSim.m_fSegmentLength = -1.0f;
  m_bIsDynamic               = true;
  m_uiSleepCounter           = 0;
}

void xiiFakeRopeComponent::SetAttachToAnchor1(bool bVal)
{
  m_RopeSim.m_bFirstNodeIsFixed = bVal;
  m_bIsDynamic                  = true;
  m_uiSleepCounter              = 0;
}

void xiiFakeRopeComponent::SetAttachToAnchor2(bool bVal)
{
  m_RopeSim.m_bLastNodeIsFixed = bVal;
  m_bIsDynamic                 = true;
  m_uiSleepCounter             = 0;
}

bool xiiFakeRopeComponent::GetAttachToAnchor1() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

bool xiiFakeRopeComponent::GetAttachToAnchor2() const
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
