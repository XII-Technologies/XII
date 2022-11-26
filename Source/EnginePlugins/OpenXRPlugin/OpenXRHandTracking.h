#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class xiiOpenXR;

class XII_OPENXRPLUGIN_DLL xiiOpenXRHandTracking : public xiiXRHandTrackingInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiOpenXRHandTracking, xiiXRHandTrackingInterface);

public:
  static bool IsHandTrackingSupported(xiiOpenXR* pOpenXR);

public:
  xiiOpenXRHandTracking(xiiOpenXR* pOpenXR);
  ~xiiOpenXRHandTracking();

  HandPartTrackingState TryGetBoneTransforms(
    xiiEnum<xiiXRHand>              hand,
    xiiEnum<xiiXRHandPart>          handPart,
    xiiEnum<xiiXRTransformSpace>    space,
    xiiDynamicArray<xiiXRHandBone>& out_bones) override;

  void UpdateJointTransforms();

private:
  friend class xiiOpenXR;

  struct JointData
  {
    XII_DECLARE_POD_TYPE();
    xiiXRHandBone m_Bone;
    bool          m_bValid;
  };

  xiiOpenXR*               m_pOpenXR        = nullptr;
  XrHandTrackerEXT         m_HandTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  XrHandJointLocationEXT   m_JointLocations[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT   m_JointVelocities[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT  m_Locations[2]{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
  XrHandJointVelocitiesEXT m_Velocities[2]{XR_TYPE_HAND_JOINT_VELOCITIES_EXT};

  xiiStaticArray<JointData, XR_HAND_JOINT_LITTLE_TIP_EXT + 1> m_JointData[2];
  xiiStaticArray<xiiUInt32, 6>                                m_HandParts[xiiXRHandPart::Little + 1];
};
