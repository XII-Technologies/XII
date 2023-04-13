#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltDistanceConstraintComponentManager = xiiComponentManager<class xiiJoltDistanceConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltDistanceConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltDistanceConstraintComponent, xiiJoltConstraintComponent, xiiJoltDistanceConstraintComponentManager);

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
  // xiiJoltDistanceConstraintComponent

public:
  xiiJoltDistanceConstraintComponent();
  ~xiiJoltDistanceConstraintComponent();

  float GetMinDistance() const { return m_fMinDistance; } // [ property ]
  void  SetMinDistance(float value);                      // [ property ]

  float GetMaxDistance() const { return m_fMaxDistance; } // [ property ]
  void  SetMaxDistance(float value);                      // [ property ]

  void  SetFrequency(float value);                    // [ property ]
  float GetFrequency() const { return m_fFrequency; } // [ property ]

  void  SetDamping(float value);                  // [ property ]
  float GetDamping() const { return m_fDamping; } // [ property ]

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

protected:
  float m_fMinDistance = 0.0f;
  float m_fMaxDistance = 1.0f;
  float m_fFrequency   = 0.0f;
  float m_fDamping     = 0.0f;
};
