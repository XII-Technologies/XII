/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

class xiiCamera;
struct xiiMsgSetColor;
struct xiiMsgExtractRenderData;

class XII_GRAPHICSCORE_DLL xiiLightRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightRenderData, xiiRenderData);

public:
  xiiColorLinearUB m_LightColor;
  xiiUInt32        m_uiTemperature;
  float            m_fIntensity;
  float            m_fRadius;
  bool             m_bCastShadows;
};

/// Base class for light components.
class XII_GRAPHICSCORE_DLL xiiLightComponent : public xiiRenderComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiLightComponent, xiiRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLightComponent

public:
  xiiLightComponent();
  ~xiiLightComponent();

  void            SetLightColor(xiiColorGammaUB lightColor); // [ property ]
  xiiColorGammaUB GetLightColor() const;                     // [ property ]

  void      SetTemperature(xiiUInt32 uiTemperature); // [ property ]
  xiiUInt32 GetTemperature() const;                  // [ property ]

  void  SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;           // [ property ]

  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg); // [ msg handler ]

  /// Calculates how far a light source would shine given the specified range and intensity.
  ///
  /// If fRange is zero, the range needed for the given intensity is returned.
  /// Otherwise the smaller value of that and fRange is returned.
  static float CalculateEffectiveRange(float fRange, float fIntensity);

  /// Calculates how large on screen (relative height) the light source would be.
  static float CalculateScreenSpaceSize(const xiiBoundingSphere& sphere, const xiiCamera& camera);

protected:
  xiiColorGammaUB m_LightColor    = xiiColor::White;
  xiiUInt32       m_uiTemperature = 6550;
  float           m_fIntensity    = 1.0f;
  bool            m_bCastShadows  = false;
};
