#pragma once

#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgSetColor;

/// \brief Base class for light render data objects.
class XII_GRAPHICSCORE_DLL xiiLightRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightRenderData, xiiRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  xiiColor  m_LightColor;
  float     m_fIntensity;
  xiiUInt32 m_uiShadowDataOffset;
};

/// \brief Base class for all xii light components containing shared properties
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

  void  SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;           // [ property ]

  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  void  SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;              // [ property ]

  void  SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;            // [ property ]

  void  SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;            // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg); // [ msg handler ]

  static float CalculateEffectiveRange(float fRange, float fIntensity);
  static float CalculateScreenSpaceSize(const xiiBoundingSphere& sphere, const xiiCamera& camera);

protected:
  xiiColorGammaUB m_LightColor    = xiiColor::White;
  float           m_fIntensity    = 10.0f;
  float           m_fPenumbraSize = 0.1f;
  float           m_fSlopeBias    = 0.25f;
  float           m_fConstantBias = 0.1f;
  bool            m_bCastShadows  = false;
};
