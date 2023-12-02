#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/DeviceTrackingComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRPoseLocation, 1)
XII_BITFLAGS_CONSTANTS(xiiXRPoseLocation::Grip, xiiXRPoseLocation::Aim)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiDeviceTrackingComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("DeviceType", xiiXRDeviceType, GetDeviceType, SetDeviceType),
    XII_ENUM_ACCESSOR_PROPERTY("PoseLocation", xiiXRPoseLocation, GetPoseLocation, SetPoseLocation),
    XII_ENUM_ACCESSOR_PROPERTY("TransformSpace", xiiXRTransformSpace, GetTransformSpace, SetTransformSpace),
    XII_MEMBER_PROPERTY("Rotation", m_bRotation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Scale", m_bScale)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("XR"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Alpha),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiDeviceTrackingComponent::xiiDeviceTrackingComponent()  = default;
xiiDeviceTrackingComponent::~xiiDeviceTrackingComponent() = default;

void xiiDeviceTrackingComponent::SetDeviceType(xiiEnum<xiiXRDeviceType> type)
{
  m_DeviceType = type;
}

xiiEnum<xiiXRDeviceType> xiiDeviceTrackingComponent::GetDeviceType() const
{
  return m_DeviceType;
}

void xiiDeviceTrackingComponent::SetPoseLocation(xiiEnum<xiiXRPoseLocation> poseLocation)
{
  m_PoseLocation = poseLocation;
}

xiiEnum<xiiXRPoseLocation> xiiDeviceTrackingComponent::GetPoseLocation() const
{
  return m_PoseLocation;
}

void xiiDeviceTrackingComponent::SetTransformSpace(xiiEnum<xiiXRTransformSpace> space)
{
  m_Space = space;
}

xiiEnum<xiiXRTransformSpace> xiiDeviceTrackingComponent::GetTransformSpace() const
{
  return m_Space;
}

void xiiDeviceTrackingComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_DeviceType;
  s << m_PoseLocation;
  s << m_Space;
  s << m_bRotation;
  s << m_bScale;
}

void xiiDeviceTrackingComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_DeviceType;
  if (uiVersion >= 2)
  {
    s >> m_PoseLocation;
  }
  s >> m_Space;
  if (uiVersion >= 3)
  {
    s >> m_bRotation;
    s >> m_bScale;
  }
}

void xiiDeviceTrackingComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (xiiXRInterface* pXRInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRInterface>())
  {
    if (!pXRInterface->IsInitialized())
      return;

    xiiXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_DeviceType);
    if (deviceID != -1)
    {
      const xiiXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
      xiiVec3                 vPosition;
      xiiQuat                 qRotation;
      if (m_PoseLocation == xiiXRPoseLocation::Grip && state.m_bGripPoseIsValid)
      {
        vPosition = state.m_vGripPosition;
        qRotation = state.m_qGripRotation;
      }
      else if (m_PoseLocation == xiiXRPoseLocation::Aim && state.m_bAimPoseIsValid)
      {
        vPosition = state.m_vAimPosition;
        qRotation = state.m_qAimRotation;
      }
      else
      {
        return;
      }
      if (m_Space == xiiXRTransformSpace::Local)
      {
        GetOwner()->SetLocalPosition(vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(qRotation);
      }
      else
      {
        xiiTransform add;
        add.SetIdentity();
        if (const xiiStageSpaceComponentManager* pStageMan = GetWorld()->GetComponentManager<xiiStageSpaceComponentManager>())
        {
          if (const xiiStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }

        const xiiTransform global(add * xiiTransform(vPosition, qRotation));
        xiiTransform       local;
        if (GetOwner()->GetParent() != nullptr)
        {
          local.SetLocalTransform(GetOwner()->GetParent()->GetGlobalTransform(), global);
        }
        else
        {
          local = global;
        }
        GetOwner()->SetLocalPosition(local.m_vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(local.m_qRotation);
        if (m_bScale)
          GetOwner()->SetLocalScaling(local.m_vScale);
      }
    }
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DeviceTrackingComponent);
