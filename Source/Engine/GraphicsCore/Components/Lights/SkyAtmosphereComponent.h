#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

using xiiSkyAtmosphereComponentManager = xiiComponentManager<class xiiSkyAtmosphereComponent, xiiBlockStorageType::Compact>;

/// \brief Hillaire/Preetham atmosphere render data.
class XII_GRAPHICSCORE_DLL xiiSkyAtmosphereRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyAtmosphereRenderData, xiiRenderData);

public:
  // Rayleigh scattering
  xiiVec3 m_vRayleighScattering = xiiVec3(5.802e-6f, 13.558e-6f, 33.1e-6f); ///< σ_sr (m^-1), per wavelength.
  float   m_fRayleighAltScale   = 8000.0f;                                  ///< Rayleigh density scale height (m).

  // Mie scattering
  float m_fMieScattering = 3.996e-6f; ///< Mie scattering coefficient (m^-1).
  float m_fMieExtinction = 4.44e-6f;  ///< Mie extinction coefficient (m^-1).
  float m_fMieAnisotropy = 0.8f;      ///< Mie phase g.
  float m_fMieAltScale   = 1200.0f;   ///< Mie density scale height (m).

  // Ozone
  xiiVec3 m_vOzoneAbsorption = xiiVec3(0.65e-6f, 1.881e-6f, 0.085e-6f);
  float   m_fOzoneAltCentre  = 25000.0f; ///< Centre altitude of ozone layer (m).
  float   m_fOzoneAltWidth   = 15000.0f; ///< Half-width of ozone layer (m).

  // Geometry
  float m_fPlanetRadius     = 6360000.0f; ///< Planet radius (m).
  float m_fAtmosphereRadius = 6460000.0f; ///< Atmosphere outer radius (m).

  xiiVec3 m_vSunDirection   = xiiVec3(0, 0, 1);
  float   m_fSunIlluminance = 120000.0f; ///< Lux.
};

/// \brief Procedural physically-based sky using Hillaire's atmosphere model.
///
/// The component is always visible and submits atmosphere parameters to the sky render pass.
/// It does not own a sun light — the sun direction is taken from a co-located DirectionalLightComponent
/// or set directly via SetSunDirection().
class XII_GRAPHICSCORE_DLL xiiSkyAtmosphereComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkyAtmosphereComponent, xiiRenderComponent, xiiSkyAtmosphereComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiSkyAtmosphereComponent();
  ~xiiSkyAtmosphereComponent();

  void  SetPlanetRadius(float fMetres);                     // [ property ]
  float GetPlanetRadius() const { return m_fPlanetRadius; } // [ property ]

  void  SetAtmosphereRadius(float fMetres);                         // [ property ]
  float GetAtmosphereRadius() const { return m_fAtmosphereRadius; } // [ property ]

  void  SetMieAnisotropy(float g);                            // [ property ]
  float GetMieAnisotropy() const { return m_fMieAnisotropy; } // [ property ]

  void  SetSunIlluminance(float fLux);                          // [ property ]
  float GetSunIlluminance() const { return m_fSunIlluminance; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiVec3 m_vRayleighScattering = xiiVec3(5.802e-6f, 13.558e-6f, 33.1e-6f);
  float   m_fRayleighAltScale   = 8000.0f;
  float   m_fMieScattering      = 3.996e-6f;
  float   m_fMieExtinction      = 4.44e-6f;
  float   m_fMieAnisotropy      = 0.8f;
  float   m_fMieAltScale        = 1200.0f;
  xiiVec3 m_vOzoneAbsorption    = xiiVec3(0.65e-6f, 1.881e-6f, 0.085e-6f);
  float   m_fOzoneAltCentre     = 25000.0f;
  float   m_fOzoneAltWidth      = 15000.0f;
  float   m_fPlanetRadius       = 6360000.0f;
  float   m_fAtmosphereRadius   = 6460000.0f;
  float   m_fSunIlluminance     = 120000.0f;
};
