/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, 2)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3::MakeZero())),
    XII_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    XII_RESOURCE_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
  }
  XII_END_PROPERTIES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltBoxShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Dimensions", GetDimensions, SetDimensions)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.001f), xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltSphereShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.001f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiJoltShapeComponent::OnActivated()
{
  SUPER::OnActivated();
  QueueBodyUpdate();
}

void xiiJoltShapeComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    pModule->UnregisterShape(GetHandle());
  QueueBodyUpdate();
  SUPER::OnDeactivated();
}

void xiiJoltShapeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->RegisterShape(GetHandle());
  QueueBodyUpdate();
}

void xiiJoltShapeComponent::SetLocalPosition(const xiiVec3& vPosition)
{
  if (!vPosition.IsValid() || m_vLocalPosition == vPosition)
    return;
  m_vLocalPosition = vPosition;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltShapeComponent::SetLocalRotation(const xiiQuat& qRotation)
{
  if (!qRotation.IsValid() || m_qLocalRotation == qRotation)
    return;
  m_qLocalRotation = qRotation;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltShapeComponent::SetSurface(const xiiSurfaceResourceHandle& hSurface)
{
  if (m_hSurface == hSurface)
    return;
  m_hSurface = hSurface;
  if (hSurface.IsValid())
    xiiResourceManager::PreloadResource(hSurface);
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_vLocalPosition << m_qLocalRotation << m_hSurface;
}
void xiiJoltShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  if (inout_stream.GetComponentTypeVersion(xiiJoltShapeComponent::GetStaticRTTI()) >= 2)
  {
    xiiVec3                  position;
    xiiQuat                  rotation;
    xiiSurfaceResourceHandle surface;
    inout_stream.GetStream() >> position >> rotation >> surface;
    SetLocalPosition(position);
    SetLocalRotation(rotation);
    SetSurface(surface);
  }
}

void xiiJoltShapeComponent::QueueBodyUpdate()
{
  xiiJoltActorComponent::QueueAncestorBodyUpdate(GetOwner());
}

xiiJoltBoxShapeComponent::xiiJoltBoxShapeComponent()  = default;
xiiJoltBoxShapeComponent::~xiiJoltBoxShapeComponent() = default;

void xiiJoltBoxShapeComponent::SetDimensions(const xiiVec3& vDimensions)
{
  if (!vDimensions.IsValid() || vDimensions.x <= 0.0f || vDimensions.y <= 0.0f || vDimensions.z <= 0.0f)
  {
    xiiLog::Warning("Jolt box dimensions must be finite and positive.");
    return;
  }

  if (m_vDimensions != vDimensions)
  {
    m_vDimensions = vDimensions;
    if (IsActiveAndInitialized())
      QueueBodyUpdate();
  }
}

void xiiJoltBoxShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_vDimensions;
}

void xiiJoltBoxShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiVec3 vDimensions = m_vDimensions;
  inout_stream.GetStream() >> vDimensions;
  SetDimensions(vDimensions);
}

xiiJoltSphereShapeComponent::xiiJoltSphereShapeComponent()  = default;
xiiJoltSphereShapeComponent::~xiiJoltSphereShapeComponent() = default;

void xiiJoltSphereShapeComponent::SetRadius(float fRadius)
{
  if (!xiiMath::IsFinite(fRadius) || fRadius <= 0.0f)
  {
    xiiLog::Warning("Jolt sphere radius must be finite and positive.");
    return;
  }

  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;
    if (IsActiveAndInitialized())
      QueueBodyUpdate();
  }
}

void xiiJoltSphereShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_fRadius;
}

void xiiJoltSphereShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  float fRadius = m_fRadius;
  inout_stream.GetStream() >> fRadius;
  SetRadius(fRadius);
}


// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltCapsuleShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltCapsuleShapeComponent::xiiJoltCapsuleShapeComponent()  = default;
xiiJoltCapsuleShapeComponent::~xiiJoltCapsuleShapeComponent() = default;
void xiiJoltCapsuleShapeComponent::SetRadius(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue <= 0.0f)
  {
    xiiLog::Warning("Jolt capsule radius must be finite and positive.");
    return;
  }
  if (m_fRadius == fValue)
    return;
  m_fRadius = fValue;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltCapsuleShapeComponent::SetHeight(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue <= 0.0f)
  {
    xiiLog::Warning("Jolt capsule height must be finite and positive.");
    return;
  }
  if (m_fHeight == fValue)
    return;
  m_fHeight = fValue;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltCapsuleShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_fRadius << m_fHeight;
}
void xiiJoltCapsuleShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  float fRadius = m_fRadius, fHeight = m_fHeight;
  inout_stream.GetStream() >> fRadius >> fHeight;
  SetRadius(fRadius);
  SetHeight(fHeight);
}

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltCylinderShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltCylinderShapeComponent::xiiJoltCylinderShapeComponent()  = default;
xiiJoltCylinderShapeComponent::~xiiJoltCylinderShapeComponent() = default;
void xiiJoltCylinderShapeComponent::SetRadius(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue <= 0.0f)
  {
    xiiLog::Warning("Jolt cylinder radius must be finite and positive.");
    return;
  }
  if (m_fRadius == fValue)
    return;
  m_fRadius = fValue;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltCylinderShapeComponent::SetHeight(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue <= 0.0f)
  {
    xiiLog::Warning("Jolt cylinder height must be finite and positive.");
    return;
  }
  if (m_fHeight == fValue)
    return;
  m_fHeight = fValue;
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltCylinderShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_fRadius << m_fHeight;
}
void xiiJoltCylinderShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  float fRadius = m_fRadius, fHeight = m_fHeight;
  inout_stream.GetStream() >> fRadius >> fHeight;
  SetRadius(fRadius);
  SetHeight(fHeight);
}

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltConvexHullShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("CollisionMesh", GetCollisionMesh, SetCollisionMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_JoltCollisionMesh")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltConvexHullShapeComponent::xiiJoltConvexHullShapeComponent()  = default;
xiiJoltConvexHullShapeComponent::~xiiJoltConvexHullShapeComponent() = default;
void xiiJoltConvexHullShapeComponent::SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh == hMesh)
    return;
  m_hCollisionMesh = hMesh;
  if (hMesh.IsValid())
    xiiResourceManager::PreloadResource(hMesh);
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltConvexHullShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_hCollisionMesh;
}
void xiiJoltConvexHullShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiJoltCollisionMeshResourceHandle hMesh;
  inout_stream.GetStream() >> hMesh;
  SetCollisionMesh(hMesh);
}

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltMeshShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("CollisionMesh", GetCollisionMesh, SetCollisionMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_JoltCollisionMesh")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltMeshShapeComponent::xiiJoltMeshShapeComponent()  = default;
xiiJoltMeshShapeComponent::~xiiJoltMeshShapeComponent() = default;
void xiiJoltMeshShapeComponent::SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh == hMesh)
    return;
  m_hCollisionMesh = hMesh;
  if (hMesh.IsValid())
    xiiResourceManager::PreloadResource(hMesh);
  if (IsActiveAndInitialized())
    QueueBodyUpdate();
}
void xiiJoltMeshShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_hCollisionMesh;
}
void xiiJoltMeshShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiJoltCollisionMeshResourceHandle hMesh;
  inout_stream.GetStream() >> hMesh;
  SetCollisionMesh(hMesh);
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltPrimitiveShapeComponent);
