#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltActorComponent, 2)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Actors"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiJoltActorComponent::xiiJoltActorComponent()  = default;
xiiJoltActorComponent::~xiiJoltActorComponent() = default;

void xiiJoltActorComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
}

void xiiJoltActorComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
}

void xiiJoltActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == xiiInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    m_uiObjectFilterID          = pModule->CreateObjectFilterID();
  }
}

void xiiJoltActorComponent::OnDeactivated()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    pBodies->RemoveBody(bodyId);
    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  pModule->DeallocateUserData(m_uiUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

void xiiJoltActorComponent::GatherShapes(xiiDynamicArray<xiiJoltSubShape>& shapes, xiiGameObject* pObject, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  xiiHybridArray<xiiJoltShapeComponent*, 8> shapeComps;
  pObject->TryGetComponentsOfBaseType(shapeComps);

  for (auto pShape : shapeComps)
  {
    if (pShape->IsActive())
    {
      pShape->CreateShapes(shapes, rootTransform, fDensity, pMaterial);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const xiiJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<xiiJoltActorComponent>(pActorComponent))
      continue;

    GatherShapes(shapes, itChild, rootTransform, fDensity, pMaterial);
  }
}

xiiResult xiiJoltActorComponent::CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const xiiJoltMaterial* pMaterial)
{
  xiiHybridArray<xiiJoltSubShape, 16> shapes;
  xiiTransform                        towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f); // pretend like there is no scaling at the root, so that each shape applies its scale

  CreateShapes(shapes, towner, fDensity, pMaterial);
  GatherShapes(shapes, GetOwner(), towner, fDensity, pMaterial);

  auto cleanShapes = [&]() {
    for (auto& s : shapes)
    {
      if (s.m_pShape)
      {
        s.m_pShape->Release();
      }
    }
  };

  XII_SCOPE_EXIT(cleanShapes());

  if (shapes.IsEmpty())
    return XII_FAILURE;

  if (shapes.GetCount() > 0)
  {
    JPH::StaticCompoundShapeSettings opt;

    for (auto shape : shapes)
    {
      auto pShape = shape.m_pShape;

      if (!shape.m_Transform.m_vScale.IsEqual(xiiVec3(1.0f), 0.01f))
      {
        auto* pScaledShape = new JPH::ScaledShape(pShape, xiiJoltConversionUtils::ToVec3(shape.m_Transform.m_vScale));
        pShape             = pScaledShape;
      }

      opt.AddShape(xiiJoltConversionUtils::ToVec3(shape.m_Transform.m_vPosition), xiiJoltConversionUtils::ToQuat(shape.m_Transform.m_qRotation).Normalized(), pShape);
    }

    auto res = opt.Create();
    if (!res.IsValid())
      return XII_FAILURE;

    pSettings->SetShape(res.Get());
    return XII_SUCCESS;
  }
  else
  {
    JPH::Shape* pShape = shapes[0].m_pShape;

    if (!shapes[0].m_Transform.m_vScale.IsEqual(xiiVec3(1.0f), 0.01f))
    {
      auto* pScaledShape = new JPH::ScaledShape(pShape, xiiJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vScale));
      pShape             = pScaledShape;
    }

    if (!shapes[0].m_Transform.m_vPosition.IsZero(0.01f) || shapes[0].m_Transform.m_qRotation != xiiQuat::IdentityQuaternion())
    {
      JPH::RotatedTranslatedShapeSettings opt(xiiJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vPosition), xiiJoltConversionUtils::ToQuat(shapes[0].m_Transform.m_qRotation), pShape);

      auto res = opt.Create();
      if (!res.IsValid())
        return XII_FAILURE;

      pShape = res.Get();
    }

    pSettings->SetShape(pShape);
    return XII_SUCCESS;
  }
}

void xiiJoltActorComponent::ExtractSubShapeGeometry(const xiiGameObject* pObject, xiiMsgExtractGeometry& msg) const
{
  xiiHybridArray<const xiiJoltShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    if (pShape->IsActive())
    {
      pShape->ExtractGeometry(msg);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const xiiJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<xiiJoltActorComponent>(pActorComponent))
      continue;

    ExtractSubShapeGeometry(itChild, msg);
  }
}

const xiiJoltUserData* xiiJoltActorComponent::GetUserData() const
{
  const xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

void xiiJoltActorComponent::SetInitialObjectFilterID(xiiUInt32 uiObjectFilterID)
{
  XII_ASSERT_DEBUG(!IsActiveAndSimulating(), "The object filter ID can't be changed after simulation has started.");
  m_uiObjectFilterID = uiObjectFilterID;
}
