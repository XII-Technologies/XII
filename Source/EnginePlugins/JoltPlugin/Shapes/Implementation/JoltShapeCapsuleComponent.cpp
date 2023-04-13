#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltShapeCapsuleComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCapsuleManipulatorAttribute("Height", "Radius"),
    new xiiCapsuleVisualizerAttribute("Height", "Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltShapeCapsuleComponent::xiiJoltShapeCapsuleComponent()  = default;
xiiJoltShapeCapsuleComponent::~xiiJoltShapeCapsuleComponent() = default;

void xiiJoltShapeCapsuleComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void xiiJoltShapeCapsuleComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void xiiJoltShapeCapsuleComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingSphere(xiiVec3(0, 0, -m_fHeight * 0.5f), m_fRadius), xiiInvalidSpatialDataCategory);
  msg.AddBounds(xiiBoundingSphere(xiiVec3(0, 0, +m_fHeight * 0.5f), m_fRadius), xiiInvalidSpatialDataCategory);
}

void xiiJoltShapeCapsuleComponent::SetRadius(float f)
{
  m_fRadius = xiiMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeCapsuleComponent::SetHeight(float f)
{
  m_fHeight = xiiMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeCapsuleComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  JPH::Ref<JPH::CapsuleShape> pNewShape = new JPH::CapsuleShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<xiiUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  JPH::Ref<JPH::RotatedTranslatedShapeSettings> pRotShapeSet = new JPH::RotatedTranslatedShapeSettings(JPH::Vec3::sZero(), JPH::Quat::sRotation(JPH::Vec3::sAxisX(), xiiAngle::Degree(90).GetRadian()), pNewShape);

  JPH::Shape* pRotShape = pRotShapeSet->Create().Get().GetPtr();
  pRotShape->SetUserData(reinterpret_cast<xiiUInt64>(GetUserData()));

  xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape         = pRotShape;
  sub.m_pShape->AddRef();
  sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCapsuleComponent);
