/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiRectangleAreaLightComponentManager = xiiComponentManager<class xiiRectangleAreaLightComponent, xiiBlockStorageType::Compact>;

/// The render data object for rectangle area lights.
class XII_GRAPHICSCORE_DLL xiiRectangleAreaLightRenderData : public xiiLightRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRectangleAreaLightRenderData, xiiLightRenderData);

public:
  xiiVec2 m_vExtents;
  xiiQuat m_qGlobalRotation;
};

class XII_GRAPHICSCORE_DLL xiiRectangleAreaLightComponent : public xiiLightComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRectangleAreaLightComponent, xiiLightComponent, xiiRectangleAreaLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  /////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  /////////////////////////////////////////////////////////////////////////
  // xiiRectangleAreaLightComponent

public:
  xiiRectangleAreaLightComponent();
  ~xiiRectangleAreaLightComponent();

  /// Sets the extents of the rectangle area light. The light is emitted from a rectangle centered on the position of the component, with the normal facing in negative X direction.
  void    SetExtents(xiiVec2 vExtents); // [ property ]
  xiiVec2 GetExtents() const;           // [ property ]

protected:
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiBoundingSphere CalculateBoundingSphere(const xiiTransform& transform, const xiiVec2& vExtents) const;

protected:
  xiiVec2 m_vExtents = xiiVec2(1.0f, 1.0f);
};

/// Visualizer attribute for rectangle area lights.
class xiiRectangleAreaLightVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRectangleAreaLightVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiRectangleAreaLightVisualizerAttribute();
  xiiRectangleAreaLightVisualizerAttribute(xiiStringView sExtentsProperty, xiiStringView sColorProperty, xiiStringView sIntensityProperty);

  xiiUntrackedString m_sExtentsProperty;
  xiiUntrackedString m_sColorProperty;
  xiiUntrackedString m_sIntensityProperty;
};
