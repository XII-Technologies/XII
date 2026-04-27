#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiPointLightComponentManager = xiiComponentManager<class xiiPointLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class XII_GRAPHICSCORE_DLL xiiPointLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointLightRenderData, xiiLightRenderData);

public:
  float m_fAttenuationRadius   = 5.0f; ///< World-space range at which attenuation reaches zero.
  float m_fFalloffExponent     = 2.0f; ///< Exponent for the attenuation curve (2 = inverse-square).
  float m_fVolumetricIntensity = 0.0f; ///< Participating-media scatter contribution factor.
};

/// \brief An omnidirectional point light that emits radiance equally in all directions.
///
/// Attenuation follows a smooth windowed inverse-power law: the falloff reaches zero at
/// m_fAttenuationRadius. The optional volumetric intensity drives in-scattering for fog/haze passes.
class XII_GRAPHICSCORE_DLL xiiPointLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPointLightComponent, xiiLightComponent, xiiPointLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPointLightComponent

public:
  xiiPointLightComponent();
  ~xiiPointLightComponent();

  void  SetAttenuationRadius(float fRadius);                          // [ property ]
  float GetAttenuationRadius() const { return m_fAttenuationRadius; } // [ property ]

  void  SetFalloffExponent(float fExp);                           // [ property ]
  float GetFalloffExponent() const { return m_fFalloffExponent; } // [ property ]

  void  SetVolumetricIntensity(float f);                                  // [ property ]
  float GetVolumetricIntensity() const { return m_fVolumetricIntensity; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float m_fAttenuationRadius   = 5.0f;
  float m_fFalloffExponent     = 2.0f;
  float m_fVolumetricIntensity = 0.0f;
};
