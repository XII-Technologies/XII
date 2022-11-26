#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltSliderConstraintComponentManager = xiiComponentManager<class xiiJoltSliderConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltSliderConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltSliderConstraintComponent, xiiJoltConstraintComponent, xiiJoltSliderConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltSliderConstraintComponent

public:
  xiiJoltSliderConstraintComponent();
  ~xiiJoltSliderConstraintComponent();

  void                             SetLimitMode(xiiJoltConstraintLimitMode::Enum mode); // [ property ]
  xiiJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; }         // [ property ]

  void  SetLowerLimitDistance(float f);                                 // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  void  SetUpperLimitDistance(float f);                                 // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  void  SetFriction(float f);                       // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void                             SetDriveMode(xiiJoltConstraintDriveMode::Enum mode); // [ property ]
  xiiJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; }         // [ property ]

  void  SetDriveTargetValue(float f);                               // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  void  SetDriveStrength(float f);                            // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;

protected:
  xiiEnum<xiiJoltConstraintLimitMode> m_LimitMode;
  float                               m_fLowerLimitDistance = 0;
  float                               m_fUpperLimitDistance = 0;
  float                               m_fFriction           = 0;

  xiiEnum<xiiJoltConstraintDriveMode> m_DriveMode;
  float                               m_fDriveTargetValue;
  float                               m_fDriveStrength = 0; // 0 means maximum strength
};
