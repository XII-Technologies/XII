#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Blend mode for the projected decal.
struct XII_GRAPHICSCORE_DLL xiiDecalBlendMode
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Opaque = 0,
    AlphaBlend,
    Additive,

    ENUM_COUNT,
    Default = AlphaBlend
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDecalBlendMode);

/// \brief Render data submitted per-frame by a decal component.
class XII_GRAPHICSCORE_DLL xiiDecalRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalRenderData, xiiRenderData);

public:
  xiiDecalResourceHandle     m_hDecal;
  xiiMat4                    m_ProjectionMatrix; ///< World-to-decal-local OBB projection.
  xiiEnum<xiiDecalBlendMode> m_BlendMode;
  float                      m_fAlpha     = 1.0f;
  float                      m_fAngleFade = 0.5f; ///< Normal-dot-projection fade threshold.
  xiiColorLinearUB           m_BaseColor  = xiiColorLinearUB(255, 255, 255, 255);
};

using xiiDecalComponentManager = xiiComponentManager<class xiiDecalComponent, xiiBlockStorageType::Compact>;

/// \brief Projects a decal texture onto scene geometry using an OBB volume.
///
/// The component defines an oriented projection box; any geometry inside the box receives
/// the decal atlas entry during the deferred decal render pass. Blend mode, opacity, and
/// normal-fade are configurable per component.
class XII_GRAPHICSCORE_DLL xiiDecalComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDecalComponent, xiiRenderComponent, xiiDecalComponentManager);

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
  // xiiDecalComponent

public:
  xiiDecalComponent();
  ~xiiDecalComponent();

  void          SetDecalFile(xiiStringView sFile); // [ property ]
  xiiStringView GetDecalFile() const;              // [ property ]

  void                          SetDecal(const xiiDecalResourceHandle& hDecal);
  const xiiDecalResourceHandle& GetDecal() const { return m_hDecal; }

  void                       SetBlendMode(xiiEnum<xiiDecalBlendMode> mode); // [ property ]
  xiiEnum<xiiDecalBlendMode> GetBlendMode() const { return m_BlendMode; }   // [ property ]

  void  SetAlpha(float fAlpha);               // [ property ]
  float GetAlpha() const { return m_fAlpha; } // [ property ]

  void  SetAngleFade(float fFade);                    // [ property ]
  float GetAngleFade() const { return m_fAngleFade; } // [ property ]

  void     SetBaseColor(const xiiColor& color); // [ property ]
  xiiColor GetBaseColor() const;                // [ property ]

  /// \brief Half-extents of the projection box in local space (X = depth axis).
  void    SetExtents(const xiiVec3& vExtents);      // [ property ]
  xiiVec3 GetExtents() const { return m_vExtents; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiDecalResourceHandle     m_hDecal;
  xiiVec3                    m_vExtents   = xiiVec3(0.5f);
  xiiEnum<xiiDecalBlendMode> m_BlendMode  = xiiDecalBlendMode::AlphaBlend;
  float                      m_fAlpha     = 1.0f;
  float                      m_fAngleFade = 0.5f;
  xiiColorLinearUB           m_BaseColor  = xiiColorLinearUB(255, 255, 255, 255);
};
