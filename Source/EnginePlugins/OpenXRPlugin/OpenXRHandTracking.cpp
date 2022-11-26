#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRSingleton.h>

XII_IMPLEMENT_SINGLETON(xiiOpenXRHandTracking);

bool xiiOpenXRHandTracking::IsHandTrackingSupported(xiiOpenXR* pOpenXR)
{
  XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
  XrSystemProperties                systemProperties{XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties};
  XrResult                          res = xrGetSystemProperties(pOpenXR->m_instance, pOpenXR->m_systemId, &systemProperties);
  if (res == XrResult::XR_SUCCESS)
  {
    return handTrackingSystemProperties.supportsHandTracking;
  }
  return false;
}

xiiOpenXRHandTracking::xiiOpenXRHandTracking(xiiOpenXR* pOpenXR) :
  m_SingletonRegistrar(this), m_pOpenXR(pOpenXR)
{
  for (xiiUInt32 uiSide : {0, 1})
  {
    const XrHandEXT            uiHand = uiSide == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
    XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    createInfo.hand = uiHand;
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrCreateHandTrackerEXT(pOpenXR->m_session, &createInfo, &m_HandTracker[uiSide]));

    m_Locations[uiSide].type           = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
    m_Locations[uiSide].next           = &m_Velocities;
    m_Locations[uiSide].jointCount     = XR_HAND_JOINT_COUNT_EXT;
    m_Locations[uiSide].jointLocations = m_JointLocations[uiSide];

    m_Velocities[uiSide].type            = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
    m_Velocities[uiSide].jointCount      = XR_HAND_JOINT_COUNT_EXT;
    m_Velocities[uiSide].jointVelocities = m_JointVelocities[uiSide];

    m_JointData[uiSide].SetCount(XR_HAND_JOINT_LITTLE_TIP_EXT + 1);
    for (xiiUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
    {
      m_JointData[uiSide][i].m_Bone.m_Transform.SetIdentity();
    }
  }

  // Map hand parts to hand joints
  m_HandParts[xiiXRHandPart::Palm].PushBack(XR_HAND_JOINT_PALM_EXT);
  m_HandParts[xiiXRHandPart::Palm].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Wrist].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_TIP_EXT);
  m_HandParts[xiiXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_DISTAL_EXT);
  m_HandParts[xiiXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_PROXIMAL_EXT);
  m_HandParts[xiiXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_METACARPAL_EXT);
  m_HandParts[xiiXRHandPart::Thumb].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_TIP_EXT);
  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_DISTAL_EXT);
  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT);
  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_PROXIMAL_EXT);
  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_METACARPAL_EXT);
  m_HandParts[xiiXRHandPart::Index].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_TIP_EXT);
  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_DISTAL_EXT);
  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT);
  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT);
  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_METACARPAL_EXT);
  m_HandParts[xiiXRHandPart::Middle].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_TIP_EXT);
  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_DISTAL_EXT);
  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_INTERMEDIATE_EXT);
  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_PROXIMAL_EXT);
  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_METACARPAL_EXT);
  m_HandParts[xiiXRHandPart::Ring].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_TIP_EXT);
  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_DISTAL_EXT);
  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT);
  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_PROXIMAL_EXT);
  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_METACARPAL_EXT);
  m_HandParts[xiiXRHandPart::Little].PushBack(XR_HAND_JOINT_WRIST_EXT);
}

xiiOpenXRHandTracking::~xiiOpenXRHandTracking()
{
  for (xiiUInt32 uiSide : {0, 1})
  {
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroyHandTrackerEXT(m_HandTracker[uiSide]));
  }
}

xiiXRHandTrackingInterface::HandPartTrackingState xiiOpenXRHandTracking::TryGetBoneTransforms(
  xiiEnum<xiiXRHand>              hand,
  xiiEnum<xiiXRHandPart>          handPart,
  xiiEnum<xiiXRTransformSpace>    space,
  xiiDynamicArray<xiiXRHandBone>& out_bones)
{
  XII_ASSERT_DEV(handPart <= xiiXRHandPart::Little, "Invalid hand part.");
  out_bones.Clear();

  for (xiiUInt32 uiJointIndex : m_HandParts[handPart])
  {
    const JointData& jointData = m_JointData[hand][uiJointIndex];
    if (!jointData.m_bValid)
      return xiiXRHandTrackingInterface::HandPartTrackingState::Untracked;

    out_bones.PushBack(jointData.m_Bone);
  }

  if (space == xiiXRTransformSpace::Global)
  {
    const xiiWorld* pWorld = m_pOpenXR->m_pWorld;
    if (const xiiStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<xiiStageSpaceComponentManager>())
    {
      if (const xiiStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        const xiiTransform globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
        for (xiiXRHandBone& bone : out_bones)
        {
          xiiTransform local = bone.m_Transform;
          bone.m_Transform.SetGlobalTransform(globalStageTransform, local);
        }
      }
    }
  }
  return xiiXRHandTrackingInterface::HandPartTrackingState::Tracked;
}

void xiiOpenXRHandTracking::UpdateJointTransforms()
{
  XII_PROFILE_SCOPE("UpdateJointTransforms");
  const XrTime              time = m_pOpenXR->m_frameState.predictedDisplayTime;
  XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
  locateInfo.baseSpace = m_pOpenXR->GetBaseSpace();
  locateInfo.time      = time;

  for (xiiUInt32 uiSide : {0, 1})
  {
    if (m_pOpenXR->m_extensions.pfn_xrLocateHandJointsEXT(m_HandTracker[uiSide], &locateInfo, &m_Locations[uiSide]) != XrResult::XR_SUCCESS)
      m_Locations[uiSide].isActive = false;

    if (m_Locations[uiSide].isActive)
    {
      for (xiiUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        const XrHandJointLocationEXT& spaceLocation = m_JointLocations[uiSide][i];
        if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
        {
          m_JointData[uiSide][i].m_bValid                       = true;
          m_JointData[uiSide][i].m_Bone.m_fRadius               = spaceLocation.radius;
          m_JointData[uiSide][i].m_Bone.m_Transform.m_vPosition = xiiOpenXR::ConvertPosition(spaceLocation.pose.position);
          m_JointData[uiSide][i].m_Bone.m_Transform.m_qRotation = xiiOpenXR::ConvertOrientation(spaceLocation.pose.orientation);
        }
        else
        {
          m_JointData[uiSide][i].m_bValid = false;
        }
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        m_JointData[uiSide][i].m_bValid = false;
      }
    }
  }
}

XII_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRHandTracking);
