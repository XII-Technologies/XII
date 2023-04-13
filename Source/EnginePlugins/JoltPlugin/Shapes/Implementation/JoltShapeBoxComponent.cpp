#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltShapeBoxComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.5f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new xiiBoxVisualizerAttribute("HalfExtents", 2.0f),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltShapeBoxComponent::xiiJoltShapeBoxComponent()  = default;
xiiJoltShapeBoxComponent::~xiiJoltShapeBoxComponent() = default;

void xiiJoltShapeBoxComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_vHalfExtents;
}

void xiiJoltShapeBoxComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();
  s >> m_vHalfExtents;
}

void xiiJoltShapeBoxComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingBox(-m_vHalfExtents, m_vHalfExtents), xiiInvalidSpatialDataCategory);
}

void xiiJoltShapeBoxComponent::ExtractGeometry(xiiMsgExtractGeometry& msg) const
{
  msg.AddBox(GetOwner()->GetGlobalTransform(), m_vHalfExtents * 2.0f);
}

void xiiJoltShapeBoxComponent::SetHalfExtents(const xiiVec3& value)
{
  m_vHalfExtents = value.CompMax(xiiVec3::ZeroVector());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeBoxComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  xiiVec3 size = m_vHalfExtents;
  size.x       = xiiMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y       = xiiMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z       = xiiMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(xiiJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<xiiUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape         = pNewShape;
  sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeBoxComponent);

