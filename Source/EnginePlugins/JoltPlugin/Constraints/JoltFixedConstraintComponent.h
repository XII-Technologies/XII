#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltFixedConstraintComponentManager = xiiComponentManager<class xiiJoltFixedConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltFixedConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltFixedConstraintComponent, xiiJoltConstraintComponent, xiiJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltFixedConstraintComponent

public:
  xiiJoltFixedConstraintComponent();
  ~xiiJoltFixedConstraintComponent();

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;
};
