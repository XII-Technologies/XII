#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCylinderComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltShapeCylinderComponent, 1, xiiComponentMode::Static)
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
    new xiiCylinderVisualizerAttribute(xiiBasisAxis::PositiveZ, "Height", "Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltShapeCylinderComponent::xiiJoltShapeCylinderComponent()  = default;
xiiJoltShapeCylinderComponent::~xiiJoltShapeCylinderComponent() = default;

void xiiJoltShapeCylinderComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void xiiJoltShapeCylinderComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void xiiJoltShapeCylinderComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingBox(xiiVec3(-m_fRadius, -m_fRadius, -m_fHeight * 0.5f), xiiVec3(m_fRadius, m_fRadius, m_fHeight * 0.5f)), xiiInvalidSpatialDataCategory);
}

void xiiJoltShapeCylinderComponent::SetRadius(float f)
{
  m_fRadius = xiiMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeCylinderComponent::SetHeight(float f)
{
  m_fHeight = xiiMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeCylinderComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::CylinderShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<xiiUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  const xiiQuat qTilt = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveY, xiiBasisAxis::PositiveZ);

  xiiTransform tOwn = GetOwner()->GetGlobalTransform();
  tOwn.m_vScale.x   = tOwn.m_vScale.z;
  tOwn.m_qRotation  = tOwn.m_qRotation * qTilt;

  xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape         = pNewShape;
  sub.m_Transform.SetLocalTransform(rootTransform, tOwn);
}
