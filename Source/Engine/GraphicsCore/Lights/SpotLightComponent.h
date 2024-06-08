#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lights/LightComponent.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

using xiiSpotLightComponentManager = xiiComponentManager<class xiiSpotLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class XII_GRAPHICSCORE_DLL xiiSpotLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotLightRenderData, xiiLightRenderData);

public:
  float                      m_fRange;
  xiiAngle                   m_InnerSpotAngle;
  xiiAngle                   m_OuterSpotAngle;
  xiiTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief The standard spot light component.
/// This component represents spot lights with various properties (e.g. a projected texture, range, spot angle, etc.)
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

  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  float GetEffectiveRange() const;

  void     SetInnerSpotAngle(xiiAngle spotAngle); // [ property ]
  xiiAngle GetInnerSpotAngle() const;             // [ property ]

  void     SetOuterSpotAngle(xiiAngle spotAngle); // [ property ]
  xiiAngle GetOuterSpotAngle() const;             // [ property ]

  void        SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;             // [ property ]

  void                              SetProjectedTexture(const xiiTexture2DResourceHandle& hProjectedTexture);
  const xiiTexture2DResourceHandle& GetProjectedTexture() const;

protected:
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  xiiBoundingSphere CalculateBoundingSphere(const xiiTransform& t, float fRange) const;

  float m_fRange          = 0.0f;
  float m_fEffectiveRange = 0.0f;

  xiiAngle m_InnerSpotAngle = xiiAngle::MakeFromDegree(15.0f);
  xiiAngle m_OuterSpotAngle = xiiAngle::MakeFromDegree(30.0f);

  xiiTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for spot lights
class XII_GRAPHICSCORE_DLL xiiSpotLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpotLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiSpotLightVisualizerAttribute();
  xiiSpotLightVisualizerAttribute(const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const xiiUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
