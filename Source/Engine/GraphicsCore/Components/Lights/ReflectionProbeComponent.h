#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>
#include <GraphicsCore/Declarations.h>

using xiiReflectionProbeComponentManager = xiiComponentManager<class xiiReflectionProbeComponent, xiiBlockStorageType::Compact>;

/// \brief Render data submitted per-frame by a reflection probe.
class XII_GRAPHICSCORE_DLL xiiReflectionProbeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeRenderData, xiiRenderData);

public:
  xiiTextureCubeResourceHandle m_hCubeMap;
  xiiVec3                      m_vCapturePosition = xiiVec3::MakeZero();
  float                        m_fInfluenceRadius = 10.0f;
  float                        m_fBlendWeight     = 1.0f;
  bool                         m_bRealtime        = false;
};

/// \brief Captures or references a cube-map reflection probe used for specular IBL.
///
/// The probe can be either baked (reference a pre-captured cube-map asset) or realtime
/// (request a dynamic re-capture every N frames). The influence radius and blend weight
/// control how the probe blends with adjacent probes and the sky.
class XII_GRAPHICSCORE_DLL xiiReflectionProbeComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiReflectionProbeComponent, xiiRenderComponent, xiiReflectionProbeComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiReflectionProbeComponent();
  ~xiiReflectionProbeComponent();

  void          SetCubeMapFile(xiiStringView sFile); // [ property ]
  xiiStringView GetCubeMapFile() const;              // [ property ]

  void  SetInfluenceRadius(float f);                              // [ property ]
  float GetInfluenceRadius() const { return m_fInfluenceRadius; } // [ property ]

  void  SetBlendWeight(float f);                          // [ property ]
  float GetBlendWeight() const { return m_fBlendWeight; } // [ property ]

  void SetRealtime(bool b);                        // [ property ]
  bool GetRealtime() const { return m_bRealtime; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiTextureCubeResourceHandle m_hCubeMap;
  float                        m_fInfluenceRadius = 10.0f;
  float                        m_fBlendWeight     = 1.0f;
  bool                         m_bRealtime        = false;
};
