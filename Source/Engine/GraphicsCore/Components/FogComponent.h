#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgUpdateLocalBounds;

using xiiFogComponentManager = xiiSettingsComponentManager<class xiiFogComponent>;

/// \brief The render data object for ambient light.
class XII_RENDERERCORE_DLL xiiFogRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFogRenderData, xiiRenderData);

public:
  xiiColor m_Color;
  float    m_fDensity;
  float    m_fHeightFalloff;
  float    m_fInvSkyDistance;
};

class XII_RENDERERCORE_DLL xiiFogComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiFogComponent, xiiSettingsComponent, xiiFogComponentManager);

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
  // xiiFogComponent

public:
  xiiFogComponent();
  ~xiiFogComponent();

  void     SetColor(xiiColor color); // [ property ]
  xiiColor GetColor() const;         // [ property ]

  void  SetDensity(float fDensity); // [ property ]
  float GetDensity() const;         // [ property ]

  void  SetHeightFalloff(float fHeightFalloff); // [ property ]
  float GetHeightFalloff() const;               // [ property ]

  void SetModulateWithSkyColor(bool bModulate); // [ property ]
  bool GetModulateWithSkyColor() const;         // [ property ]

  void  SetSkyDistance(float fDistance); // [ property ]
  float GetSkyDistance() const;          // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiColor m_Color                 = xiiColor(0.2f, 0.2f, 0.3f);
  float    m_fDensity              = 1.0f;
  float    m_fHeightFalloff        = 10.0f;
  float    m_fSkyDistance          = 1000.0f;
  bool     m_bModulateWithSkyColor = false;
};
