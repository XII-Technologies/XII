#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct xiiMsgExtractRenderData;

typedef xiiComponentManager<class xiiDebugTextComponent, xiiBlockStorageType::Compact> xiiDebugTextComponentManager;

/// \brief This component prints debug text at the owner object's position.
class XII_RENDERERCORE_DLL xiiDebugTextComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDebugTextComponent, xiiComponent, xiiDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiDebugTextComponent
public:
  xiiDebugTextComponent();
  ~xiiDebugTextComponent();

  xiiString       m_sText; // [ property ]
  xiiColorGammaUB m_Color; // [ property ]

  float m_fValue0; // [ property ]
  float m_fValue1; // [ property ]
  float m_fValue2; // [ property ]
  float m_fValue3; // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const; // [ msg handler ]
};
