#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/PowerConnectorComponent.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiEventMsgSetPowerInput);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventMsgSetPowerInput, 1, xiiRTTIDefaultAllocator<xiiEventMsgSetPowerInput>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PrevValue", m_uiPrevValue),
    XII_MEMBER_PROPERTY("NewValue", m_uiNewValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPowerConnectorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Output", GetOutput, SetOutput),
    XII_ACCESSOR_PROPERTY("Buddy", DummyGetter, SetBuddyReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("ConnectedTo", DummyGetter, SetConnectedToReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSensorDetectedObjectsChanged, OnMsgSensorDetectedObjectsChanged),
    XII_MESSAGE_HANDLER(xiiMsgObjectGrabbed, OnMsgObjectGrabbed),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(IsConnected),
    XII_SCRIPT_FUNCTION_PROPERTY(IsAttached),
    XII_SCRIPT_FUNCTION_PROPERTY(Detach),
    // XII_SCRIPT_FUNCTION_PROPERTY(Attach, In, "Object"), // not supported (yet)
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiPowerConnectorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hBuddy);
  inout_stream.WriteGameObjectHandle(m_hConnectedTo);

  s << m_uiOutput;
}

void xiiPowerConnectorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  m_hBuddy       = inout_stream.ReadGameObjectHandle();
  m_hConnectedTo = inout_stream.ReadGameObjectHandle();

  s >> m_uiOutput;
}

void xiiPowerConnectorComponent::ConnectToSocket(xiiGameObjectHandle hSocket)
{
  if (IsConnected())
    return;

  if (GetOwner()->GetWorld()->GetClock().GetAccumulatedTime() - m_DetachTime < xiiTime::MakeFromSeconds(1))
  {
    // recently detached -> wait a bit before allowing to attach again
    return;
  }

  Attach(hSocket);
}

void xiiPowerConnectorComponent::SetOutput(xiiUInt16 value)
{
  if (m_uiOutput == value)
    return;

  m_uiOutput = value;

  OutputChanged(m_uiOutput);
}

void xiiPowerConnectorComponent::SetInput(xiiUInt16 value)
{
  if (m_uiInput == value)
    return;

  InputChanged(m_uiInput, value);
  m_uiInput = value;
}

void xiiPowerConnectorComponent::SetBuddyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetBuddy(resolver(szReference, GetHandle(), "Buddy"));
}

void xiiPowerConnectorComponent::SetBuddy(xiiGameObjectHandle hNewBuddy)
{
  if (m_hBuddy == hNewBuddy)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hBuddy = hNewBuddy;
    return;
  }

  xiiGameObjectHandle hPrevBuddy = m_hBuddy;
  m_hBuddy                       = {};

  xiiGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevBuddy, pBuddy))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(0);
      pConnector->SetBuddy({});
    }
  }

  m_hBuddy = hNewBuddy;

  if (GetOwner()->GetWorld()->TryGetObject(hNewBuddy, pBuddy))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetBuddy(GetOwner()->GetHandle());
      pConnector->SetOutput(m_uiInput);
    }
  }
}

void xiiPowerConnectorComponent::SetConnectedToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetConnectedTo(resolver(szReference, GetHandle(), "ConnectedTo"));
}

void xiiPowerConnectorComponent::SetConnectedTo(xiiGameObjectHandle hNewConnectedTo)
{
  if (m_hConnectedTo == hNewConnectedTo)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hConnectedTo = hNewConnectedTo;
    return;
  }

  xiiGameObjectHandle hPrevConnectedTo = m_hConnectedTo;
  m_hConnectedTo                       = {};

  xiiGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevConnectedTo, pConnectedTo))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(0);
      pConnector->SetConnectedTo({});
    }
  }

  m_hConnectedTo = hNewConnectedTo;

  if (GetOwner()->GetWorld()->TryGetObject(hNewConnectedTo, pConnectedTo))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetConnectedTo(GetOwner()->GetHandle());
      pConnector->SetInput(m_uiOutput);
    }
  }

  if (hNewConnectedTo.IsInvalidated() && IsAttached())
  {
    // make sure that if we get disconnected, we also clean up our detachment state
    Detach();
  }
}

bool xiiPowerConnectorComponent::IsConnected() const
{
  // since connectors automatically disconnect themselves from their peers upon destruction, this should be sufficient (no need to check object for existence)
  return !m_hConnectedTo.IsInvalidated();
}

bool xiiPowerConnectorComponent::IsAttached() const
{
  return !m_hAttachPoint.IsInvalidated();
}

void xiiPowerConnectorComponent::OnDeactivated()
{
  Detach();
  SetBuddy({});

  SUPER::OnDeactivated();
}

void xiiPowerConnectorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiGameObjectHandle hAlreadyConnectedTo = m_hConnectedTo;
  m_hConnectedTo.Invalidate();

  if (!hAlreadyConnectedTo.IsInvalidated())
  {
    Attach(hAlreadyConnectedTo);
  }

  if (m_uiInput != 0)
  {
    InputChanged(0, m_uiInput);
  }

  if (m_uiOutput != 0)
  {
    OutputChanged(m_uiOutput);
  }
}

void xiiPowerConnectorComponent::OnMsgSensorDetectedObjectsChanged(xiiMsgSensorDetectedObjectsChanged& msg)
{
  if (!msg.m_DetectedObjects.IsEmpty())
  {
    ConnectToSocket(msg.m_DetectedObjects[0]);
  }
}

void xiiPowerConnectorComponent::OnMsgObjectGrabbed(xiiMsgObjectGrabbed& msg)
{
  if (msg.m_bGotGrabbed)
  {
    Detach();

    m_hGrabbedBy = msg.m_hGrabbedBy;

    if (xiiGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(true);
    }
  }
  else
  {
    m_hGrabbedBy.Invalidate();

    if (xiiGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(false);
    }
  }
}

void xiiPowerConnectorComponent::Attach(xiiGameObjectHandle hSocket)
{
  xiiWorld* pWorld = GetOwner()->GetWorld();

  xiiGameObject* pSocket;
  if (!pWorld->TryGetObject(hSocket, pSocket))
    return;

  xiiPowerConnectorComponent* pConnector;
  if (pSocket->TryGetComponentOfBaseType(pConnector))
  {
    // don't connect to an already connected object
    if (pConnector->IsConnected())
      return;
  }

  const xiiTransform tSocket = pSocket->GetGlobalTransform();

  xiiGameObjectDesc go;
  go.m_hParent = hSocket;

  xiiGameObject* pAttach;
  m_hAttachPoint = pWorld->CreateObject(go, pAttach);

  xiiPhysicsWorldModuleInterface* pPhysicsWorldModule = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();

  xiiPhysicsWorldModuleInterface::FixedJointConfig cfg;
  cfg.m_hActorA     = {};
  cfg.m_hActorB     = GetOwner()->GetHandle();
  cfg.m_LocalFrameA = tSocket;
  pPhysicsWorldModule->AddFixedJointComponent(pAttach, cfg);

  SetConnectedTo(hSocket);

  if (!m_hGrabbedBy.IsInvalidated())
  {
    xiiGameObject* pGrab;
    if (pWorld->TryGetObject(m_hGrabbedBy, pGrab))
    {
      xiiMsgReleaseObjectGrab msg;
      msg.m_hGrabbedObjectToRelease = GetOwner()->GetHandle();
      pGrab->SendMessage(msg);
    }
  }
}

void xiiPowerConnectorComponent::Detach()
{
  if (IsConnected())
  {
    m_DetachTime = GetOwner()->GetWorld()->GetClock().GetAccumulatedTime();

    SetConnectedTo({});
  }

  if (!m_hAttachPoint.IsInvalidated())
  {
    GetOwner()->GetWorld()->DeleteObjectDelayed(m_hAttachPoint, false);
    m_hAttachPoint.Invalidate();
  }
}

void xiiPowerConnectorComponent::InputChanged(xiiUInt16 uiPrevInput, xiiUInt16 uiInput)
{
  if (!IsActiveAndSimulating())
    return;

  xiiEventMsgSetPowerInput msg;
  msg.m_uiPrevValue = uiPrevInput;
  msg.m_uiNewValue  = uiInput;

  GetOwner()->PostEventMessage(msg, this, xiiTime());

  if (m_hBuddy.IsInvalidated())
    return;

  xiiGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(m_hBuddy, pBuddy))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(uiInput);
    }
  }
}

void xiiPowerConnectorComponent::OutputChanged(xiiUInt16 uiOutput)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_hConnectedTo.IsInvalidated())
    return;

  xiiGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(m_hConnectedTo, pConnectedTo))
  {
    xiiPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(uiOutput);
    }
  }
}
