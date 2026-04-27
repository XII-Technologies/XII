#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data submitted per-frame by an impostor component.
class XII_GRAPHICSCORE_DLL xiiImpostorRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImpostorRenderData, xiiRenderData);

public:
  xiiTexture2DResourceHandle m_hTexture;

  /// \brief World-space half-size of the billboard quad.
  float m_fHalfWidth  = 0.5f;
  float m_fHalfHeight = 0.5f;

  /// \brief Distance at which the impostor starts fading in (near) or out (far).
  float m_fFadeInDist  = 10.0f;
  float m_fFadeOutDist = 200.0f;

  /// \brief RGBA tint/opacity.
  xiiColor m_Color = xiiColor::White;
};

using xiiImpostorComponentManager = xiiComponentManager<class xiiImpostorComponent, xiiBlockStorageType::Compact>;

/// \brief Renders a camera-facing billboard impostor for distant-object representation.
///
/// Used to replace expensive geometry (trees, rocks, complex props) beyond a certain
/// view distance. The billboard rotates to always face the camera around the object's up axis.
class XII_GRAPHICSCORE_DLL xiiImpostorComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiImpostorComponent, xiiRenderComponent, xiiImpostorComponentManager);

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
  // xiiImpostorComponent

public:
  xiiImpostorComponent();
  ~xiiImpostorComponent();

  void          SetTextureFile(xiiStringView sFile); // [ property ]
  xiiStringView GetTextureFile() const;              // [ property ]

  void                              SetTexture(const xiiTexture2DResourceHandle& hTexture);
  const xiiTexture2DResourceHandle& GetTexture() const { return m_hTexture; }

  void  SetHalfWidth(float f);                        // [ property ]
  float GetHalfWidth() const { return m_fHalfWidth; } // [ property ]

  void  SetHalfHeight(float f);                         // [ property ]
  float GetHalfHeight() const { return m_fHalfHeight; } // [ property ]

  void  SetFadeInDist(float f);                         // [ property ]
  float GetFadeInDist() const { return m_fFadeInDist; } // [ property ]

  void  SetFadeOutDist(float f);                          // [ property ]
  float GetFadeOutDist() const { return m_fFadeOutDist; } // [ property ]

  void     SetColor(const xiiColor& color);     // [ property ]
  xiiColor GetColor() const { return m_Color; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiTexture2DResourceHandle m_hTexture;
  float                      m_fHalfWidth   = 0.5f;
  float                      m_fHalfHeight  = 0.5f;
  float                      m_fFadeInDist  = 10.0f;
  float                      m_fFadeOutDist = 200.0f;
  xiiColor                   m_Color        = xiiColor::White;
};
