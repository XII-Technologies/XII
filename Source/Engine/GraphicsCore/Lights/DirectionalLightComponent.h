#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lights/LightComponent.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

using xiiDirectionalLightComponentManager = xiiComponentManager<class xiiDirectionalLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for directional lights.
class XII_GRAPHICSCORE_DLL xiiDirectionalLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionalLightRenderData, xiiLightRenderData);

public:
};

/// \brief The standard directional light component.
/// This component represents directional lights.
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

  void      SetNumCascades(xiiUInt32 uiNumCascades); // [ property ]
  xiiUInt32 GetNumCascades() const;                  // [ property ]

  void  SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;                // [ property ]

  void  SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;              // [ property ]

  void  SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                 // [ property ]

  void  SetNearPlaneOffset(float fNearPlaneOffset); // [ property ]
  float GetNearPlaneOffset() const;                 // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiUInt32 m_uiNumCascades    = 3;
  float     m_fMinShadowRange  = 50.0f;
  float     m_fFadeOutStart    = 0.8f;
  float     m_fSplitModeWeight = 0.7f;
  float     m_fNearPlaneOffset = 100.0f;
};
