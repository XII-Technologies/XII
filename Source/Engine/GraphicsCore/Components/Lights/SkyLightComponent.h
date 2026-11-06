/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

struct xiiMsgExtractRenderData;
using xiiSkyLightComponentManager = xiiComponentManager<class xiiSkyLightComponent, xiiBlockStorageType::Compact>;

/// Immutable source and settings for a world-wide image-based light. Only the owner's rotation affects sampling.
class XII_GRAPHICSCORE_DLL xiiSkyLightRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyLightRenderData, xiiRenderData);

public:
  xiiTextureCubeResourceHandle m_hEnvironmentMap;
  float                        m_fIntensity  = 1.0f;
  float                        m_fSaturation = 1.0f;
  xiiInt32                     m_iPriority   = 0;
};

/// Provides diffuse irradiance and roughness-filtered specular radiance outside local probe coverage.
/// The highest priority active sky wins, with component identity breaking ties deterministically.
/// The source resource shares its immutable convolution with local probes using the same cubemap.
class XII_GRAPHICSCORE_DLL xiiSkyLightComponent final : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkyLightComponent, xiiRenderComponent, xiiSkyLightComponentManager);

public:
  xiiSkyLightComponent();
  ~xiiSkyLightComponent();

  xiiBitflags<xiiRenderExtractionCategory> GetRenderExtractionCategories() const override { return xiiRenderExtractionCategory::Lights; }
  void                                     SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                                     DeserializeComponent(xiiWorldReader& inout_stream) override;
  xiiResult                                GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                                SetEnvironmentMap(const xiiTextureCubeResourceHandle& hEnvironmentMap);
  const xiiTextureCubeResourceHandle& GetEnvironmentMap() const { return m_hEnvironmentMap; }
  void                                SetIntensity(float fIntensity);
  float                               GetIntensity() const { return m_fIntensity; }
  void                                SetSaturation(float fSaturation);
  float                               GetSaturation() const { return m_fSaturation; }
  void                                SetPriority(xiiInt32 iPriority);
  xiiInt32                            GetPriority() const { return m_iPriority; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiTextureCubeResourceHandle m_hEnvironmentMap;
  float                        m_fIntensity  = 1.0f;
  float                        m_fSaturation = 1.0f;
  xiiInt32                     m_iPriority   = 0;
};
