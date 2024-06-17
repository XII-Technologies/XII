#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiHeadBoneComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(80)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(89.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
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
  qOld = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), m_CurVerticalRotation);
  qNew = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), m_NewVerticalRotation);

  const xiiQuat qChange = qNew * qOld.GetInverse();

  const xiiQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void xiiHeadBoneComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void xiiHeadBoneComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void xiiHeadBoneComponent::SetVerticalRotation(float fRadians)
{
  m_NewVerticalRotation = xiiAngle::MakeFromRadian(fRadians);
}

void xiiHeadBoneComponent::ChangeVerticalRotation(float fRadians)
{
  m_NewVerticalRotation += xiiAngle::MakeFromRadian(fRadians);
}
