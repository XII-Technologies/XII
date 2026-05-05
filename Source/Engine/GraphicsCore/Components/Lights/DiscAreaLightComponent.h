/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiDiscAreaLightComponentManager = xiiComponentManager<class xiiDiscAreaLightComponent, xiiBlockStorageType::Compact>;

/// \brief Render data for circular area lights.
class XII_GRAPHICSCORE_DLL xiiDiscAreaLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDiscAreaLightRenderData, xiiLightRenderData);

public:
  float   m_fRange;
  float   m_fShadowFadeOutRange;
  xiiQuat m_qGlobalRotation;
};

/// \brief Circular area light that emits from a disc whose normal points along negative X.
class XII_GRAPHICSCORE_DLL xiiDiscAreaLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDiscAreaLightComponent, xiiLightComponent, xiiDiscAreaLightComponentManager);

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
  // xiiDiscAreaLightComponent

public:
  xiiDiscAreaLightComponent();
  ~xiiDiscAreaLightComponent();

  /// \brief Sets the radius of the emitting disc.
  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  /// \brief Sets the influence range. If zero, the range is derived from intensity.
  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  /// \brief Returns the final influence range.
  float GetEffectiveRange() const;

  /// \brief Sets the range used to fade out shadows. If zero, the light range is used.
  void  SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;       // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

protected:
  float m_fRadius             = 0.5f;
  float m_fRange              = 0.0f;
  float m_fEffectiveRange     = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;
};

/// \brief Visualizer attribute for disc area lights.
class XII_GRAPHICSCORE_DLL xiiDiscAreaLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDiscAreaLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiDiscAreaLightVisualizerAttribute();
  xiiDiscAreaLightVisualizerAttribute(xiiStringView sRadiusProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty);

  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
