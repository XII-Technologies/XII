#pragma once

#include <RendererCore/Components/RenderComponent.h>

using xiiAlwaysVisibleComponentManager = class xiiAlwaysVisibleComponent;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class XII_RENDERERCORE_DLL xiiAlwaysVisibleComponent : public xiiRenderComponent
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
