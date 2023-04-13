#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltSwingTwistConstraintComponentManager = xiiComponentManager<class xiiJoltSwingTwistConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltSwingTwistConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltSwingTwistConstraintComponent, xiiJoltConstraintComponent, xiiJoltSwingTwistConstraintComponentManager);

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
  // xiiJoltSwingTwistConstraintComponent

public:
  xiiJoltSwingTwistConstraintComponent();
  ~xiiJoltSwingTwistConstraintComponent();

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  void     SetSwingLimitY(xiiAngle f);                      // [ property ]
  xiiAngle GetSwingLimitY() const { return m_SwingLimitY; } // [ property ]

  void     SetSwingLimitZ(xiiAngle f);                      // [ property ]
  xiiAngle GetSwingLimitZ() const { return m_SwingLimitZ; } // [ property ]

  void  SetFriction(float f);                       // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void     SetLowerTwistLimit(xiiAngle f);                          // [ property ]
  xiiAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  void     SetUpperTwistLimit(xiiAngle f);                          // [ property ]
  xiiAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  //void SetTwistDriveMode(xiiJoltConstraintDriveMode::Enum mode);                          // [ property ]
  //xiiJoltConstraintDriveMode::Enum GetTwistDriveMode() const { return m_TwistDriveMode; } // [ property ]

  //void SetTwistDriveTargetValue(xiiAngle f);                                    // [ property ]
  //xiiAngle GetTwistDriveTargetValue() const { return m_TwistDriveTargetValue; } // [ property ]

  //void SetTwistDriveStrength(float f);                                  // [ property ]
  //float GetTwistDriveStrength() const { return m_fTwistDriveStrength; } // [ property ]

protected:
  xiiAngle m_SwingLimitY;
  xiiAngle m_SwingLimitZ;

  float m_fFriction = 0.0f;

  xiiAngle m_LowerTwistLimit = xiiAngle::Degree(90);
  xiiAngle m_UpperTwistLimit = xiiAngle::Degree(90);

  // not sure whether these are useful
  // maybe just expose an 'untwist' feature, with strength/frequency and drive to position 0 ?
  // driving to velocity makes no sense, since the constraint always has a lower/upper twist limit
  // probably would need a 6DOF joint for more advanced use cases
  //xiiEnum<xiiJoltConstraintDriveMode> m_TwistDriveMode;
  //xiiAngle m_TwistDriveTargetValue;
  //float m_fTwistDriveStrength = 0; // 0 means maximum strength
};
