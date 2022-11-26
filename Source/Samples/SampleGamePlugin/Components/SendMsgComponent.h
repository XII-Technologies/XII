#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct xiiMsgComponentInternalTrigger;

// This component manager does literally nothing, meaning the managed components do not need to be update, at all
// BEGIN-DOCS-CODE-SNIPPET: component-manager-trivial
using SendMsgComponentManager = xiiComponentManager<class SendMsgComponent, xiiBlockStorageType::Compact>;
// END-DOCS-CODE-SNIPPET

class XII_SAMPLEGAMEPLUGIN_DLL SendMsgComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(SendMsgComponent, xiiComponent, SendMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SendMsgComponent

public:
  SendMsgComponent();
  ~SendMsgComponent();

private:
  xiiDynamicArray<xiiString> m_TextArray; // [ property ]

  void OnSendText(xiiMsgComponentInternalTrigger& msg); // [ msg handler ]

  xiiUInt32 m_uiNextString = 0;
};
