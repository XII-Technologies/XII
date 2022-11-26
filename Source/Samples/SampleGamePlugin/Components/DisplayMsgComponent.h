#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct xiiMsgSetText;
struct xiiMsgSetColor;

// BEGIN-DOCS-CODE-SNIPPET: component-manager-simple
using DisplayMsgComponentManager = xiiComponentManagerSimple<class DisplayMsgComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::FreeList>;
// END-DOCS-CODE-SNIPPET

class XII_SAMPLEGAMEPLUGIN_DLL DisplayMsgComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(DisplayMsgComponent, xiiComponent, DisplayMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DisplayMsgComponent

public:
  DisplayMsgComponent();
  ~DisplayMsgComponent();

private:
  void Update();

  void OnSetText(xiiMsgSetText& msg);   // [ msg handler ]
  void OnSetColor(xiiMsgSetColor& msg); // [ msg handler ]

  xiiString m_sCurrentText;
  xiiColor  m_TextColor = xiiColor::Yellow;
};
