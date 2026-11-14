/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltStaticCollisionComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltStaticCollisionComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiDynamicEnumAttribute("JoltCollisionLayer")),
    XII_ACCESSOR_PROPERTY("ReportContacts", GetReportContacts, SetReportContacts),
    XII_RESOURCE_ACCESSOR_PROPERTY("CollisionMesh", GetCollisionMesh, SetCollisionMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_JoltCollisionMesh", xiiDependencyFlags::Package)),
    XII_RESOURCE_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgStaticMeshChanged, OnGeometryChanged),
    XII_MESSAGE_HANDLER(xiiMsgChildrenChanged, OnChildrenChanged),
    XII_MESSAGE_HANDLER(xiiMsgComponentsChanged, OnComponentsChanged),
    XII_MESSAGE_HANDLER(xiiMsgParentChanged, OnParentChanged),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiJoltStaticCollisionComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->EnableParentChangesNotifications();
  GetOwner()->EnableStaticTransformChangesNotifications();
  xiiMsgStaticMeshChanged::Notify(*GetOwner());
  InvalidateGeometry();
}

void xiiJoltStaticCollisionComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    pModule->RemoveStaticCollision(GetHandle());
  m_State            = State::Inactive;
  m_bReportedFailure = false;
  xiiMsgStaticMeshChanged::Notify(*GetOwner());
  SUPER::OnDeactivated();
}

void xiiJoltStaticCollisionComponent::InvalidateGeometry()
{
  if (IsActiveAndInitialized())
  {
    m_State = State::PendingGeometry;
    GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->QueueStaticCollision(GetHandle());
  }
}

void xiiJoltStaticCollisionComponent::OnGeometryChanged(xiiMsgStaticMeshChanged&) { InvalidateGeometry(); }
void xiiJoltStaticCollisionComponent::OnChildrenChanged(xiiMsgChildrenChanged& msg)
{
  InvalidateGeometry();
  const xiiGameObject* pChild = nullptr;
  if (static_cast<const xiiWorld*>(GetWorld())->TryGetObject(msg.m_hChild, pChild))
    if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
      pModule->InvalidateStaticCollisionDescendants(*pChild);
}
void xiiJoltStaticCollisionComponent::OnComponentsChanged(xiiMsgComponentsChanged&) { InvalidateGeometry(); }
void xiiJoltStaticCollisionComponent::OnParentChanged(xiiMsgParentChanged&) { InvalidateGeometry(); }

void xiiJoltStaticCollisionComponent::SetCollisionLayer(xiiUInt8 uiLayer)
{
  if (m_uiCollisionLayer != uiLayer)
  {
    m_uiCollisionLayer = uiLayer;
    InvalidateGeometry();
  }
}

void xiiJoltStaticCollisionComponent::SetReportContacts(bool bReport)
{
  if (m_bReportContacts != bReport)
  {
    m_bReportContacts = bReport;
    InvalidateGeometry();
  }
}

void xiiJoltStaticCollisionComponent::SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    InvalidateGeometry();
  }
}

void xiiJoltStaticCollisionComponent::SetSurface(const xiiSurfaceResourceHandle& hSurface)
{
  if (m_hSurface != hSurface)
  {
    m_hSurface = hSurface;
    InvalidateGeometry();
  }
}

void xiiJoltStaticCollisionComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  ref_stream.GetStream() << m_hCollisionMesh;
  ref_stream.GetStream() << m_hSurface;
  ref_stream.GetStream() << m_uiCollisionLayer << m_bReportContacts;
}

void xiiJoltStaticCollisionComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  ref_stream.GetStream() >> m_hCollisionMesh;
  ref_stream.GetStream() >> m_hSurface;
  ref_stream.GetStream() >> m_uiCollisionLayer >> m_bReportContacts;
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltStaticCollisionComponent);
