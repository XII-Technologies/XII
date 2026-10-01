/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

struct xiiMsgExtractRenderData;

using xiiReflectionCaptureComponentManager = xiiComponentManager<class xiiReflectionCaptureComponent, xiiBlockStorageType::Compact>;

/// Shape used to evaluate a local reflection probe's influence volume.
struct xiiReflectionProbeInfluenceShape
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Sphere,
    Box,

    Default = Box
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiReflectionProbeInfluenceShape);

/// Immutable render packet extracted from an authored reflection capture.
///
/// The cubemap is expected to contain a full GGX pre-filter mip chain. Keeping
/// filtering in the asset pipeline avoids repeating expensive convolution for
/// static probes every frame while still allowing dynamic captures to replace
/// the handle when their asynchronous convolution completes.
class XII_GRAPHICSCORE_DLL xiiReflectionCaptureRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionCaptureRenderData, xiiRenderData);

public:
  xiiTextureCubeResourceHandle              m_hReflectionMap;
  xiiTransform                              m_GlobalTransform = xiiTransform::MakeIdentity();
  xiiVec3                                   m_vHalfExtents    = xiiVec3(5.0f);
  xiiEnum<xiiReflectionProbeInfluenceShape> m_InfluenceShape;
  float                                     m_fSphereRadius      = 5.0f;
  float                                     m_fBlendDistance     = 1.0f;
  float                                     m_fIntensity         = 1.0f;
  float                                     m_fSaturation        = 1.0f;
  xiiInt32                                  m_iPriority          = 0;
  bool                                      m_bParallaxCorrected = true;
};

/// Authors a local, pre-filtered image-based-lighting probe.
///
/// Multiple probes can overlap. The GPU selects the highest-priority probes
/// touching each lighting cluster and blends them using the configured fade
/// distance. Box probes optionally use parallax-corrected cubemap lookup.
class XII_GRAPHICSCORE_DLL xiiReflectionCaptureComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiReflectionCaptureComponent, xiiRenderComponent, xiiReflectionCaptureComponentManager);

public:
  xiiReflectionCaptureComponent();
  ~xiiReflectionCaptureComponent();

  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                                SetReflectionMap(const xiiTextureCubeResourceHandle& hReflectionMap);
  const xiiTextureCubeResourceHandle& GetReflectionMap() const;

  void                                      SetInfluenceShape(xiiEnum<xiiReflectionProbeInfluenceShape> shape);
  xiiEnum<xiiReflectionProbeInfluenceShape> GetInfluenceShape() const;

  void    SetHalfExtents(xiiVec3 vHalfExtents);
  xiiVec3 GetHalfExtents() const;

  void  SetSphereRadius(float fRadius);
  float GetSphereRadius() const;

  void  SetBlendDistance(float fDistance);
  float GetBlendDistance() const;

  void  SetIntensity(float fIntensity);
  float GetIntensity() const;

  void  SetSaturation(float fSaturation);
  float GetSaturation() const;

  void     SetPriority(xiiInt32 iPriority);
  xiiInt32 GetPriority() const;

  void SetParallaxCorrected(bool bEnabled);
  bool GetParallaxCorrected() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiTextureCubeResourceHandle              m_hReflectionMap;
  xiiEnum<xiiReflectionProbeInfluenceShape> m_InfluenceShape;
  xiiVec3                                   m_vHalfExtents       = xiiVec3(5.0f);
  float                                     m_fSphereRadius      = 5.0f;
  float                                     m_fBlendDistance     = 1.0f;
  float                                     m_fIntensity         = 1.0f;
  float                                     m_fSaturation        = 1.0f;
  xiiInt32                                  m_iPriority          = 0;
  bool                                      m_bParallaxCorrected = true;
};
