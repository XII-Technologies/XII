/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltActorComponent, 3)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiDynamicEnumAttribute("JoltCollisionLayer")),
    XII_ACCESSOR_PROPERTY("ReportContacts", GetReportContacts, SetReportContacts)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_RESOURCE_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
  }
  XII_END_PROPERTIES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltStaticActorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltStaticActorComponent::xiiJoltStaticActorComponent()  = default;
xiiJoltStaticActorComponent::~xiiJoltStaticActorComponent() = default;

void xiiJoltActorComponent::SetCollisionLayer(xiiUInt8 uiCollisionLayer)
{
  if (m_uiCollisionLayer == uiCollisionLayer)
    return;

  m_uiCollisionLayer = uiCollisionLayer;
  QueueBodyUpdate();
}

void xiiJoltActorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_uiCollisionLayer << m_bReportContacts;
  inout_stream.GetStream() << m_hSurface;
}

void xiiJoltActorComponent::SetSurface(const xiiSurfaceResourceHandle& hSurface)
{
  if (m_hSurface == hSurface)
    return;
  m_hSurface = hSurface;
  if (m_hSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hSurface);
  if (IsActiveAndSimulating())
    GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->SetBodySurface(GetHandle(), m_hSurface);
}

void xiiJoltActorComponent::SetReportContacts(bool bReportContacts)
{
  if (m_bReportContacts == bReportContacts)
    return;
  m_bReportContacts = bReportContacts;
  if (IsActiveAndSimulating())
  {
    if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
      pModule->SetBodyContactReporting(GetHandle(), bReportContacts);
  }
}

void xiiJoltActorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_uiCollisionLayer;
  if (inout_stream.GetComponentTypeVersion(xiiJoltActorComponent::GetStaticRTTI()) >= 2)
    inout_stream.GetStream() >> m_bReportContacts;
  if (inout_stream.GetComponentTypeVersion(xiiJoltActorComponent::GetStaticRTTI()) >= 3)
    inout_stream.GetStream() >> m_hSurface;
}

void xiiJoltActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->QueueBodyUpdate(GetHandle());
  QueueSiblingBodyUpdates();
  QueueAncestorBodyUpdate(GetOwner()->GetParent());
}

void xiiJoltActorComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
  {
    pModule->RemoveBody(GetHandle());
    QueueSiblingBodyUpdates();
    QueueAncestorBodyUpdate(GetOwner()->GetParent());
  }
  SUPER::OnDeactivated();
}

void xiiJoltActorComponent::QueueBodyUpdate()
{
  if (IsActiveAndSimulating())
  {
    if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    {
      pModule->QueueBodyUpdate(GetHandle());
    }
  }
}

void xiiJoltActorComponent::QueueAncestorBodyUpdate(xiiGameObject* pObject)
{
  for (; pObject != nullptr; pObject = pObject->GetParent())
  {
    bool bHasActor = false;
    for (auto* pComponent : pObject->GetComponents())
    {
      if (auto* pActor = xiiDynamicCast<xiiJoltActorComponent*>(pComponent); pActor != nullptr && pActor->IsActiveAndSimulating())
      {
        pActor->QueueBodyUpdate();
        bHasActor = true;
      }
    }
    if (bHasActor)
      return;
  }
}

void xiiJoltActorComponent::QueueSiblingBodyUpdates()
{
  for (auto* pComponent : GetOwner()->GetComponents())
  {
    if (auto* pActor = xiiDynamicCast<xiiJoltActorComponent*>(pComponent); pActor != nullptr && pActor != this)
    {
      pActor->QueueBodyUpdate();
    }
  }
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltActorComponent);
