#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgComponentInternalTrigger;
struct xiiMsgDeleteGameObject;

/// \brief Base class for components that define volumes in which a camera shake effect shall be applied.
///
/// Derived classes implement different shape types and how the shake strength is calculated.
class XII_GAMECOMPONENTS_DLL xiiCameraShakeVolumeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiCameraShakeVolumeComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraShakeVolumeComponent

public:
  xiiCameraShakeVolumeComponent();
  ~xiiCameraShakeVolumeComponent();

  /// \brief The spatial category used to find camera shake volume components through the spatial system.
  static xiiSpatialData::Category SpatialDataCategory;

  /// \brief How long a shake burst should last. Zero for constant shaking.
  xiiTime m_BurstDuration; // [ property ]

  /// \brief How strong the shake should be at the strongest point. Typically a value between one and zero.
  float m_fStrength; // [ property ]

  /// \brief Calculates the shake strength at the given global position.
  float ComputeForceAtGlobalPosition(const xiiSimdVec4f& vGlobalPos) const;

  /// \brief Calculates the shake strength in local space of the component.
  virtual float ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const = 0;

  /// \brief In case of a burst shake, defines whether the component should delete itself afterwards.
  xiiEnum<xiiOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using xiiCameraShakeVolumeSphereComponentManager = xiiComponentManager<class xiiCameraShakeVolumeSphereComponent, xiiBlockStorageType::Compact>;

/// \brief A spherical volume in which a camera shake will be applied.
///
/// The shake strength is strongest at the center of the sphere and gradually weaker towards the sphere radius.
///
/// \see xiiCameraShakeVolumeComponent
/// \see xiiCameraShakeComponent
class XII_GAMECOMPONENTS_DLL xiiCameraShakeVolumeSphereComponent : public xiiCameraShakeVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCameraShakeVolumeSphereComponent, xiiCameraShakeVolumeComponent, xiiCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraShakeVolumeSphereComponent

public:
  xiiCameraShakeVolumeSphereComponent();
  ~xiiCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void  SetRadius(float fVal);                  // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);

  float        m_fRadius = 1.0f;
  xiiSimdFloat m_fOneDivRadius;
};
