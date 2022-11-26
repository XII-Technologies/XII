#pragma once

#include <RendererCore/Components/RenderComponent.h>

typedef xiiComponentManager<class xiiAlwaysVisibleComponent, xiiBlockStorageType::Compact> xiiAlwaysVisibleComponentManager;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class XII_RENDERERCORE_DLL xiiAlwaysVisibleComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAlwaysVisibleComponent, xiiRenderComponent, xiiAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAlwaysVisibleComponent

public:
  xiiAlwaysVisibleComponent();
  ~xiiAlwaysVisibleComponent();
};
