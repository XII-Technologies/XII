/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiDirectionalLightComponentManager = xiiComponentManager<class xiiDirectionalLightComponent, xiiBlockStorageType::Compact>;

/// The render data object for directional lights.
class XII_GRAPHICSCORE_DLL xiiDirectionalLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalLightRenderData, xiiLightRenderData);

public:
  xiiVec3 m_vDirection;
};

/// A directional light component. This represents a light source that is infinitely far away and shines in a specific direction, like the sun.
///
/// The direction of the light is determined by the forward direction of the game object transform. The position of the game object has no effect on the lighting.
class XII_GRAPHICSCORE_DLL xiiDirectionalLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDirectionalLightComponent, xiiLightComponent, xiiDirectionalLightComponentManager);

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
  // xiiDirectionalLightComponent

public:
  xiiDirectionalLightComponent();
  ~xiiDirectionalLightComponent();

  /// Angular diameter of the emitter disc (the "sun disc") as seen from the ground.
  ///
  /// A non-zero value produces softer specular highlights via representative-point shading. Has no effect on attenuation since directional lights are treated as infinitely far away.
  /// Reference values: sun ≈ 0.53°, full moon ≈ 0.52°. Values above a few degrees are physically implausible but may be used for stylised looks.
  void     SetSourceAngle(xiiAngle sourceAngle); // [ property ]
  xiiAngle GetSourceAngle() const;               // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

protected:
  xiiAngle m_SourceAngle = xiiAngle::MakeFromDegree(0.0f);
};

/// Visualizer attribute for the angular size (source angle) of a directional light.
///
/// Shows a sphere as an intuitive size reference, its world-space size is proportional to the configured angle.
class XII_GRAPHICSCORE_DLL xiiDirectionalLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiDirectionalLightVisualizerAttribute();
  xiiDirectionalLightVisualizerAttribute(xiiStringView sAngleProperty, xiiStringView sColorProperty);

  const xiiUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty2; }
};
