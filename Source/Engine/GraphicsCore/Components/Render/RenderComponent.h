/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <GraphicsCore/Pipeline/RenderExtraction.h>

/// Base class for objects that should be rendered.
class XII_GRAPHICSCORE_DLL xiiRenderComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiRenderComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  xiiRenderComponent();
  ~xiiRenderComponent();
  virtual xiiBitflags<xiiRenderExtractionCategory> GetRenderExtractionCategories() const { return xiiRenderExtractionCategory::All; }

  /// Opt in only when packets are independent of cameras and extraction requirement sets.
  /// Shared packets are immutable until the CPU rendering boundary has finished.
  virtual bool IsRenderDataViewIndependent() const { return false; }

  /// Called by xiiRenderComponent::OnUpdateLocalBounds().
  ///
  /// If XII_SUCCESS is returned, out_bounds and out_bAlwaysVisible will be integrated into the xiiMsgUpdateLocalBounds ref_msg,
  /// otherwise the out values are simply ignored.
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) = 0;

  /// Call this when some value was modified that affects the size of the local bounding box and it should be recomputed.
  void TriggerLocalBoundsUpdate();

  /// Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  static xiiUInt32 GetUniqueIdForRendering(const xiiComponent& component, xiiUInt32 uiInnerIndex = 0, xiiUInt32 uiInnerIndexShift = 24);

  /// Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  XII_ALWAYS_INLINE xiiUInt32 GetUniqueIdForRendering(xiiUInt32 uiInnerIndex = 0, xiiUInt32 uiInnerIndexShift = 24) const { return GetUniqueIdForRendering(*this, uiInnerIndex, uiInnerIndexShift); }

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
  void OnTransformChanged(xiiMsgTransformChanged& msg);
};
