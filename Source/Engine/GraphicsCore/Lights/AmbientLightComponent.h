#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct xiiMsgUpdateLocalBounds;

using xiiAmbientLightComponentManager = xiiSettingsComponentManager<class xiiAmbientLightComponent>;

class XII_RENDERERCORE_DLL xiiAmbientLightComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAmbientLightComponent, xiiSettingsComponent, xiiAmbientLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiAmbientLightComponent

public:
  xiiAmbientLightComponent();
  ~xiiAmbientLightComponent();

  void            SetTopColor(xiiColorGammaUB color); // [ property ]
  xiiColorGammaUB GetTopColor() const;                // [ property ]

  void            SetBottomColor(xiiColorGammaUB color); // [ property ]
  xiiColorGammaUB GetBottomColor() const;                // [ property ]

  void  SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;           // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void UpdateSkyIrradiance();

  xiiColorGammaUB m_TopColor    = xiiColor(0.2f, 0.2f, 0.3f);
  xiiColorGammaUB m_BottomColor = xiiColor(0.1f, 0.1f, 0.15f);
  float           m_fIntensity  = 1.0f;
};
