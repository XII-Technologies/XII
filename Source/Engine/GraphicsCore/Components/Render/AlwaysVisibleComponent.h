/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Components/Render/RenderComponent.h>

using xiiAlwaysVisibleComponentManager = xiiComponentManager<class xiiAlwaysVisibleComponent, xiiBlockStorageType::Compact>;

/// Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class XII_GRAPHICSCORE_DLL xiiAlwaysVisibleComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAlwaysVisibleComponent, xiiRenderComponent, xiiAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAlwaysVisibleComponent

public:
  xiiAlwaysVisibleComponent();
  ~xiiAlwaysVisibleComponent();
};
