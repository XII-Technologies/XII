#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lights/LightComponent.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

using xiiPointLightComponentManager = xiiComponentManager<class xiiPointLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class XII_GRAPHICSCORE_DLL xiiPointLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointLightRenderData, xiiLightRenderData);

public:
  float                        m_fRange;
  xiiTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief The standard point light component.
/// This component represents point lights with various properties (e.g. a projected cube map, range, etc.)
class XII_GRAPHICSCORE_DLL xiiPointLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPointLightComponent, xiiLightComponent, xiiPointLightComponentManager);

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
  // xiiPointLightComponent

public:
  xiiPointLightComponent();
  ~xiiPointLightComponent();

  void  SetRange(float fRange); // [ property ]
  float GetRange() const;       // [ property ]

  float GetEffectiveRange() const;

  void          SetProjectedTextureFile(xiiStringView sFile); // [ property ]
  xiiStringView GetProjectedTextureFile() const;              // [ property ]

  void                                SetProjectedTexture(const xiiTextureCubeResourceHandle& hProjectedTexture);
  const xiiTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  float m_fRange          = 0.0f;
  float m_fEffectiveRange = 0.0f;

  xiiTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class XII_GRAPHICSCORE_DLL xiiPointLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiPointLightVisualizerAttribute();
  xiiPointLightVisualizerAttribute(xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty);

  const xiiUntrackedString& GetRangeProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetIntensityProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty3; }
};
