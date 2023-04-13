#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltConeConstraintComponentManager = xiiComponentManager<class xiiJoltConeConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltConeConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltConeConstraintComponent, xiiJoltConstraintComponent, xiiJoltConeConstraintComponentManager);

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
  // xiiJoltConeConstraintComponent

public:
  xiiJoltConeConstraintComponent();
  ~xiiJoltConeConstraintComponent();

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  void     SetConeAngle(xiiAngle f);                    // [ property ]
  xiiAngle GetConeAngle() const { return m_ConeAngle; } // [ property ]

protected:
  xiiAngle m_ConeAngle;
};
