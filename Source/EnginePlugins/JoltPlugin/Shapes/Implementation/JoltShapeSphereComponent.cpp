#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltShapeSphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiSphereVisualizerAttribute("Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltShapeSphereComponent::xiiJoltShapeSphereComponent()  = default;
xiiJoltShapeSphereComponent::~xiiJoltShapeSphereComponent() = default;

void xiiJoltShapeSphereComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
}

void xiiJoltShapeSphereComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_fRadius;
}

void xiiJoltShapeSphereComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), m_fRadius), xiiInvalidSpatialDataCategory);
}

void xiiJoltShapeSphereComponent::SetRadius(float f)
{
  m_fRadius = xiiMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeSphereComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::SphereShape(m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<xiiUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape         = pNewShape;
  sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeSphereComponent);
