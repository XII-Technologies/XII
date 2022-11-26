#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-manager
using DemoComponentManager = xiiComponentManagerSimple<class DemoComponent, xiiComponentUpdateType::WhenSimulating>;
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-class
class DemoComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(DemoComponent, xiiComponent, DemoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DemoComponent

public:
  DemoComponent();
  ~DemoComponent();

private:
  void Update();

  float    m_fAmplitude = 1.0f;                 // [ property ]
  xiiAngle m_Speed      = xiiAngle::Degree(90); // [ property ]
};
// END-DOCS-CODE-SNIPPET
