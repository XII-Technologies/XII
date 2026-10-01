/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiTubeAreaLightComponentManager = xiiComponentManager<class xiiTubeAreaLightComponent, xiiBlockStorageType::Compact>;

/// Render data for a capsule-shaped tube emitter aligned with local +X.
class XII_GRAPHICSCORE_DLL xiiTubeAreaLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTubeAreaLightRenderData, xiiLightRenderData);

public:
  float   m_fRange;
  float   m_fLength;
  float   m_fShadowFadeOutRange;
  xiiQuat m_qGlobalRotation;
};

/// Finite tube/capsule Lambertian emitter authored in physical units.
class XII_GRAPHICSCORE_DLL xiiTubeAreaLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTubeAreaLightComponent, xiiLightComponent, xiiTubeAreaLightComponentManager);

public:
  xiiTubeAreaLightComponent();
  ~xiiTubeAreaLightComponent();

  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void  SetLength(float fLength); // [ property ]
  float GetLength() const;        // [ property ]

  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  float GetEffectiveRange() const;

  void  SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;       // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float GetEmittingArea() const;
  float GetMaximumProjectedArea() const;
  float GetLuminanceNits() const;
  float GetMaximumCandela() const;

  float m_fLength             = 1.0f;
  float m_fRadius             = 0.05f;
  float m_fRange              = 0.0f;
  float m_fEffectiveRange     = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;
};
