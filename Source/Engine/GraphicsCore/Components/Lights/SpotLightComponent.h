#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>
#include <GraphicsCore/Declarations.h>

using xiiSpotLightComponentManager = xiiComponentManager<class xiiSpotLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class XII_GRAPHICSCORE_DLL xiiSpotLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotLightRenderData, xiiLightRenderData);

public:
  xiiAngle                   m_InnerConeAngle; ///< Full-intensity cone half-angle.
  xiiAngle                   m_OuterConeAngle; ///< Zero-intensity cone half-angle.
  float                      m_fRange = 10.0f; ///< Maximum reach of the light.
  xiiTexture2DResourceHandle m_hIESProfile;    ///< Optional IES luminous intensity profile.
};

/// \brief A cone-shaped spot light emitting from a point in a specified direction.
///
/// The angle between the forward axis and a ray determines intensity: full between the inner cone
/// and zero at the outer cone (smooth penumbra). An optional IES profile modulates distribution.
class XII_GRAPHICSCORE_DLL xiiSpotLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpotLightComponent, xiiLightComponent, xiiSpotLightComponentManager);

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
  // xiiSpotLightComponent

public:
  xiiSpotLightComponent();
  ~xiiSpotLightComponent();

  void     SetInnerConeAngle(xiiAngle angle);                     // [ property ]
  xiiAngle GetInnerConeAngle() const { return m_InnerConeAngle; } // [ property ]

  void     SetOuterConeAngle(xiiAngle angle);                     // [ property ]
  xiiAngle GetOuterConeAngle() const { return m_OuterConeAngle; } // [ property ]

  void  SetRange(float fRange);               // [ property ]
  float GetRange() const { return m_fRange; } // [ property ]

  void          SetIESProfileFile(xiiStringView sFile); // [ property ]
  xiiStringView GetIESProfileFile() const;              // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiAngle                   m_InnerConeAngle = xiiAngle::MakeFromDegree(15.0f);
  xiiAngle                   m_OuterConeAngle = xiiAngle::MakeFromDegree(30.0f);
  float                      m_fRange         = 10.0f;
  xiiTexture2DResourceHandle m_hIESProfile;
};
