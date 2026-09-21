/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiPointLightComponentManager = xiiComponentManager<class xiiPointLightComponent, xiiBlockStorageType::Compact>;

/// The render data object for point lights.
class XII_GRAPHICSCORE_DLL xiiPointLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointLightRenderData, xiiLightRenderData);

public:
  float   m_fRange;
  float   m_fLength;
  float   m_fShadowFadeOutRange;
  xiiQuat m_qGlobalRotation;
};

/// A point light component. This represents a light source that emits light in all directions from a single point in space, like a light bulb.
class XII_GRAPHICSCORE_DLL xiiPointLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPointLightComponent, xiiLightComponent, xiiPointLightComponentManager);

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
  // xiiPointLightComponent

public:
  xiiPointLightComponent();
  ~xiiPointLightComponent();

  /// Sets the radius of the light source. If zero, the radius is automatically determined from the intensity.
  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  /// Returns the final radius of the light source.
  float GetEffectiveRange() const;

  /// Sets the length of the tube. Zero means the light is a point light.
  void  SetLength(float fLength); // [ property ]
  float GetLength() const;        // [ property ]

  /// Radius of the tube's cross-section. Affects the size of specular highlights. Zero means the light is a point light.
  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  /// Sets the radius that is used to determine when to fade out shadows. If zero the radius of the light source is used.
  void  SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;       // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

protected:
  float m_fLength             = 0.0f;
  float m_fRadius             = 0.0f;
  float m_fRange              = 0.0f;
  float m_fEffectiveRange     = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;
};

/// Visualizer attribute for point lights. Also renders a tube (capsule) when Length or Radius is non-zero.
class XII_GRAPHICSCORE_DLL xiiPointLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiPointLightVisualizerAttribute();
  xiiPointLightVisualizerAttribute(xiiStringView sLengthProperty, xiiStringView sRadiusProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty);

  const xiiUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetRangeProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetIntensityProperty() const { return m_sProperty4; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty5; }
};
