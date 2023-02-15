#include <GameplayPlugin/GameplayPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameplayPlugin/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiHeadBoneComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(80)), new xiiClampValueAttribute(xiiAngle::Degree(0.0f), xiiAngle::Degree(89.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Transform"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetVerticalRotation, In, "Radians"),
    XII_SCRIPT_FUNCTION_PROPERTY(ChangeVerticalRotation, In, "Radians"),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHeadBoneComponent::xiiHeadBoneComponent()  = default;
xiiHeadBoneComponent::~xiiHeadBoneComponent() = default;

void xiiHeadBoneComponent::Update()
{
  m_NewVerticalRotation = xiiMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);

  xiiQuat qOld, qNew;
  qOld.SetFromAxisAndAngle(xiiVec3(0, 1, 0), m_CurVerticalRotation);
  qNew.SetFromAxisAndAngle(xiiVec3(0, 1, 0), m_NewVerticalRotation);

  const xiiQuat qChange = qNew * -qOld;

  const xiiQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void xiiHeadBoneComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void xiiHeadBoneComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void xiiHeadBoneComponent::SetVerticalRotation(float radians)
{
  m_NewVerticalRotation = xiiAngle::Radian(radians);
}

void xiiHeadBoneComponent::ChangeVerticalRotation(float radians)
{
  m_NewVerticalRotation += xiiAngle::Radian(radians);
}
