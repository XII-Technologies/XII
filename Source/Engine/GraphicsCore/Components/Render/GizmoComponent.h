#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Selects the visual style of a gizmo helper.
struct XII_GRAPHICSCORE_DLL xiiGizmoType
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    TranslationAxes = 0, ///< Three axis arrows.
    RotationRings,       ///< Three rotation rings.
    ScaleHandles,        ///< Uniform and per-axis scale boxes.
    BoundingBox,         ///< Axis-aligned bounding box outline.
    Sphere,              ///< Sphere wireframe.
    Custom,              ///< Mesh specified by the component.

    ENUM_COUNT,
    Default = TranslationAxes
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGizmoType);

/// \brief Render data submitted per-frame by a gizmo component.
class XII_GRAPHICSCORE_DLL xiiGizmoRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoRenderData, xiiRenderData);

public:
  xiiEnum<xiiGizmoType> m_GizmoType;
  xiiColor              m_Color     = xiiColor::White;
  float                 m_fScale    = 1.0f;
  bool                  m_bSelected = false;
};

using xiiGizmoComponentManager = xiiComponentManager<class xiiGizmoComponent, xiiBlockStorageType::Compact>;

/// \brief Renders an editor/debug visual helper gizmo in world space.
///
/// Gizmos are always visible (not frustum culled) and typically only rendered in editor
/// views. They carry no gameplay relevance. The rendered shape is determined by xiiGizmoType.
class XII_GRAPHICSCORE_DLL xiiGizmoComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGizmoComponent, xiiRenderComponent, xiiGizmoComponentManager);

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
  // xiiGizmoComponent

public:
  xiiGizmoComponent();
  ~xiiGizmoComponent();

  void                  SetGizmoType(xiiEnum<xiiGizmoType> type);    // [ property ]
  xiiEnum<xiiGizmoType> GetGizmoType() const { return m_GizmoType; } // [ property ]

  void     SetColor(const xiiColor& color);     // [ property ]
  xiiColor GetColor() const { return m_Color; } // [ property ]

  void  SetScale(float fScale);               // [ property ]
  float GetScale() const { return m_fScale; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiEnum<xiiGizmoType> m_GizmoType = xiiGizmoType::BoundingBox;
  xiiColor              m_Color     = xiiColor::White;
  float                 m_fScale    = 1.0f;
};
