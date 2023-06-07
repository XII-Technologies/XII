#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct xiiMsgExtractRenderData;

using xiiRenderTargetComponentManager = class xiiRenderTargetActivatorComponent;

class XII_RENDERERCORE_DLL xiiRenderTargetActivatorComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRenderTargetActivatorComponent, xiiRenderComponent, xiiRenderTargetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderTargetActivatorComponent

public:
  xiiRenderTargetActivatorComponent();
  ~xiiRenderTargetActivatorComponent();

  void        SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;             // [ property ]

  void                               SetRenderTarget(const xiiRenderToTexture2DResourceHandle& hResource);
  xiiRenderToTexture2DResourceHandle GetRenderTarget() const { return m_hRenderTarget; }

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
};
