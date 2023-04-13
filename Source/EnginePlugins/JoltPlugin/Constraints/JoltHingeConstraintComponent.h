#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltHingeConstraintComponentManager = xiiComponentManager<class xiiJoltHingeConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltHingeConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltHingeConstraintComponent, xiiJoltConstraintComponent, xiiJoltHingeConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltHingeConstraintComponent

public:
  xiiJoltHingeConstraintComponent();
  ~xiiJoltHingeConstraintComponent();

  void                             SetLimitMode(xiiJoltConstraintLimitMode::Enum mode); // [ property ]
  xiiJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; }         // [ property ]

  void     SetLowerLimitAngle(xiiAngle f);                     // [ property ]
  xiiAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  void     SetUpperLimitAngle(xiiAngle f);                     // [ property ]
  xiiAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  void  SetFriction(float f);                       // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void                             SetDriveMode(xiiJoltConstraintDriveMode::Enum mode); // [ property ]
  xiiJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; }         // [ property ]

  void     SetDriveTargetValue(xiiAngle f);                           // [ property ]
  xiiAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  void  SetDriveStrength(float f);                            // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

protected:
  xiiEnum<xiiJoltConstraintLimitMode> m_LimitMode;
  xiiAngle                            m_LowerLimit;
  xiiAngle                            m_UpperLimit;
  float                               m_fFriction = 0;
  xiiEnum<xiiJoltConstraintDriveMode> m_DriveMode;
  xiiAngle                            m_DriveTargetValue;
  float                               m_fDriveStrength = 0; // 0 means maximum strength
};
