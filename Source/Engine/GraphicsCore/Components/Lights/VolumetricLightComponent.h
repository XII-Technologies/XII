#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiVolumetricLightComponentManager = xiiComponentManager<class xiiVolumetricLightComponent, xiiBlockStorageType::Compact>;

/// \brief Render data for volumetric (participating-media) lights.
class XII_GRAPHICSCORE_DLL xiiVolumetricLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumetricLightRenderData, xiiLightRenderData);

public:
  float m_fScattering = 0.1f;  ///< Scattering coefficient σ_s.
  float m_fAbsorption = 0.02f; ///< Absorption coefficient σ_a (extinction = σ_s + σ_a).
  float m_fAnisotropy = 0.0f;  ///< Henyey-Greenstein phase function g ([-1,1]).
};

/// \brief Marks a light as a volumetric/participating-media source.
///
/// Volumetric lights drive in-scattering computation for fog, god-rays, and haze. The participating
/// media coefficients are passed directly to the volumetric lighting pass alongside the base light
/// colour and intensity from xiiLightComponent.
class XII_GRAPHICSCORE_DLL xiiVolumetricLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumetricLightComponent, xiiLightComponent, xiiVolumetricLightComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiVolumetricLightComponent();
  ~xiiVolumetricLightComponent();

  void  SetScattering(float f);                         // [ property ]
  float GetScattering() const { return m_fScattering; } // [ property ]

  void  SetAbsorption(float f);                         // [ property ]
  float GetAbsorption() const { return m_fAbsorption; } // [ property ]

  void  SetAnisotropy(float f);                         // [ property ]
  float GetAnisotropy() const { return m_fAnisotropy; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float m_fScattering = 0.1f;
  float m_fAbsorption = 0.02f;
  float m_fAnisotropy = 0.0f;
};
