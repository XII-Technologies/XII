#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

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

  /// \brief Called by xiiRenderComponent::OnUpdateLocalBounds().
  /// If XII_SUCCESS is returned, \a bounds and \a bAlwaysVisible will be integrated into the xiiMsgUpdateLocalBounds result,
  /// otherwise the out values are simply ignored.
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) = 0;

  void TriggerLocalBoundsUpdate();

  static xiiUInt32 GetUniqueIdForRendering(const xiiComponent* pComponent, xiiUInt32 uiInnerIndex = 0, xiiUInt32 uiInnerIndexShift = 24);

  XII_ALWAYS_INLINE xiiUInt32 GetUniqueIdForRendering(xiiUInt32 uiInnerIndex = 0, xiiUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
