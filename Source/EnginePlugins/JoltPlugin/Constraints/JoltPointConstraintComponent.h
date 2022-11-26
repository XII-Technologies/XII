#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using xiiJoltPointConstraintComponentManager = xiiComponentManager<class xiiJoltPointConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltPointConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltPointConstraintComponent, xiiJoltConstraintComponent, xiiJoltPointConstraintComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltPointConstraintComponent

public:
  xiiJoltPointConstraintComponent();
  ~xiiJoltPointConstraintComponent();
};
