/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiSphereAreaLightComponentManager = xiiComponentManager<class xiiSphereAreaLightComponent, xiiBlockStorageType::Compact>;

/// Render data for an omnidirectional spherical area emitter.
class XII_GRAPHICSCORE_DLL xiiSphereAreaLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSphereAreaLightRenderData, xiiLightRenderData);

public:
  float m_fRange;
  float m_fShadowFadeOutRange;
};

/// Finite spherical Lambertian emitter authored in physical photometric units.
class XII_GRAPHICSCORE_DLL xiiSphereAreaLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSphereAreaLightComponent, xiiLightComponent, xiiSphereAreaLightComponentManager);

public:
  xiiSphereAreaLightComponent();
  ~xiiSphereAreaLightComponent();

  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  float GetEffectiveRange() const;

  void  SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;       // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float GetLuminanceNits() const;
  float GetOnAxisCandela() const;

  float m_fRadius             = 0.25f;
  float m_fRange              = 0.0f;
  float m_fEffectiveRange     = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;
};
