#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// \brief This class can be used to define the render targets to be used by a xiiView.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderTargets
{
  bool operator==(const xiiGALRenderTargets& other) const;
  bool operator!=(const xiiGALRenderTargets& other) const;

  xiiGALTextureHandle m_hRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiGALTextureHandle m_hDSTarget;
};

// \brief This class can be used to construct render target setups on the stack.
class XII_GRAPHICSFOUNDATION_DLL xiiGALRenderTargetSetup
{
public:
  xiiGALRenderTargetSetup();

  xiiGALRenderTargetSetup& SetRenderTarget(xiiUInt8 uiIndex, xiiGALTextureViewHandle hRenderTarget);
  xiiGALRenderTargetSetup& SetDepthStencilTarget(xiiGALTextureViewHandle hDSTarget);

  bool operator==(const xiiGALRenderTargetSetup& other) const;
  bool operator!=(const xiiGALRenderTargetSetup& other) const;

  inline xiiUInt8 GetRenderTargetCount() const;

  inline xiiGALTextureViewHandle GetRenderTarget(xiiUInt8 uiIndex) const;
  inline xiiGALTextureViewHandle GetDepthStencilTarget() const;

  void DestroyAllAttachedViews();

protected:
  xiiGALTextureViewHandle m_hRTs[XII_GAL_MAX_RENDERTARGET_COUNT];
  xiiGALTextureViewHandle m_hDSTarget;

  xiiUInt8 m_uiRTCount = 0;
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderingSetup
{
  bool operator==(const xiiGALRenderingSetup& other) const;
  bool operator!=(const xiiGALRenderingSetup& other) const;

  xiiGALRenderTargetSetup m_RenderTargetSetup;
  xiiUInt32               m_uiRenderTargetClearMask = 0x0U;
  bool                    m_bClearDepth             = false;
  bool                    m_bClearStencil           = false;
  bool                    m_bDiscardColor           = false;
  bool                    m_bDiscardDepth           = false;
};

#include <GraphicsFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
