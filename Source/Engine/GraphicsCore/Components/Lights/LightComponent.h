#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>

struct xiiMsgSetColor;

/// \brief Base class for light components.
class XII_GRAPHICSCORE_DLL xiiLightComponent : public xiiRenderComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiLightComponent, xiiRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLightComponent

public:
  xiiLightComponent();
  ~xiiLightComponent();

  void      SetTemperature(xiiUInt32 uiTemperature); // [ property ]
  xiiUInt32 GetTemperature() const;                  // [ property ]

  void            SetLightColor(xiiColorGammaUB lightColor); // [ property ]
  xiiColorGammaUB GetLightColor() const;                     // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg); // [ msg handler ]

protected:
  xiiColorGammaUB m_LightColor    = xiiColor::White;
  xiiUInt32       m_uiTemperature = 6550;
};
