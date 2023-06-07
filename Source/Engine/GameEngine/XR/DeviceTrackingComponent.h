#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

struct xiiXRPoseLocation
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    Grip,
    Aim,
    Default = Grip,
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRPoseLocation);

//////////////////////////////////////////////////////////////////////////

using xiiDeviceTrackingComponentManager = class xiiDeviceTrackingComponent;

/// \brief Tracks the position of a XR device and applies it to the owner.
class XII_GAMEENGINE_DLL xiiDeviceTrackingComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDeviceTrackingComponent, xiiComponent, xiiDeviceTrackingComponentManager);

public:
  xiiDeviceTrackingComponent();
  ~xiiDeviceTrackingComponent();

  /// \brief Sets the type of device this component is going to track.
  void                     SetDeviceType(xiiEnum<xiiXRDeviceType> type);
  xiiEnum<xiiXRDeviceType> GetDeviceType() const;

  void                       SetPoseLocation(xiiEnum<xiiXRPoseLocation> poseLocation);
  xiiEnum<xiiXRPoseLocation> GetPoseLocation() const;

  /// \brief Whether to set the owner's local or global transform, see xiiXRTransformSpace.
  void                         SetTransformSpace(xiiEnum<xiiXRTransformSpace> space);
  xiiEnum<xiiXRTransformSpace> GetTransformSpace() const;

  //
  // xiiComponent Interface
  //

protected:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  void Update();

  xiiEnum<xiiXRDeviceType>     m_DeviceType;
  xiiEnum<xiiXRPoseLocation>   m_PoseLocation;
  xiiEnum<xiiXRTransformSpace> m_Space;
  bool                         m_bRotation = true;
  bool                         m_bScale    = true;
};
