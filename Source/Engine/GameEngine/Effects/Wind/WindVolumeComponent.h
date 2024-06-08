#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgComponentInternalTrigger;
struct xiiMsgDeleteGameObject;

class XII_GAMEENGINE_DLL xiiWindVolumeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiWindVolumeComponent, xiiComponent);

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
  // xiiWindVolumeComponent

public:
  xiiWindVolumeComponent();
  ~xiiWindVolumeComponent();

  static xiiSpatialData::Category SpatialDataCategory;

  xiiTime                  m_BurstDuration;             // [ property ]
  xiiEnum<xiiWindStrength> m_Strength;                  // [ property ]
  bool                     m_bReverseDirection = false; // [ property ]

  xiiSimdVec4f ComputeForceAtGlobalPosition(const xiiSimdVec4f& vGlobalPos) const;

  virtual xiiSimdVec4f ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const = 0;

  xiiEnum<xiiOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);

  float GetWindInMetersPerSecond() const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using xiiWindVolumeSphereComponentManager = xiiComponentManager<class xiiWindVolumeSphereComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiWindVolumeSphereComponent : public xiiWindVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiWindVolumeSphereComponent, xiiWindVolumeComponent, xiiWindVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiWindVolumeSphereComponent

public:
  xiiWindVolumeSphereComponent();
  ~xiiWindVolumeSphereComponent();

  virtual xiiSimdVec4f ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void  SetRadius(float fVal);                  // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);

  float        m_fRadius = 1.0f;
  xiiSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct xiiWindVolumeCylinderMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Directional,
    Vortex,

    Default = Directional
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiWindVolumeCylinderMode);

using xiiWindVolumeCylinderComponentManager = xiiComponentManager<class xiiWindVolumeCylinderComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiWindVolumeCylinderComponent : public xiiWindVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiWindVolumeCylinderComponent, xiiWindVolumeComponent, xiiWindVolumeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiWindVolumeCylinderComponent

public:
  xiiWindVolumeCylinderComponent();
  ~xiiWindVolumeCylinderComponent();

  virtual xiiSimdVec4f ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void  SetRadius(float fVal);                  // [ property ]

  float GetLength() const { return m_fLength; } // [ property ]
  void  SetLength(float fVal);                  // [ property ]

  xiiEnum<xiiWindVolumeCylinderMode> m_Mode; // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);

  float        m_fRadius = 1.0f;
  float        m_fLength = 5.0f;
  xiiSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using xiiWindVolumeConeComponentManager = xiiComponentManager<class xiiWindVolumeConeComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiWindVolumeConeComponent : public xiiWindVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiWindVolumeConeComponent, xiiWindVolumeComponent, xiiWindVolumeConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiWindVolumeCylinderComponent

public:
  xiiWindVolumeConeComponent();
  ~xiiWindVolumeConeComponent();

  virtual xiiSimdVec4f ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const override;

  float GetLength() const { return m_fLength; } // [ property ]
  void  SetLength(float fVal);                  // [ property ]

  xiiAngle GetAngle() const { return m_Angle; } // [ property ]
  void     SetAngle(xiiAngle val);              // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);

  float    m_fLength = 1.0f;
  xiiAngle m_Angle   = xiiAngle::MakeFromDegree(45);
};
