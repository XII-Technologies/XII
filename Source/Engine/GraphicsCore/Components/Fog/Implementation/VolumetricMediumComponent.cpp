/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Fog/VolumetricMediumComponent.h>

xiiVolumetricMediumComponentManager::xiiVolumetricMediumComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiVolumetricMediumComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

void xiiVolumetricMediumComponentManager::Initialize()
{
  auto description    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiVolumetricMediumComponentManager::Update, this);
  description.m_Phase = xiiWorldUpdatePhase::PostTransform;
  RegisterUpdateFunction(description);
}

void xiiVolumetricMediumComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  XII_IGNORE_UNUSED(context);
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
      it->SynchronizeMedium();
  }
}

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVolumetricMediumComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("Shape", xiiVolumetricMediumShape, GetShape, SetShape),
    XII_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.01f), xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Scattering", GetScattering, SetScattering)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.08f)), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant()), new xiiSuffixAttribute(" m^-1")),
    XII_ACCESSOR_PROPERTY("Absorption", GetAbsorption, SetAbsorption)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.02f)), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant()), new xiiSuffixAttribute(" m^-1")),
    XII_ACCESSOR_PROPERTY("Emission", GetEmission, SetEmission)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3::MakeZero()), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant()), new xiiSuffixAttribute(" cd/m^2")),
    XII_ACCESSOR_PROPERTY("Anisotropy", GetAnisotropy, SetAnisotropy)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(-0.95f, 0.95f)),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new xiiDefaultValueAttribute(0)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgTransformChanged, OnMsgTransformChanged),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lighting/Volumetrics"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiVolumetricMediumComponent::xiiVolumetricMediumComponent()  = default;
xiiVolumetricMediumComponent::~xiiVolumetricMediumComponent() = default;

void xiiVolumetricMediumComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();
  stream << m_Shape;
  stream << m_vHalfExtents;
  stream << m_vScattering;
  stream << m_vAbsorption;
  stream << m_vEmission;
  stream << m_fAnisotropy;
  stream << m_iPriority;
}

void xiiVolumetricMediumComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();
  stream >> m_Shape;
  stream >> m_vHalfExtents;
  stream >> m_vScattering;
  stream >> m_vAbsorption;
  stream >> m_vEmission;
  stream >> m_fAnisotropy;
  stream >> m_iPriority;

  m_vHalfExtents = m_vHalfExtents.CompMax(xiiVec3(0.01f));
  m_vScattering  = m_vScattering.CompMax(xiiVec3::MakeZero());
  m_vAbsorption  = m_vAbsorption.CompMax(xiiVec3::MakeZero());
  m_vEmission    = m_vEmission.CompMax(xiiVec3::MakeZero());
  m_fAnisotropy  = xiiMath::Clamp(m_fAnisotropy, -0.95f, 0.95f);
}

xiiResult xiiVolumetricMediumComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  out_bAlwaysVisible = false;
  out_bounds = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), m_vHalfExtents, m_vHalfExtents.GetLength());
  return XII_SUCCESS;
}

void xiiVolumetricMediumComponent::SetShape(xiiEnum<xiiVolumetricMediumShape> shape)
{
  if (m_Shape == shape)
    return;
  m_Shape = shape;
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

xiiEnum<xiiVolumetricMediumShape> xiiVolumetricMediumComponent::GetShape() const { return m_Shape; }

void xiiVolumetricMediumComponent::SetHalfExtents(xiiVec3 vHalfExtents)
{
  vHalfExtents = vHalfExtents.CompMax(xiiVec3(0.01f));
  if (m_vHalfExtents == vHalfExtents)
    return;
  m_vHalfExtents = vHalfExtents;
  m_bSynchronizationDirty = true;
  TriggerLocalBoundsUpdate();
  SynchronizeMedium();
}

xiiVec3 xiiVolumetricMediumComponent::GetHalfExtents() const { return m_vHalfExtents; }

void xiiVolumetricMediumComponent::SetScattering(xiiVec3 vScattering)
{
  m_vScattering = vScattering.CompMax(xiiVec3::MakeZero());
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

xiiVec3 xiiVolumetricMediumComponent::GetScattering() const { return m_vScattering; }

void xiiVolumetricMediumComponent::SetAbsorption(xiiVec3 vAbsorption)
{
  m_vAbsorption = vAbsorption.CompMax(xiiVec3::MakeZero());
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

xiiVec3 xiiVolumetricMediumComponent::GetAbsorption() const { return m_vAbsorption; }

void xiiVolumetricMediumComponent::SetEmission(xiiVec3 vEmission)
{
  m_vEmission = vEmission.CompMax(xiiVec3::MakeZero());
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

xiiVec3 xiiVolumetricMediumComponent::GetEmission() const { return m_vEmission; }

void xiiVolumetricMediumComponent::SetAnisotropy(float fAnisotropy)
{
  m_fAnisotropy = xiiMath::Clamp(fAnisotropy, -0.95f, 0.95f);
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

float xiiVolumetricMediumComponent::GetAnisotropy() const { return m_fAnisotropy; }

void xiiVolumetricMediumComponent::SetPriority(xiiInt32 iPriority)
{
  m_iPriority = iPriority;
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

xiiInt32 xiiVolumetricMediumComponent::GetPriority() const { return m_iPriority; }

void xiiVolumetricMediumComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

void xiiVolumetricMediumComponent::OnDeactivated()
{
  ReleaseMedium();
  m_bSynchronizationDirty = true;
  SUPER::OnDeactivated();
}

void xiiVolumetricMediumComponent::OnMsgTransformChanged(xiiMsgTransformChanged& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  m_bSynchronizationDirty = true;
  SynchronizeMedium();
}

void xiiVolumetricMediumComponent::SynchronizeMedium()
{
  if (!IsActiveAndInitialized() || !xiiVolumetricMediumManager::IsSubsystemInitialized())
    return;

  const xiiTransform& transform = GetOwner()->GetGlobalTransform();
  if (m_hMedium.IsValid() && !m_bSynchronizationDirty && transform == m_LastGlobalTransform)
    return;

  xiiVolumetricMediumDescription description;
  description.m_Shape        = m_Shape;
  description.m_vCenter      = transform.m_vPosition;
  description.m_qRotation    = transform.m_qRotation;
  description.m_vHalfExtents = m_vHalfExtents.CompMul(transform.m_vScale.Abs()).CompMax(xiiVec3(0.01f));
  description.m_vScattering  = m_vScattering;
  description.m_vAbsorption  = m_vAbsorption;
  description.m_vEmission    = m_vEmission;
  description.m_fAnisotropy  = m_fAnisotropy;
  description.m_iPriority    = m_iPriority;

  if (m_hMedium.IsValid() && xiiVolumetricMediumManager::UpdateMedium(m_hMedium, description).Succeeded())
  {
    m_LastGlobalTransform   = transform;
    m_bSynchronizationDirty = false;
    return;
  }

  ReleaseMedium();
  m_hMedium = xiiVolumetricMediumManager::RegisterMedium(description);
  if (m_hMedium.IsValid())
  {
    m_LastGlobalTransform   = transform;
    m_bSynchronizationDirty = false;
  }
}

void xiiVolumetricMediumComponent::ReleaseMedium()
{
  if (!m_hMedium.IsValid())
    return;
  xiiVolumetricMediumManager::UnregisterMedium(m_hMedium);
  m_hMedium = {};
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Fog_Implementation_VolumetricMediumComponent);
