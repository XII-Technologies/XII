#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

using xiiLightProbeGridComponentManager = xiiComponentManager<class xiiLightProbeGridComponent, xiiBlockStorageType::Compact>;

/// \brief Render data submitted per-frame by a light probe grid component.
class XII_GRAPHICSCORE_DLL xiiLightProbeGridRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLightProbeGridRenderData, xiiRenderData);

public:
  xiiTexture3DResourceHandle m_hSHAtlas;      ///< 3D texture containing packed SH coefficients per probe.
  xiiVec3I32                 m_GridDims;      ///< Number of probes along X, Y, Z axes.
  xiiVec3                    m_vSpacing;      ///< World-space distance between probes on each axis.
  xiiUInt8                   m_uiSHOrder = 2; ///< 1 = L1 (4 coeffs), 2 = L2 (9 coeffs) per colour channel.
};

/// \brief Manages a 3D grid of spherical-harmonic irradiance probes for indirect diffuse GI.
///
/// Probe data is baked offline or updated at runtime by the LightProbeGridManager world module.
/// At runtime the component exposes the SH atlas texture to the lighting pass for GI interpolation.
class XII_GRAPHICSCORE_DLL xiiLightProbeGridComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLightProbeGridComponent, xiiRenderComponent, xiiLightProbeGridComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiLightProbeGridComponent();
  ~xiiLightProbeGridComponent();

  void              SetGridDims(const xiiVec3I32& dims);       // [ property ]
  const xiiVec3I32& GetGridDims() const { return m_GridDims; } // [ property ]

  void           SetSpacing(const xiiVec3& vSpacing);      // [ property ]
  const xiiVec3& GetSpacing() const { return m_vSpacing; } // [ property ]

  void     SetSHOrder(xiiUInt8 uiOrder);              // [ property ]
  xiiUInt8 GetSHOrder() const { return m_uiSHOrder; } // [ property ]

  void          SetSHAtlasFile(xiiStringView sFile); // [ property ]
  xiiStringView GetSHAtlasFile() const;              // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiTexture3DResourceHandle m_hSHAtlas;
  xiiVec3I32                 m_GridDims  = xiiVec3I32(8, 4, 8);
  xiiVec3                    m_vSpacing  = xiiVec3(2.0f);
  xiiUInt8                   m_uiSHOrder = 2;
};
