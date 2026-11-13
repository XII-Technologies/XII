/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginJolt/Authoring/JoltComponentAdapters.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltAuthoringAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  bool CanEdit(const xiiDocumentObject* object)
  {
    const auto* document = object->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
    if (document->IsReadOnly())
      return false;
    const auto* scene = xiiDynamicCast<const xiiSceneDocument*>(document);
    return !scene || scene->GetGameMode() == GameMode::Off;
  }
  xiiQtEngineDocumentWindow* Window(const xiiDocumentObject* object)
  {
    return qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(object->GetDocumentObjectManager()->GetDocument()->GetMainDocument()));
  }
  template <typename Gizmo, typename Adapter>
  void Configure(Gizmo& gizmo, Adapter* adapter, const xiiDocumentObject* object, void (Adapter::*callback)(const xiiGizmoEvent&))
  {
    gizmo.SetOwner(Window(object), nullptr);
    gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(callback, adapter));
  }
  void ReleaseActiveGizmo(xiiArrayPtr<xiiGizmo*> gizmos)
  {
    for (auto* gizmo : gizmos)
      if (gizmo->IsActiveInputContext())
      {
        gizmo->FocusLost(true);
        xiiEditorInputContext::SetActiveInputContext(nullptr);
      }
  }
} // namespace

xiiJoltShapeManipulatorAdapter::~xiiJoltShapeManipulatorAdapter()
{
  if (m_bInteracting)
  {
    m_bInteracting = false;
    CancelTemporayInteraction();
  }
  xiiGizmo* gizmos[] = {&m_Translate, &m_Rotate, &m_Box, &m_Sphere, &m_Capsule};
  ReleaseActiveGizmo(gizmos);
}
void xiiJoltShapeManipulatorAdapter::Finalize()
{
  Configure(m_Translate, this, m_pObject, &xiiJoltShapeManipulatorAdapter::GizmoEvent);
  Configure(m_Rotate, this, m_pObject, &xiiJoltShapeManipulatorAdapter::GizmoEvent);
  Configure(m_Box, this, m_pObject, &xiiJoltShapeManipulatorAdapter::GizmoEvent);
  Configure(m_Sphere, this, m_pObject, &xiiJoltShapeManipulatorAdapter::GizmoEvent);
  Configure(m_Capsule, this, m_pObject, &xiiJoltShapeManipulatorAdapter::GizmoEvent);
}
void xiiJoltShapeManipulatorAdapter::Update() { UpdateGizmoTransform(); }
void xiiJoltShapeManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_bInteracting && !CanEdit(m_pObject))
  {
    m_bInteracting = false;
    CancelTemporayInteraction();
    xiiGizmo* gizmos[] = {&m_Translate, &m_Rotate, &m_Box, &m_Sphere, &m_Capsule};
    ReleaseActiveGizmo(gizmos);
  }
  if (m_bInteracting)
    return;
  const auto* attribute = static_cast<const xiiJoltShapeManipulatorAttribute*>(m_pManipulatorAttr);
  auto*       accessor  = GetObjectAccessor();
  const auto  parent    = GetObjectTransform();
  const auto  position  = accessor->Get<xiiVec3>(m_pObject, GetProperty("LocalPosition"));
  const auto  rotation  = accessor->Get<xiiQuat>(m_pObject, GetProperty("LocalRotation"));
  auto        transform = parent * xiiTransform(position, rotation);
  const bool  valid     = transform.m_vPosition.IsValid() && transform.m_qRotation.IsValid() && transform.m_vScale.IsValid() &&
    transform.m_vScale.x != 0 && transform.m_vScale.y != 0 && transform.m_vScale.z != 0;
  const bool visible = m_bManipulatorIsVisible && valid && CanEdit(m_pObject);
  m_Translate.SetVisible(visible);
  m_Rotate.SetVisible(visible);
  auto frame     = transform;
  frame.m_vScale = xiiVec3(1.0f);
  m_Translate.SetTransformation(frame);
  m_Rotate.SetTransformation(frame);
  const bool box     = attribute->m_sProperty1 == "Dimensions";
  const bool radius  = attribute->m_sProperty1 == "Radius";
  const bool height  = !attribute->m_sProperty2.IsEmpty();
  const bool capsule = m_pObject->GetTypeAccessor().GetType()->GetTypeName().IsEqual("xiiJoltCapsuleShapeComponent");
  m_Box.SetVisible(visible && (box || (height && !capsule)));
  m_Sphere.SetVisible(visible && radius && !height);
  m_Capsule.SetVisible(visible && capsule);
  if (box)
    m_Box.SetSize(accessor->Get<xiiVec3>(m_pObject, GetProperty("Dimensions")));
  if (radius)
  {
    const float r = accessor->Get<float>(m_pObject, GetProperty("Radius"));
    m_Sphere.SetOuterSphere(r);
    if (height)
    {
      const float h = accessor->Get<float>(m_pObject, GetProperty("Height"));
      m_Capsule.SetLength(h);
      m_Capsule.SetRadius(r);
      m_vCylinderSize = xiiVec3(2 * r, 2 * r, h);
      if (!capsule)
        m_Box.SetSize(m_vCylinderSize);
    }
  }
  m_Box.SetTransformation(transform);
  m_Sphere.SetTransformation(transform);
  m_Capsule.SetTransformation(transform);
}
void xiiJoltShapeManipulatorAdapter::GizmoEvent(const xiiGizmoEvent& event)
{
  if (event.m_Type == xiiGizmoEvent::Type::BeginInteractions)
  {
    if (!CanEdit(m_pObject))
      return;
    m_bInteracting = true;
    BeginTemporaryInteraction();
    return;
  }
  if (!m_bInteracting)
    return;
  if (event.m_Type == xiiGizmoEvent::Type::CancelInteractions || event.m_Type == xiiGizmoEvent::Type::EndInteractions)
  {
    if (event.m_Type == xiiGizmoEvent::Type::CancelInteractions)
      CancelTemporayInteraction();
    else
      EndTemporaryInteraction();
    m_bInteracting = false;
    Update();
    return;
  }
  if (event.m_Type != xiiGizmoEvent::Type::Interaction)
    return;
  if (!CanEdit(m_pObject))
    return;
  const auto* attribute = static_cast<const xiiJoltShapeManipulatorAttribute*>(m_pManipulatorAttr);
  if (event.m_pGizmo == &m_Translate || event.m_pGizmo == &m_Rotate)
  {
    const auto local = xiiTransform::MakeLocalTransform(GetObjectTransform(), static_cast<const xiiGizmo*>(event.m_pGizmo)->GetTransformation());
    if (event.m_pGizmo == &m_Translate)
      ChangeProperties("LocalPosition", local.m_vPosition);
    else
      ChangeProperties("LocalRotation", local.m_qRotation);
  }
  else if (event.m_pGizmo == &m_Box)
  {
    const auto size = m_Box.GetSize().CompMax(xiiVec3(0.001f));
    if (attribute->m_sProperty1 == "Dimensions")
      ChangeProperties("Dimensions", size);
    else
    {
      const float radial = xiiMath::Abs(size.x - m_vCylinderSize.x) >= xiiMath::Abs(size.y - m_vCylinderSize.y) ? size.x : size.y;
      ChangeProperties("Radius", 0.5f * radial, "Height", size.z);
    }
  }
  else if (event.m_pGizmo == &m_Sphere)
    ChangeProperties("Radius", xiiMath::Max(0.001f, m_Sphere.GetOuterRadius()));
  else if (event.m_pGizmo == &m_Capsule)
    ChangeProperties("Radius", xiiMath::Max(0.001f, m_Capsule.GetRadius()), "Height", xiiMath::Max(0.001f, m_Capsule.GetLength()));
}

xiiJoltJointManipulatorAdapter::~xiiJoltJointManipulatorAdapter()
{
  if (m_bInteracting)
  {
    m_bInteracting = false;
    CancelTemporayInteraction();
  }
  xiiGizmo* gizmos[] = {&m_Translate, &m_Rotate};
  ReleaseActiveGizmo(gizmos);
}
void xiiJoltJointManipulatorAdapter::Finalize()
{
  Configure(m_Translate, this, m_pObject, &xiiJoltJointManipulatorAdapter::GizmoEvent);
  Configure(m_Rotate, this, m_pObject, &xiiJoltJointManipulatorAdapter::GizmoEvent);
}
bool xiiJoltJointManipulatorAdapter::GetActorTransform(xiiTransform& transform) const
{
  const auto* attribute = static_cast<const xiiJoltJointManipulatorAttribute*>(m_pManipulatorAttr);
  const auto  reference = GetObjectAccessor()->Get<xiiString>(m_pObject, GetProperty(attribute->m_sProperty2));
  transform             = xiiTransform::MakeIdentity();
  if (reference.IsEmpty())
    return attribute->m_sProperty2 == "ActorA";
  if (!xiiConversionUtils::IsStringUuid(reference))
    return false;
  const auto other = GetObjectAccessor()->Get<xiiString>(m_pObject, GetProperty(attribute->m_sProperty2 == "ActorA" ? "ActorB" : "ActorA"));
  if (reference.IsEqual_NoCase(other))
    return false;
  const auto* document = m_pObject->GetDocumentObjectManager()->GetDocument();
  const auto* actor    = document->GetObjectManager()->GetObject(xiiConversionUtils::ConvertStringToUuid(reference));
  if (!actor)
    return false;
  bool hasActor = false;
  for (const auto* component : actor->GetChildren())
    if (component->GetTypeAccessor().GetType()->IsDerivedFrom<xiiJoltActorComponent>())
      hasActor = true;
  if (!hasActor)
    return false;
  if (document->ComputeObjectTransformation(actor, transform).Failed())
    return false;
  return transform.m_vPosition.IsValid() && transform.m_qRotation.IsValid() && transform.m_vScale.IsValid() &&
    transform.m_vScale.x != 0 && transform.m_vScale.y != 0 && transform.m_vScale.z != 0;
}
void xiiJoltJointManipulatorAdapter::Update() { UpdateGizmoTransform(); }
void xiiJoltJointManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_bInteracting && !CanEdit(m_pObject))
  {
    m_bInteracting = false;
    CancelTemporayInteraction();
    xiiGizmo* gizmos[] = {&m_Translate, &m_Rotate};
    ReleaseActiveGizmo(gizmos);
  }
  if (m_bInteracting)
    return;
  xiiTransform actor;
  const bool   visible = m_bManipulatorIsVisible && CanEdit(m_pObject) && GetActorTransform(actor);
  m_Translate.SetVisible(visible);
  const auto typeName = m_pObject->GetTypeAccessor().GetType()->GetTypeName();
  m_Rotate.SetVisible(visible && !typeName.IsEqual("xiiJoltPointJointComponent") && !typeName.IsEqual("xiiJoltDistanceJointComponent"));
  if (!visible)
    return;
  const auto frame  = GetObjectAccessor()->Get<xiiTransform>(m_pObject, GetProperty(m_pManipulatorAttr->m_sProperty1));
  auto       global = actor * frame;
  global.m_vScale   = xiiVec3(1.0f);
  m_Translate.SetTransformation(global);
  m_Rotate.SetTransformation(global);
}
void xiiJoltJointManipulatorAdapter::GizmoEvent(const xiiGizmoEvent& event)
{
  if (event.m_Type == xiiGizmoEvent::Type::BeginInteractions)
  {
    if (!CanEdit(m_pObject))
      return;
    m_bInteracting = true;
    BeginTemporaryInteraction();
    return;
  }
  if (!m_bInteracting)
    return;
  if (event.m_Type == xiiGizmoEvent::Type::CancelInteractions || event.m_Type == xiiGizmoEvent::Type::EndInteractions)
  {
    if (event.m_Type == xiiGizmoEvent::Type::CancelInteractions)
      CancelTemporayInteraction();
    else
      EndTemporaryInteraction();
    m_bInteracting = false;
    Update();
    return;
  }
  if (event.m_Type != xiiGizmoEvent::Type::Interaction || !CanEdit(m_pObject))
    return;
  xiiTransform actor;
  if (!GetActorTransform(actor))
    return;
  auto       frame  = GetObjectAccessor()->Get<xiiTransform>(m_pObject, GetProperty(m_pManipulatorAttr->m_sProperty1));
  const auto global = static_cast<const xiiGizmo*>(event.m_pGizmo)->GetTransformation();
  if (event.m_pGizmo == &m_Translate)
    frame.m_vPosition = (actor.m_qRotation.GetInverse() * (global.m_vPosition - actor.m_vPosition)).CompDiv(actor.m_vScale);
  else
    frame.m_qRotation = actor.m_qRotation.GetInverse() * global.m_qRotation;
  frame.m_vScale = xiiVec3(1.0f);
  ChangeProperties(m_pManipulatorAttr->m_sProperty1, frame);
}

void xiiJoltVisualizerAdapter::Finalize()
{
  const auto* document   = xiiDynamicCast<const xiiAssetDocument*>(m_pObject->GetDocumentObjectManager()->GetDocument());
  m_Sync.m_ComponentGuid = m_pObject->GetGuid();
  if (document)
    document->AddSyncObject(&m_Sync);
}
void xiiJoltVisualizerAdapter::Update() { UpdateGizmoTransform(); }
void xiiJoltVisualizerAdapter::UpdateGizmoTransform()
{
  if (m_Sync.m_bVisible != m_bVisualizerIsVisible)
  {
    m_Sync.m_bVisible = m_bVisualizerIsVisible;
    m_Sync.SetModified();
  }
}

void xiiRegisterJoltComponentAdapters()
{
  auto& manipulators = xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory;
  manipulators.RegisterCreator(xiiGetStaticRTTI<xiiJoltShapeManipulatorAttribute>(), [](const xiiRTTI*) -> xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiJoltShapeManipulatorAdapter); });
  manipulators.RegisterCreator(xiiGetStaticRTTI<xiiJoltJointManipulatorAttribute>(), [](const xiiRTTI*) -> xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiJoltJointManipulatorAdapter); });
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiJoltVisualizerAttribute>(), [](const xiiRTTI*) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiJoltVisualizerAdapter); });
}
void xiiUnregisterJoltComponentAdapters()
{
  // Clear active adapters while their code and sync object types are still loaded.
  for (const auto* manager : xiiDocumentManager::GetAllDocumentManagers())
    for (const auto* document : manager->GetAllOpenDocuments())
    {
      const xiiHybridArray<xiiPropertySelection, 8>* selection = nullptr;
      const auto*                                    attribute = xiiManipulatorManager::GetSingleton()->GetActiveManipulator(document, selection);
      if (attribute && (attribute->IsInstanceOf<xiiJoltShapeManipulatorAttribute>() || attribute->IsInstanceOf<xiiJoltJointManipulatorAttribute>()))
        xiiManipulatorManager::GetSingleton()->ClearActiveManipulator(document);
    }
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiJoltVisualizerAttribute>());
  auto& manipulators = xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory;
  manipulators.UnregisterCreator(xiiGetStaticRTTI<xiiJoltShapeManipulatorAttribute>());
  manipulators.UnregisterCreator(xiiGetStaticRTTI<xiiJoltJointManipulatorAttribute>());
}

XII_STATICLINK_FILE(EditorPluginJolt, EditorPluginJolt_Authoring_Implementation_JoltComponentAdapters);
