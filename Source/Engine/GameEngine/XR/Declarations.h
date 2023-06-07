#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>

struct xiiHMDInfo
{
  xiiString  m_sDeviceName;
  xiiString  m_sDeviceDriver;
  xiiSizeU32 m_vEyeRenderTargetSize;
};

/// \brief Defines the stage space used for the XR experience.
///
/// This value is set by the xiiStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct xiiXRStageSpace
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRStageSpace);

struct xiiXRTransformSpace
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    Local,  ///< Sets the local transform to the pose in stage space. Use if owner is direct child of xiiStageSpaceComponent.
    Global, ///< Uses the global transform of the device in world space.
    Default = Local,
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRTransformSpace);

struct xiiXRDeviceType
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    HMD,
    LeftController,
    RightController,
    DeviceID0,
    DeviceID1,
    DeviceID2,
    DeviceID3,
    DeviceID4,
    DeviceID5,
    DeviceID6,
    DeviceID7,
    DeviceID8,
    DeviceID9,
    DeviceID10,
    DeviceID11,
    DeviceID12,
    DeviceID13,
    DeviceID14,
    DeviceID15,
    Default = HMD,
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRDeviceType);

using xiiXRDeviceID = xiiInt8;

/// \brief A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the xiiStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct XII_GAMEENGINE_DLL xiiXRDeviceState
{
  xiiXRDeviceState();

  xiiVec3 m_vGripPosition;
  xiiQuat m_qGripRotation;

  xiiVec3 m_vAimPosition;
  xiiQuat m_qAimRotation;

  xiiEnum<xiiXRDeviceType> m_Type;
  bool                     m_bGripPoseIsValid   = false;
  bool                     m_bAimPoseIsValid    = false;
  bool                     m_bDeviceIsConnected = false;
};

/// \brief Defines features the given device supports.
struct xiiXRDeviceFeatures
{
  using StorageType = xiiUInt32;
  enum Enum : xiiUInt32
  {
    None                      = 0,
    Trigger                   = XII_BIT(0),  ///< Float input. If fully pressed, will also trigger 'Select'.
    Select                    = XII_BIT(1),  ///< Bool input.
    Menu                      = XII_BIT(2),  ///< Bool input.
    Squexiie                  = XII_BIT(3),  ///< Bool input.
    PrimaryAnalogStick        = XII_BIT(4),  ///< 2D axis input.
    PrimaryAnalogStickClick   = XII_BIT(5),  ///< Bool input.
    PrimaryAnalogStickTouch   = XII_BIT(6),  ///< Bool input.
    SecondaryAnalogStick      = XII_BIT(7),  ///< 2D axis input.
    SecondaryAnalogStickClick = XII_BIT(8),  ///< Bool input.
    SecondaryAnalogStickTouch = XII_BIT(9),  ///< Bool input.
    GripPose                  = XII_BIT(10), ///< 3D Pose input.
    AimPose                   = XII_BIT(11), ///< 3D Pose input.
    Default                   = None
  };

  struct Bits
  {
    StorageType Trigger : 1;
    StorageType Select : 1;
    StorageType Menu : 1;
    StorageType Squexiie : 1;
    StorageType PrimaryAnalogStick : 1;
    StorageType PrimaryAnalogStickClick : 1;
    StorageType PrimaryAnalogStickTouch : 1;
    StorageType SecondaryAnalogStick : 1;
    StorageType SecondaryAnalogStickClick : 1;
    StorageType SecondaryAnalogStickTouch : 1;
    StorageType GripPose : 1;
    StorageType AimPose : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiXRDeviceFeatures);


struct xiiXRDeviceEventData
{
  enum class Type : xiiUInt8
  {
    DeviceAdded,
    DeviceRemoved,
  };

  Type          m_Type;
  xiiXRDeviceID uiDeviceID = 0;
};

using xiiXRDeviceEvent = xiiEvent<const xiiXRDeviceEventData&>;
