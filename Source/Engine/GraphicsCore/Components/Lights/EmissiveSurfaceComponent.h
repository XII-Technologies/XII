#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiEmissiveSurfaceComponentManager = xiiComponentManager<class xiiEmissiveSurfaceComponent, xiiBlockStorageType::Compact>;

/// \brief Render data for emissive surface mesh-based lights.
class XII_GRAPHICSCORE_DLL xiiEmissiveSurfaceRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEmissiveSurfaceRenderData, xiiRenderData);

public:
  xiiColorLinearUB m_EmissiveColor   = xiiColorLinearUB(255, 255, 255, 255);
  float            m_fIntensityScale = 1.0f; ///< Multiplier on top of the material's emissive channel.
};

/// \brief Tags a mesh as an emissive-surface light source for area lighting approximations.
///
/// The component does not own the mesh; it relies on a co-located StaticMeshComponent for geometry.
/// It supplies emissive colour and intensity so that the lighting system can register the mesh as
/// an analytic area emitter (e.g. LTC-based rectangle/sphere light).
class XII_GRAPHICSCORE_DLL xiiEmissiveSurfaceComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiEmissiveSurfaceComponent, xiiRenderComponent, xiiEmissiveSurfaceComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiEmissiveSurfaceComponent();
  ~xiiEmissiveSurfaceComponent();

  void     SetEmissiveColor(const xiiColor& color); // [ property ]
  xiiColor GetEmissiveColor() const;                // [ property ]

  void  SetIntensityScale(float f);                             // [ property ]
  float GetIntensityScale() const { return m_fIntensityScale; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiColorLinearUB m_EmissiveColor   = xiiColorLinearUB(255, 255, 255, 255);
  float            m_fIntensityScale = 1.0f;
};
