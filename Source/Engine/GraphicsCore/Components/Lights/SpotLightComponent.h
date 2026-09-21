/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiSpotLightComponentManager = xiiComponentManager<class xiiSpotLightComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiSpotLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotLightRenderData, xiiLightRenderData);

public:
  xiiQuat  m_qGlobalRotation;
  float    m_fRange;
  float    m_fShadowFadeOutRange;
  xiiAngle m_InnerSpotAngle;
  xiiAngle m_OuterSpotAngle;
};

/// A spot light component. This represents a light source that emits light in a cone shape, like a flashlight or a spotlight on a stage.
class XII_GRAPHICSCORE_DLL xiiSpotLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpotLightComponent, xiiLightComponent, xiiSpotLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  /////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  /////////////////////////////////////////////////////////////////////////
  // xiiSpotLightComponent

public:
  xiiSpotLightComponent();
  ~xiiSpotLightComponent();

  /// Sets the radius (or length of the cone) of the lightsource. If zero, it is automatically determined from the intensity.
  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  /// Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  /// Radius of the emitter disc at the spot light's origin. A non-zero value produces softer specular highlights and area-light style shading. Does not affect attenuation.
  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  /// Sets the radius that is used to determine when to fade out shadows. If zero the radius of the lightsource is used.
  void  SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;       // [ property ]

  /// Sets the inner angle where the spotlight has equal brightness.
  void     SetInnerSpotAngle(xiiAngle spotAngle); // [ property ]
  xiiAngle GetInnerSpotAngle() const;             // [ property ]

  /// Sets the outer angle of the spotlight's cone. The light will fade out between the inner and outer angle.
  void     SetOuterSpotAngle(xiiAngle spotAngle); // [ property ]
  xiiAngle GetOuterSpotAngle() const;             // [ property ]

protected:
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiBoundingSphere CalculateBoundingSphere(const xiiTransform& transform, float fRange) const;

protected:
  float    m_fRange              = 0.0f;
  float    m_fEffectiveRange     = 0.0f;
  float    m_fShadowFadeOutRange = 0.0f;
  float    m_fRadius             = 0.0f;
  xiiAngle m_InnerSpotAngle      = xiiAngle::MakeFromDegree(15.0f);
  xiiAngle m_OuterSpotAngle      = xiiAngle::MakeFromDegree(30.0f);
};

/// Visualizer attribute for spot lights. Also renders a cone when the range is non-zero.
class XII_GRAPHICSCORE_DLL xiiSpotLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiSpotLightVisualizerAttribute();
  xiiSpotLightVisualizerAttribute(xiiStringView sAngleProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty, xiiStringView sRadiusProperty = nullptr);

  const xiiUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty4; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty5; }
};
