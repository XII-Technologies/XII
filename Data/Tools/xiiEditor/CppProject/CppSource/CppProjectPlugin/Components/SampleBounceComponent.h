#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>

using SampleBounceComponentManager = xiiComponentManagerSimple<class SampleBounceComponent, xiiComponentUpdateType::WhenSimulating>;

class SampleBounceComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(SampleBounceComponent, xiiComponent, SampleBounceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SampleBounceComponent

public:
  SampleBounceComponent();
  ~SampleBounceComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;             // [ property ]
  xiiAngle m_Speed = xiiAngle::Degree(90); // [ property ]
};
