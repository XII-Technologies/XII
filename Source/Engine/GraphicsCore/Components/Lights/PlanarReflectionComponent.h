#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>
#include <GraphicsCore/Declarations.h>

using xiiPlanarReflectionComponentManager = xiiComponentManager<class xiiPlanarReflectionComponent, xiiBlockStorageType::Compact>;

/// \brief Render data for a planar reflection component.
class XII_GRAPHICSCORE_DLL xiiPlanarReflectionRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlanarReflectionRenderData, xiiRenderData);

public:
  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
  xiiVec4                            m_ReflectionPlane = xiiVec4(0, 0, 1, 0); ///< Plane equation (nx,ny,nz,d) in world space.
  float                              m_fClipOffset     = 0.01f;               ///< Bias to avoid self-reflection artifacts.
  xiiUInt32                          m_uiWidth         = 512;
  xiiUInt32                          m_uiHeight        = 512;
};

/// \brief Renders a real-time planar mirror/reflection into a render target.
///
/// The component defines the reflection plane via its game object's local XY plane. The view
/// is mirrored across this plane and rendered into the owned render target, which can be referenced
/// in materials for water, glossy floors, and mirror surfaces.
class XII_GRAPHICSCORE_DLL xiiPlanarReflectionComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPlanarReflectionComponent, xiiRenderComponent, xiiPlanarReflectionComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiPlanarReflectionComponent();
  ~xiiPlanarReflectionComponent();

  void  SetClipOffset(float f);                         // [ property ]
  float GetClipOffset() const { return m_fClipOffset; } // [ property ]

  void      SetWidth(xiiUInt32 w);                 // [ property ]
  xiiUInt32 GetWidth() const { return m_uiWidth; } // [ property ]

  void      SetHeight(xiiUInt32 h);                  // [ property ]
  xiiUInt32 GetHeight() const { return m_uiHeight; } // [ property ]

  const xiiRenderToTexture2DResourceHandle& GetRenderTarget() const { return m_hRenderTarget; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
  float                              m_fClipOffset = 0.01f;
  xiiUInt32                          m_uiWidth     = 512;
  xiiUInt32                          m_uiHeight    = 512;
};
