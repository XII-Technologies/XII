#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/World.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Textures/RenderToTexture2DResource.h>

struct xiiMsgExtractRenderData;

using xiiRenderTargetComponentManager = xiiComponentManager<class xiiRenderTargetActivatorComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiRenderTargetActivatorComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRenderTargetActivatorComponent, xiiRenderComponent, xiiRenderTargetComponentManager);

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
  // xiiRenderTargetActivatorComponent

public:
  xiiRenderTargetActivatorComponent();
  ~xiiRenderTargetActivatorComponent();

  void                                      SetRenderTarget(const xiiRenderToTexture2DResourceHandle& hResource); // [property]
  const xiiRenderToTexture2DResourceHandle& GetRenderTarget() const { return m_hRenderTarget; }                   // [property]

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
};
