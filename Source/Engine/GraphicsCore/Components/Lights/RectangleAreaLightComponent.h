#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiRectAreaLightComponentManager = xiiComponentManager<class xiiRectAreaLightComponent, xiiBlockStorageType::Compact>;

/// \brief The render data object for rectangular area lights.
class XII_GRAPHICSCORE_DLL xiiRectAreaLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRectAreaLightRenderData, xiiLightRenderData);

public:
  float m_fWidth    = 1.0f;  ///< Emitter rectangle width in metres.
  float m_fHeight   = 1.0f;  ///< Emitter rectangle height in metres.
  bool  m_bTwoSided = false; ///< Whether the emitter is double-sided.
};

/// \brief A planar rectangular area light, useful for windows, monitors, and strip lights.
///
/// The light is emitted from the XY plane in local space. Width extends along +X and height
/// along +Y. Two-sided mode enables back-face emission.
class XII_GRAPHICSCORE_DLL xiiRectAreaLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRectAreaLightComponent, xiiLightComponent, xiiRectAreaLightComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiRectAreaLightComponent();
  ~xiiRectAreaLightComponent();

  void  SetWidth(float f);                    // [ property ]
  float GetWidth() const { return m_fWidth; } // [ property ]

  void  SetHeight(float f);                     // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

  void SetTwoSided(bool b);                        // [ property ]
  bool GetTwoSided() const { return m_bTwoSided; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float m_fWidth    = 1.0f;
  float m_fHeight   = 1.0f;
  bool  m_bTwoSided = false;
};
