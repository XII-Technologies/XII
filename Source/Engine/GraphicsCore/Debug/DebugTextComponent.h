#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct xiiMsgExtractRenderData;

using xiiDebugTextComponentManager = xiiComponentManager<class xiiDebugTextComponent, xiiBlockStorageType::Compact>;

/// \brief This component prints debug text at the owner object's position.
class XII_GRAPHICSCORE_DLL xiiDebugTextComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDebugTextComponent, xiiComponent, xiiDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiDebugTextComponent
public:
  xiiDebugTextComponent();
  ~xiiDebugTextComponent();

  xiiString       m_sText; // [ property ]
  xiiColorGammaUB m_Color; // [ property ]

  float m_fValue0 = 0.0f; // [ property ]
  float m_fValue1 = 0.0f; // [ property ]
  float m_fValue2 = 0.0f; // [ property ]
  float m_fValue3 = 0.0f; // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const; // [ msg handler ]
};
