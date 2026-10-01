/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Vec2.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// Physical and procedural controls for a world-scale cloud layer.
struct XII_GRAPHICSCORE_DLL xiiVolumetricCloudSettings
{
  float   m_fLayerAltitudeMeters         = 2000.0f;
  float   m_fShadowScaleMeters           = 4000.0f;
  float   m_fDetailScale                 = 4.0f;
  float   m_fCoverage                    = 0.55f;
  float   m_fOpticalDepth                = 2.0f;
  float   m_fShadowStrength              = 1.0f;
  xiiVec2 m_vWindVelocityMetersPerSecond = xiiVec2(12.0f, 4.0f);
  bool    m_bCastShadows                 = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricCloudSettings);

/// Per-view cloud-layer data consumed by lighting and atmosphere passes.
class XII_GRAPHICSCORE_DLL xiiVolumetricCloudRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumetricCloudRenderData, xiiRenderData);

public:
  xiiVolumetricCloudSettings m_Settings;
  xiiVec3                    m_vLayerOrigin     = xiiVec3::MakeZero();
  xiiVec3                    m_vProjectionAxisU = xiiVec3(1.0f, 0.0f, 0.0f);
  xiiVec3                    m_vProjectionAxisV = xiiVec3(0.0f, 1.0f, 0.0f);
  xiiVec3                    m_vLayerNormal     = xiiVec3(0.0f, 0.0f, 1.0f);
  xiiInt32                   m_iPriority        = 0;
};

using xiiVolumetricCloudComponentManager = xiiComponentManager<class xiiVolumetricCloudComponent, xiiBlockStorageType::Compact>;

/// Authors a world-scale cloud layer. The highest-priority extracted layer is
/// selected per view; ties use the stable rendering ID.
class XII_GRAPHICSCORE_DLL xiiVolumetricCloudComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumetricCloudComponent, xiiRenderComponent, xiiVolumetricCloudComponentManager);

public:
  xiiVolumetricCloudComponent();
  ~xiiVolumetricCloudComponent();

  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                              SetCloudSettings(const xiiVolumetricCloudSettings& settings);
  const xiiVolumetricCloudSettings& GetCloudSettings() const;

  void     SetPriority(xiiInt32 iPriority);
  xiiInt32 GetPriority() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiVolumetricCloudSettings m_Settings;
  xiiInt32                   m_iPriority = 0;
};
