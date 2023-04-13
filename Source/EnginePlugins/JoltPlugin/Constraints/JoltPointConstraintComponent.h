#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltPointConstraintComponentManager = xiiComponentManager<class xiiJoltPointConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltPointConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltPointConstraintComponent, xiiJoltConstraintComponent, xiiJoltPointConstraintComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual bool ExceededBreakingPoint() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltPointConstraintComponent

public:
  xiiJoltPointConstraintComponent();
  ~xiiJoltPointConstraintComponent();
};
