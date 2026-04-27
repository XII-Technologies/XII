#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data for an offscreen render target component.
class XII_GRAPHICSCORE_DLL xiiRenderTargetRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderTargetRenderData, xiiRenderData);

public:
  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
  xiiUInt32                          m_uiWidth       = 512;
  xiiUInt32                          m_uiHeight      = 512;
  xiiUInt8                           m_uiMSAASamples = 1;
};

using xiiRenderTargetComponentManager = xiiComponentManager<class xiiRenderTargetComponent, xiiBlockStorageType::Compact>;

/// \brief Owns an offscreen render target and exposes it as a texture for use in materials or post-process.
class XII_GRAPHICSCORE_DLL xiiRenderTargetComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRenderTargetComponent, xiiComponent, xiiRenderTargetComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

public:
  xiiRenderTargetComponent();
  ~xiiRenderTargetComponent();

  void      SetWidth(xiiUInt32 uiWidth);           // [ property ]
  xiiUInt32 GetWidth() const { return m_uiWidth; } // [ property ]

  void      SetHeight(xiiUInt32 uiHeight);           // [ property ]
  xiiUInt32 GetHeight() const { return m_uiHeight; } // [ property ]

  void     SetMSAASamples(xiiUInt8 uiSamples);                // [ property ]
  xiiUInt8 GetMSAASamples() const { return m_uiMSAASamples; } // [ property ]

  const xiiRenderToTexture2DResourceHandle& GetRenderTarget() const { return m_hRenderTarget; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  void RecreateRenderTarget();

  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
  xiiUInt32                          m_uiWidth       = 512;
  xiiUInt32                          m_uiHeight      = 512;
  xiiUInt8                           m_uiMSAASamples = 1;
};
